/*
 * Copyright (c) 2005
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

#include "VRAY_DemoMountain.h"
#include <UT/UT_DSOVersion.h>
#include <GEO/GEO_AttributeHandle.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPoly.h>

using namespace HDK_Sample;

#define MAX_SPLITS	8

#define UT_DEBUG	1
#include <UT/UT_Counter.h>
static UT_Counter	theCount("Mountains");
static UT_Counter	theTotal("TotalMountains");

static VRAY_ProceduralArg	theArgs[] = {
    VRAY_ProceduralArg("p0",	"real",	"-1 -1 0"),
    VRAY_ProceduralArg("p1",	"real",	" 1 -1 0"),
    VRAY_ProceduralArg("p2",	"real",	" 0  1 0"),
    VRAY_ProceduralArg()
};

VRAY_Procedural *
allocProcedural(const char *)
{
    return new VRAY_DemoMountain();
}

const VRAY_ProceduralArg *
getProceduralArgs(const char *)
{
    return theArgs;
}

VRAY_DemoMountain::VRAY_DemoMountain(int splits)
    : mySplits(splits)
{
    theCount++;
    theTotal++;
}

VRAY_DemoMountain::~VRAY_DemoMountain()
{
    theCount--;
}

const char *
VRAY_DemoMountain::className() const
{
    return "VRAY_DemoMountain";
}

int
VRAY_DemoMountain::initialize(const UT_BoundingBox *)
{
    uint		seed = 0xbabecafe;

    myP[0].assign(-1, -1, 0, seed);
    SYSfastRandom(seed);		// Permute seed
    myP[1].assign( 1, -1, 0, seed);
    SYSfastRandom(seed);		// Permute seed
    myP[2].assign( 0, -1, 0, seed);
    mySplits = 1;

    // Get parameter values
    import("p0", myP[0].pos.data(), 3);
    import("p1", myP[1].pos.data(), 3);
    import("p2", myP[2].pos.data(), 3);
    return 1;
}

void
VRAY_DemoMountain::computeBounds(UT_BoundingBox &box, bool include_displace)
{
    box.initBounds(myP[0].pos);
    box.enlargeBounds(myP[1].pos);
    box.enlargeBounds(myP[2].pos);
    if (include_displace)
    {
	fpreal	bounds = 0.5 / (fpreal)mySplits;
	box.expandBounds(0, bounds);
    }
}


void
VRAY_DemoMountain::getBoundingBox(UT_BoundingBox &box)
{
    computeBounds(box, true);
    fpreal	bounds = 0.5 / (fpreal)mySplits;
    box.initBounds(myP[0].pos);
    box.enlargeBounds(myP[1].pos);
    box.enlargeBounds(myP[2].pos);
    box.expandBounds(0, bounds);
}

void
VRAY_DemoMountain::render()
{
    fpreal		 lod;
    UT_BoundingBox	 box;

    // Invoke the measuring code on the bounding box to determine the level of
    // detail.  The level of detail is the square root of the number of pixels
    // covered by the bounding box (with all shading factors considered).
    // However, for computing LOD, we do *not* want to include displacement
    // bounds
    computeBounds(box, false);
    lod = getLevelOfDetail(box);

    if (lod < 6 || mySplits > MAX_SPLITS)
    {
	// Render geometry
	fractalRender();
    }
    else
    {
	// Split into child procedurals
	fractalSplit();
    }
}

static void
edgeSplit(FractalPoint &P, const FractalPoint &p0, const FractalPoint &p1,
		fpreal scale)
{
    uint	seed;
    fpreal	disp;
    
    seed = (p0.seed + p1.seed)/2;
    disp = SYSfastRandomZero(seed)*scale;
    P.seed = seed;
    P.pos = (p0.pos + p1.pos) * .5;
    P.pos(2) += 0.25 * disp * distance3d(p0.pos, p1.pos);
}

void
VRAY_DemoMountain::fractalSplit()
{
    VRAY_DemoMountain	*kids[4];
    int			 i;
    fpreal		 scale;

    scale = 1.0 / (fpreal)mySplits;
    for (i = 0; i < 4; i++)
	kids[i] = new VRAY_DemoMountain(mySplits+1);

    for (i = 0; i < 3; i++)
    {
	kids[i]->myP[0] = myP[i];
	edgeSplit(kids[i]->myP[1], myP[i], myP[(i+1)%3], scale);
    }
    for (i = 0; i < 3; i++)
    {
	kids[3]->myP[i] = kids[(i+1)%3]->myP[2] = kids[i]->myP[1];
    }
    for (i = 0; i < 4; i++)
    {
	VRAY_ProceduralChildPtr	child = createChild();
	child->addProcedural(kids[i]);
    }
}

void
VRAY_DemoMountain::fractalRender()
{
    // Build some geometry
    VRAY_ProceduralGeo	 geo = createGeometry();
    GU_PrimPoly		*poly = GU_PrimPoly::build(geo.get(), 3);

#if 0
    // Optionally, create a primitive color attribute "Cd"
    uint seed = myP[0].seed + myP[1].seed + myP[2].seed;
    UT_Vector3 clr;
    clr.x() = SYSfastRandom(seed);
    clr.y() = SYSfastRandom(seed);
    clr.z() = SYSfastRandom(seed);
    GA_RWHandleV3 Cd(geo->addDiffuseAttribute(GA_ATTRIB_PRIMITIVE));
    Cd.set(poly->getMapOffset(), clr);
#endif

    for (int i = 0; i < 3; i++)
    {
        geo->setPos3(poly->getPointOffset(i),
                     myP[i].pos.x(), myP[i].pos.y(), myP[i].pos.z());
    }

    // Now, add the geometry to mantra.
    VRAY_ProceduralChildPtr	child = createChild();
    child->addGeometry(geo);
}
