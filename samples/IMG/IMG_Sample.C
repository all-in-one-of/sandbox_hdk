/*
 * Copyright (c) 2015
 *	Side Effects Software Inc.  All rights reserved.
 *
 * Redistribution and use of Houdini Development Kit samples in source and
 * binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. The name of Side Effects Software may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SIDE EFFECTS SOFTWARE `AS IS' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL SIDE EFFECTS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *----------------------------------------------------------------------------
 * Read/Write raw files
 */

#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <UT/UT_Endian.h>		// For byte swapping
#include <UT/UT_DSOVersion.h>
#include <UT/UT_SysClone.h>
#include <IMG/IMG_Format.h>
#include "IMG_Sample.h"

#define MAGIC		0x1234567a
#define MAGIC_SWAP	0xa7654321		// Swapped magic number

namespace HDK_Sample {
typedef struct {
    unsigned int	magic;		// Magic number
    unsigned int	xres;		// Width of image
    unsigned int	yres;		// Height of image
    unsigned int	model;		// My color model
    unsigned int	data;		// Data type
} IMG_SampleHeader;

/// Custom image file format definition.  This class defines the properties of
/// the custom image format.
/// @see IMG_Sample
class IMG_SampleFormat : public IMG_Format {
public:
	     IMG_SampleFormat() {}
    virtual ~IMG_SampleFormat() {}

    virtual const char	*getFormatName() const;
    virtual const char	*getFormatLabel() const;
    virtual const char	*getFormatDescription() const;
    virtual const char	*getDefaultExtension() const;
    virtual IMG_File	*createFile()	 const;

    // Methods to determine if this is one of our recognized files.
    //	The extension is the first try.  If there are multiple matches,
    //	then we resort to the magic number (when reading)
    virtual int		 checkExtension(const char *filename) const;
    virtual int		 checkMagic(unsigned int) const;
    virtual IMG_DataType	 getSupportedTypes() const
	{ return IMG_DT_ALL; }
    virtual IMG_ColorModel       getSupportedColorModels() const
	{ return IMG_CM_ALL; }

    // Configuration information for the format
    virtual void		getMaxResolution(unsigned &x,
					         unsigned &y) const;

    virtual int			isReadRandomAccess() const  { return 0; }
    virtual int			isWriteRandomAccess() const { return 0; }
};
}	// End HDK_Sample namespace

using namespace HDK_Sample;

const char *
IMG_SampleFormat::getFormatName() const
{
    // Very brief label (no spaces)
    return "Sample";
}

const char *
IMG_SampleFormat::getFormatLabel() const
{
    // A simple description of the format
    return "Sample HDK Format";
}

const char *
IMG_SampleFormat::getFormatDescription() const
{
    // A more verbose description of the image format.  Things you might put in
    // here are the version of the format, etc.
    return "HDK Sample image format.  Not very useful";
}

const char *
IMG_SampleFormat::getDefaultExtension() const
{
    // The default extension for the format files.  If there is no default
    // extension, the format won't appear in the menus to choose image format
    // types.
    return "smp";
}

IMG_File *
IMG_SampleFormat::createFile() const
{
    return new IMG_Sample;
}

int
IMG_SampleFormat::checkExtension(const char *filename) const
{
    static const char	*extensions[] = { "smp", ".SMP", 0 };
    return matchExtensions(filename, extensions);
}

int
IMG_SampleFormat::checkMagic(unsigned int magic) const
{
    // Check if we hit our magic number
    return (magic == MAGIC || magic == MAGIC_SWAP);
}

void
IMG_SampleFormat::getMaxResolution(unsigned &x, unsigned &y) const
{
    x = UINT_MAX;		// Stored as shorts
    y = UINT_MAX;
}


//////////////////////////////////////////////////////////////////
//
//  Sample file loader/saver
//
//////////////////////////////////////////////////////////////////

IMG_Sample::IMG_Sample()
{
    myByteSwap = 0;
}

IMG_Sample::~IMG_Sample()
{
    close();
}

int
IMG_Sample::open()
{
    return readHeader();
}

