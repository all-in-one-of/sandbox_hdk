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

#undef UT_ASSERT_LEVEL
#define UT_ASSERT_LEVEL	4

#include <UT/UT_DSOVersion.h>
#include <UT/UT_MTwister.h>
#include <UT/UT_StackBuffer.h>
#include <VRAY/VRAY_ProcGT.h>
#include <GT/GT_PrimitiveBuilder.h>
#include <GT/GT_PrimCurveMesh.h>
#include <GT/GT_DANumeric.h>
#include <GT/GT_DAConstantValue.h>
#include "VRAY_DemoGT.h"

using namespace HDK_Sample;

// Sample definition in an IFD
// ray_procedural -m -1 -.1 -1 -M 1 .1 1 demogt debug 0 count 10000 segments 2

static VRAY_ProceduralArg	theArgs[] = {
    VRAY_ProceduralArg("maxradius",	"real",		"0.01"),
    VRAY_ProceduralArg("count",		"int",		"100"),
    VRAY_ProceduralArg("segments",	"int",		"1"),
    VRAY_ProceduralArg("debug",		"int",		"0"),
    VRAY_ProceduralArg()
};

VRAY_Procedural *
allocProcedural(const char *)
{
    return new VRAY_DemoGT();
}

const VRAY_ProceduralArg *
getProceduralArgs(const char *)
{
    return theArgs;
}

VRAY_DemoGT::VRAY_DemoGT()
    : myMaxRadius(0.05)
    , myCurveCount(0)
    , mySegments(1)
    , myDebug(false)
{
    myBox.initBounds(0, 0, 0);
}

VRAY_DemoGT::~VRAY_DemoGT()
{
}

const char *
VRAY_DemoGT::className() const
{
    return "VRAY_DemoGT";
}

int
VRAY_DemoGT::initialize(const UT_BoundingBox *box)
{
    if (box)
	myBox = *box;
    else
	myBox = UT_BoundingBox(-1, -1, -1, 1, 1, 1);
    
    // Import the number of motion segments
    if (!import("segments", &mySegments, 1))
	mySegments = 1;

    if (!import("count", &myCurveCount, 1))
	myCurveCount = 100;

    if (!import("maxradius", &myMaxRadius, 1))
	myMaxRadius = 0.05;

    int		ival;
    if (import("debug", &ival, 1))
	myDebug = (ival != 0);

    if (myDebug)
	return true;
    // Only need to render if there's geometry to render
    return myCurveCount > 0 && mySegments > 0 && myMaxRadius > 0;
}

void
VRAY_DemoGT::getBoundingBox(UT_BoundingBox &box)
{
    box = myBox;

    // Now, expand bounds to include the maximum radius of a curve
    box.expandBounds(0, myMaxRadius);
}

