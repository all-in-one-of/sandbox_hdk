/*
 * Copyright (c) 2017
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
 */

#include "f3d_io.h"
#include <UT/UT_DSOVersion.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimVolume.h>
#include <GEO/GEO_AttributeHandle.h>
#include <GEO/GEO_IOTranslator.h>
#include <SOP/SOP_Node.h>
#include <UT/UT_Assert.h>
#include <UT/UT_IOTable.h>

#include <Field3D/InitIO.h>

#include <iostream>
#include <stdio.h>


namespace HDK_Sample {

class GEO_Field3DIOTranslator : public GEO_IOTranslator
{
public:
	     GEO_Field3DIOTranslator() {}
    virtual ~GEO_Field3DIOTranslator() {}

    virtual GEO_IOTranslator *duplicate() const;

    virtual const char *formatName() const;

    virtual int		checkExtension(const char *name);

    virtual int		checkMagicNumber(unsigned magic);

    virtual GA_Detail::IOStatus fileLoad(GEO_Detail *gdp, UT_IStream &is, bool ate_magic);
    virtual GA_Detail::IOStatus fileSave(const GEO_Detail *gdp, std::ostream &os);
    virtual GA_Detail::IOStatus fileSaveToFile(const GEO_Detail *gdp, const char *fname);
};

}

using namespace HDK_Sample;

GEO_IOTranslator *
GEO_Field3DIOTranslator::duplicate() const
{
    return new GEO_Field3DIOTranslator();
}

const char *
GEO_Field3DIOTranslator::formatName() const
{
    return "Field3D Volume Format";
}

int
GEO_Field3DIOTranslator::checkExtension(const char *name) 
{
    UT_String		sname(name);

    if (sname.fileExtension() && !strcmp(sname.fileExtension(), ".f3d"))
	return true;
    return false;
}

int
GEO_Field3DIOTranslator::checkMagicNumber(unsigned magic)
{
    return 0;
}

GA_Detail::IOStatus
GEO_Field3DIOTranslator::fileLoad(GEO_Detail *gdp, UT_IStream &is, bool ate_magic)
{
    UT_WorkBuffer			buf;

    if (!is.isRandomAccessFile(buf))
    {
	std::cerr << "Error: Attempt to load Field3D from non-file source.\n";
	return false;
    }

    return f3d_fileLoad(gdp, buf.buffer());
}

GA_Detail::IOStatus
GEO_Field3DIOTranslator::fileSave(const GEO_Detail *gdp, std::ostream &os)
{
    return false;
}

GA_Detail::IOStatus
GEO_Field3DIOTranslator::fileSaveToFile(const GEO_Detail *gdp, const char *fname)
{
    if (!fname)
	return false;

    return f3d_fileSave(gdp, fname, F3D_BITDEPTH_AUTO, F3D_GRIDTYPE_SPARSE, true);
}

void
newGeometryIO(void *)
{
    Field3D::initIO();
    GU_Detail::registerIOTranslator(new GEO_Field3DIOTranslator());

    UT_ExtensionList		*geoextension;
    geoextension = UTgetGeoExtensions();
    if (!geoextension->findExtension("f3d"))
	geoextension->addExtension("f3d");
}