/// Default texture options passed down by mantra.
/// See also the vm_saveoption SOHO setting.
static const char	*theTextureOptions[] = {
    "camera:orthowidth",	// Orthographic camera width
    "camera:zoom",		// Perspective camera zoom
    "camera:projection",	// 0 = perspective, 1 = orthographic, etc.
    "image:crop",		// Crop window (xmin, xmax, ymin, ymax)
    "image:window",		// Screen window (x0, x1, y0, y1)
    "image:pixelaspect",	// Pixel aspect ratio (not frame aspect)
    "image:samples",		// Sampling information
    "space:world",		// World space transform of camera
    NULL
};

static void
writeTextureOption(const char *token, const char *value)
{
    //cout << "Sample: " << token << " := " << value << endl;
}

int
IMG_Sample::create(const IMG_Stat &stat)
{
    // Store the image stats and write out the header.
    myStat = stat;
    if (!writeHeader())
	return 0;

    // When mantra renders to this format, options set in the vm_saveoption
    // string will be passed down to the image format.  This allows you to
    // query information about the renderer settings.  This is optional of
    // course.
    for (int i = 0; theTextureOptions[i]; ++i)
    {
	const char	*value;
	value = getOption(theTextureOptions[i]);
	if (value)
	    writeTextureOption(theTextureOptions[i], value);
    }
    return true;
}

int
IMG_Sample::closeFile()
{
    // If we're writing data, flush out the stream
    if (myOS) myOS->flush();	// Flush out the data

    return 1;	// return success
}

static inline void
swapHeader(IMG_SampleHeader &header)
{
    UTswapBytes((int *)&header, sizeof(header));
}

int
IMG_Sample::readHeader()
{
    IMG_SampleHeader	header;
    IMG_Plane		*plane;

    if (!readBytes((char *)&header, sizeof(IMG_SampleHeader)))
	return 0;

    if (header.magic == MAGIC_SWAP)
    {
	myByteSwap = 1;
	swapHeader(header);
    }
    else if (header.magic != MAGIC)
	return 0;			// Magic number failed.

    myStat.setResolution(header.xres, header.yres);
    plane = myStat.addDefaultPlane();
    plane->setColorModel((IMG_ColorModel)header.model);
    plane->setDataType((IMG_DataType)header.data);

    // Now, we're ready to read the data.
    return 1;
}

int
IMG_Sample::writeHeader()
{
    IMG_SampleHeader	header;

    header.magic = MAGIC;		// Always create native byte order
    header.xres = myStat.getXres();
    header.yres = myStat.getYres();
    header.model = myStat.getPlane()->getColorModel();
    header.data  = myStat.getPlane()->getDataType();

    if (!myOS->write((char *)&header, sizeof(IMG_SampleHeader)))
	return 0;

    // Now, we're ready to write the scanlines...
    return 1;
}

int
IMG_Sample::readScanline(int y, void *buf)
{
    int		nbytes;

    if (y >= myStat.getYres()) return 0;

    nbytes = myStat.bytesPerScanline();
    if (!readBytes((char *)buf, nbytes))
	return 0;

    // If the file was written on a different architecture, we might need to
    // swap the data.
    if (myByteSwap)
    {
	switch (myStat.getPlane()->getDataType())
	{
	    case IMG_UCHAR:	break;		// Nope
  	    case IMG_FLOAT16:
	    case IMG_USHORT:
		UTswapBytes((short *)buf, nbytes/sizeof(short));
		break;
	    case IMG_UINT:
		UTswapBytes((int *)buf, nbytes/sizeof(int));
		break;
	    case IMG_FLOAT:
		UTswapBytes((float *)buf, nbytes/sizeof(float));
		break;
	    default:
		break;
	}
    }

    return 1;
}

int
IMG_Sample::writeScanline(int /*y*/, const void *buf)
{
    // If we specified a translator in creation, the buf passed in will be in
    // the format we want, that is, the translator will make sure the data is
    // in the correct format.

    // Since we always write in native format, we don't have to swap
    return (!myOS->write((char *)buf, myStat.bytesPerScanline())) ? 0 : 1;
}

////////////////////////////////////////////////////////////////////
//
//  Now, we load the format
//
////////////////////////////////////////////////////////////////////
void
newIMGFormat(void *)
{
    new IMG_SampleFormat();
}