static GT_PrimitiveHandle
makeCurveMesh(const UT_BoundingBox &box,
	int ncurves,
	fpreal maxradius,
	int segments)
{
    static const int			 pts_per_curve = 4;
    UT_MersenneTwister			 twist;
    GT_Real16Array			*f16;
    int					 npts = ncurves * pts_per_curve;

    // Allocate colors as uniform fpreal16 data
    GT_DataArrayHandle			 clr;
    f16 = new GT_Real16Array(ncurves, 3, GT_TYPE_COLOR);
    for (int i = 0; i < ncurves*3; ++i)
	f16->data()[i] = twist.frandom();

    clr.reset(f16);

    // Allocate widths as per-point fpreal16 data
    GT_DataArrayHandle			 widths;
    f16 = new GT_Real16Array(npts, 1);
    for (int curve = 0; curve < ncurves; curve++)
    {
	fpreal16	*val = f16->data() + pts_per_curve * curve;
	fpreal		 width = SYSlerp(maxradius, maxradius*.1,
					fpreal(twist.frandom())) * 2;
	for (int pt = 0; pt < pts_per_curve; ++pt)
	{
	    val[pt] = width;
	    width *= .7;	// Taper each curve
	}
    }
    widths.reset(f16);

    // Allocate "P" as per-point fpreal32 data
    UT_StackBuffer<GT_DataArrayHandle>	 P(segments);	// One for each segment
    UT_StackBuffer<GT_Real32Array *>	 Pdata(segments);
    UT_StackBuffer<fpreal>		 Py(pts_per_curve);
    for (int i = 0; i < segments; ++i)
    {
	Pdata[i] = new GT_Real32Array(npts, 3, GT_TYPE_POINT);
	P[i] = Pdata[i];
    }

    for (int i = 0; i < pts_per_curve; ++i)
    {
	fpreal	fity = fpreal(i)/(pts_per_curve-1);
	Py[i] = SYSlerp((fpreal)box.ymin(), (fpreal)box.ymax(), fity);
    }

    // Now, set positions
    for (int curve = 0; curve < ncurves; ++curve)
    {
	fpreal	tx = SYSlerp(.1, .9, twist.frandom());
	fpreal	tz = SYSlerp(.1, .9, twist.frandom());
	for (int seg = 0; seg < segments; ++seg)
	{
	    fpreal	px, pz;
	    fpreal	vx, vz;
	    px = SYSlerp(box.xmin(), box.xmax(), tx);
	    pz = SYSlerp(box.zmin(), box.zmax(), tz);

	    // Choose a different velocity trajectory per motion segment
	    vx = SYSlerp(0.0, box.xsize()*.1/segments, twist.frandom()-.5);
	    vz = SYSlerp(0.0, box.zsize()*.1/segments, twist.frandom()-.5);

	    // Choose a different height per motion segment
	    fpreal	height = SYSlerp(.5, 1.0, twist.frandom());
	    int	off = curve * pts_per_curve * 3;
	    for (int pt = 0; pt < pts_per_curve; ++pt)
	    {
		Pdata[seg]->data()[off+pt*3+0] = px;
		Pdata[seg]->data()[off+pt*3+1] = Py[pt] * height;
		Pdata[seg]->data()[off+pt*3+2] = pz;
		UT_ASSERT(px >= box.xmin() && px < box.xmax());
		UT_ASSERT(pz >= box.zmin() && pz < box.zmax());
		px += vx;
		pz += vz;
		vx *= .4;
		vz *= .4;
	    }
	}
    }
    // Debug the P values
    //P[0]->dumpValues("P");

    // Now we have all the data arrays, we can build the curve
    GT_AttributeMap	*amap;
    GT_AttributeList	*uniform;
    GT_AttributeList	*vertex;

    // First, create the uniform attributes
    amap = new GT_AttributeMap();
    amap->add("Cd", true);
    uniform = new GT_AttributeList(amap, segments);	// 2 motion segments
    for (int seg = 0; seg < segments; ++seg)
	uniform->set(0, clr, seg);	// Share color data for segment

    // Now, create the per-vertex attributes
    amap = new GT_AttributeMap();
    amap->add("P", true);
    amap->add("width", true);
    vertex = new GT_AttributeList(amap, segments);
    for (int seg = 0; seg < segments; ++seg)
    {
	vertex->set(0, P[seg], seg);	// Set P for the motion segment
	vertex->set(1, widths, seg);	// Share width data
    }

    // Create the vertex per curve counts
    GT_DataArrayHandle	vertex_counts;
    vertex_counts.reset(new GT_IntConstant(ncurves, pts_per_curve));

    // Create a new curve mesh
    GT_PrimitiveHandle prim(new GT_PrimCurveMesh(GT_BASIS_LINEAR,
				vertex_counts,
				GT_AttributeListHandle(vertex),
				GT_AttributeListHandle(uniform),
				GT_AttributeListHandle(),
				false));

    // Debug the curve primitive
    //prim->dumpPrimitive();
    return prim;
}

static GT_PrimitiveHandle
testBox(const UT_BoundingBox &box, fpreal32 maxwidth)
{
    GT_BuilderStatus	err;
    GT_PrimitiveHandle	prim;
    fpreal16		clr[3] = { .3, .5, 1 };
    prim = GT_PrimitiveBuilder::wireBox(err, box,
	    GT_VariadicAttributes()
		<< GT_Attribute("constant fpreal32 width", &maxwidth)
		<< GT_Attribute("constant color16 Cd", clr)
	);
    return prim;
}

void
VRAY_DemoGT::render()
{
    // To render this procedural, we create a new GT procedural as a child
    VRAY_ProceduralChildPtr	obj = createChild();

    GT_PrimitiveHandle	prim;
    if (myDebug)
	prim = testBox(myBox, myMaxRadius*2);
    else
    {
	int				one = 1;
	prim = makeCurveMesh(myBox, myCurveCount, myMaxRadius, mySegments);

	// Turn on subdivision curve rendering
	obj->changeSetting("object:rendersubdcurves", 1, &one);
    }
    int	zero;
    obj->changeSetting("geometry:computeN", 1, &zero);

    // Add the child procedural
    obj->addProcedural(new VRAY_ProcGT(prim));
}
