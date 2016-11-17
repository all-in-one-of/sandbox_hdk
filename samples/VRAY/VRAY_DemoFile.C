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
 * This is a sample procedural DSO
 */

#include <UT/UT_DSOVersion.h>
#include <GU/GU_Detail.h>
#include "VRAY_DemoFile.h"

using namespace HDK_Sample;

static VRAY_ProceduralArg	theArgs[] = {
    VRAY_ProceduralArg("file",		"string",	""),
    VRAY_ProceduralArg("blurfile",	"string",	""),
    VRAY_ProceduralArg("velocityblur",	"int",		"0"),
    VRAY_ProceduralArg("shutter",	"real",		"1"),
    VRAY_ProceduralArg()
};

VRAY_Procedural *
allocProcedural(const char *)
{
    return new VRAY_DemoFile();
}

const VRAY_ProceduralArg *
getProceduralArgs(const char *)
{
    return theArgs;
}

VRAY_DemoFile::VRAY_DemoFile()
    : myShutter(1),
      myPreBlur(0),
      myPostBlur(0)
{
    myBox.initBounds(0, 0, 0);
}

VRAY_DemoFile::~VRAY_DemoFile()
{
}

const char *
VRAY_DemoFile::className() const
{
    return "VRAY_DemoFile";
}

int
VRAY_DemoFile::initialize(const UT_BoundingBox *box)
{
    int		ival;

    // The user is required to specify the bounds of the geometry.
    // Alternatively, if the bounds aren't specified, we could forcibly load
    // the geometry at this point and compute the bounds ourselves.
    if (!box)
    {
	fprintf(stderr, "The %s procedural needs a bounding box specified\n",
		    className());
	return 0;
    }

    // Stash away the box the user specified
    myBox = *box;
    
    // Import the shutter time.  This is a scale between 0 and 1
    if (!import("shutter", &myShutter, 1))
	myShutter = 1;

    // A toggle for whether to use velocityblur or not.  Since there's no
    // "bool" type for parameters, import into an int.
    if (import("velocityblur", &ival, 1))
	myVelocityBlur = (ival != 0);
    else
	myVelocityBlur = false;

    // Import the filenames for t0 and t1
    import("file", myFile);
    import("blurfile", myBlurFile);

    // Import the shutter settings for velocity blur. The 'camera:shutter'
    // stores the start and end of the shutter window in fraction of
    // a frame. Divide by the current FPS value to get the correct
    // scaling for velocity blur.
    fpreal	fps = 24.0, shutter[2] = {0};
    import("global:fps", &fps, 1);
    import("camera:shutter", shutter, 2);
    myPreBlur = -(myShutter * shutter[0]) / fps;
    myPostBlur = (myShutter * shutter[1]) / fps;

    return 1;
}

void
VRAY_DemoFile::getBoundingBox(UT_BoundingBox &box)
{
    box = myBox;
}

void
VRAY_DemoFile::render()
{
    // Allocate geometry.
    // Warning:  When allocating geometry for a procedural, do not simply
    // construct a GU_Detail, but call VRAY_Procedural::createGeometry().
    VRAY_ProceduralGeo	g0 = createGeometry();

    // Load geometry from disk
    if (!g0->load(myFile, 0).success())
    {
	fprintf(stderr, "Unable to load geometry[0]: '%s'\n",
			myFile.c_str());
	return;
    }

    // Add geometry to mantra.
    if (myVelocityBlur)
    {
	// If performing velocity blur, then we add velocity blur geometry
	g0.addVelocityBlur(myPreBlur, myPostBlur);
    }
    else
    {
	// Otherwise, we check to see if there's a motion blur geometry file
	if (myBlurFile.isstring())
	{
	    VRAY_ProceduralGeo	g1 = g0.appendSegmentGeometry(myShutter);
	    if (!g1->load(myBlurFile, 0).success())
	    {
		fprintf(stderr, "Unable to load geometry[1]: '%s'\n",
			myBlurFile.c_str());
		g0.removeSegmentGeometry(g1);
	    }
	}
    }
    VRAY_ProceduralChildPtr	obj = createChild();
    obj->addGeometry(g0);
}
