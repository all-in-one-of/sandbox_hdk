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
 * Sample multi-input wipe COP
 *    Produces an overexposure/bloom dip-to-white transition effect between two
 *    sequences using crossfader controls.
 */
#include <UT/UT_DSOVersion.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>

#include <SYS/SYS_Floor.h>
#include <SYS/SYS_Math.h>

#include <TIL/TIL_Plane.h>
#include <TIL/TIL_Region.h>
#include <TIL/TIL_Tile.h>
#include <COP2/COP2_CookAreaInfo.h>
#include "COP2_MultiInputWipe.h"

using namespace HDK_Sample;

COP_MULTI_SWITCHER(5, "Blend");

static PRM_Name names[] =
{
    PRM_Name("fadera",		"A Fader"),
    PRM_Name("faderb",		"B Fader"),
    PRM_Name("boostval",	"Overexposure Boost"),
    PRM_Name("bloomblur",	"Bloom Blur"),
    PRM_Name("fademode",	"Fade Rate"),
};

#define FADE_LINEAR 0
#define FADE_SQUARE 1
#define FADE_ROOT   2
static PRM_Name fadeItems[] =
{
    PRM_Name("linear",	"Linear"),
    PRM_Name("squared",	"Squared"),
    PRM_Name("root",	"Square Root"),
    PRM_Name(0),
};
static PRM_ChoiceList	fadeMenu((PRM_ChoiceListType)
		(PRM_CHOICELIST_EXCLUSIVE | PRM_CHOICELIST_REPLACE), fadeItems);


static PRM_Range wipeRange(PRM_RANGE_RESTRICTED, 0.0f, 
			   PRM_RANGE_RESTRICTED, 1.0f);
static PRM_Range boostRange(PRM_RANGE_UI, 0.0f, PRM_RANGE_UI, 1.0f);
static PRM_Range blurRange(PRM_RANGE_RESTRICTED, 0.0f, PRM_RANGE_UI, 20.0f);
static PRM_Default boostValue(0.9);
static PRM_Default boostBlur(9);

PRM_Template
COP2_MultiInputWipe::myTemplateList[] =
{
    PRM_Template(PRM_SWITCHER,	2, &PRMswitcherName, switcher),
    
    // comp page.
    PRM_Template(PRM_FLT,	TOOL_PARM, 1, &names[0], PRMoneDefaults, 0,
				&wipeRange),
    PRM_Template(PRM_FLT,	TOOL_PARM, 1, &names[1], PRMzeroDefaults, 0,
				&wipeRange),
    PRM_Template(PRM_FLT,	TOOL_PARM, 1, &names[2], &boostValue, 0,
				&boostRange),
    PRM_Template(PRM_FLT,	TOOL_PARM, 1, &names[3], &boostBlur, 0,
				&blurRange),
    PRM_Template(PRM_ORD,	POPUP_PARM, 1, &names[4], PRMoneDefaults, 
				&fadeMenu),
    PRM_Template(),
};

OP_TemplatePair COP2_MultiInputWipe::myTemplatePair( 
	COP2_MultiInputWipe::myTemplateList,
	&COP2_MultiBase::myTemplatePair );

OP_VariablePair COP2_MultiInputWipe::myVariablePair( 0,
	&COP2_Node::myVariablePair );

const char *	COP2_MultiInputWipe::myInputLabels[] =
{
    "Wipe A",
    "Wipe B",
    0
};
  
OP_Node *
COP2_MultiInputWipe::myConstructor(	OP_Network	*net,
					const char	*name,
					OP_Operator	*op)
{
    return new COP2_MultiInputWipe(net, name, op);
}

COP2_MultiInputWipe::COP2_MultiInputWipe(OP_Network *parent,
			       const char *name,
			       OP_Operator *entry)
    : COP2_MultiBase(parent, name, entry)
{
}

COP2_MultiInputWipe::~COP2_MultiInputWipe()
{
}

namespace HDK_Sample {

class cop2_MultiInputWipeData : public COP2_ContextData
{
public:
		 cop2_MultiInputWipeData() {}
    virtual	~cop2_MultiInputWipeData() {}

    // Stashed parameters and data 
    float	myFaderA;
    float	myFaderB;
    float	myBoostA;
    float	myBoostB;
    int		myBlurRadA;
    int		myBlurRadB;
    float	myBlurA;
    float	myBlurB;
    bool	myPassA;
    bool	myPassB;
};

}

COP2_ContextData *
COP2_MultiInputWipe::newContextData(const TIL_Plane *, int , float t,
				    int xres, int , int , int )
{
    cop2_MultiInputWipeData *data = new cop2_MultiInputWipeData();
    int fademode;
    float blur;
    float boost;

    // Compute the cross fader values and stash them in the context data
    data->myFaderA = evalFloat("fadera",0,t);
    data->myFaderB = evalFloat("faderb",0,t);

    fademode = evalInt("fademode", 0, t);

    if(fademode == FADE_SQUARE)
    {
	data->myFaderA *= data->myFaderA;
	data->myFaderB *= data->myFaderB;
    }
    else if(fademode == FADE_ROOT)
    {
	data->myFaderA = SYSsqrt(data->myFaderA);
	data->myFaderB = SYSsqrt(data->myFaderB);
    }

    // See if we can pass through one input or the other, without modification.
    data->myPassA = (data->myFaderA == 1.0f && data->myFaderB == 0.0f);
    data->myPassB = (data->myFaderB == 1.0f && data->myFaderA == 0.0f);

    // determine how much of a boost each image will be getting.
    boost = evalFloat("boostval", 0, t) * 0.5f;
    data->myBoostA = data->myFaderB * boost;
    data->myBoostB = data->myFaderA * boost;

    // Blur needs to be adjusted if we aren't cooking at full res.
    blur = evalFloat("bloomblur", 0, t) * getXScaleFactor(xres);
    
    // Set up how much each input will be blurred
    data->myBlurA = data->myFaderB * blur;
    data->myBlurB = data->myFaderA * blur;
    data->myBlurRadA = (int)SYSceil(data->myBlurA * 0.5f);
    data->myBlurRadB = (int)SYSceil(data->myBlurB * 0.5f);

    return data;
}


void
COP2_MultiInputWipe::computeImageBounds(COP2_Context &context)
{
    cop2_MultiInputWipeData *data =
	static_cast<cop2_MultiInputWipeData *>(context.data());
    bool init = false;
    int x1,y1,x2,y2;
    int ix1, ix2, iy1, iy2;

    x1 = 0;
    y1 = 0;
    x2 = context.myXres-1;
    y2 = context.myYres-1;

    // The image bounds are a union of all input bounds.
    for(int i=0; i<nInputs(); i++)
    {
	if(getInputBounds(i, context, ix1, iy1, ix2, iy2))
	{
	    if(!init)
	    {
		x1 = ix1;
		y1 = iy1;
		x2 = ix2;
		y2 = iy2;
		init = true;
	    }
	    else
	    {
		if(ix1 < x1) x1 = ix1;
		if(ix2 > x2) x2 = ix2;
		if(iy1 < y1) y1 = iy1;
		if(iy2 > y2) y2 = iy2;
	    }
	}
    }

    // Finally, we need to expand the bounds by the maximum blur radius.
    if(!data->myPassA && !data->myPassB)
    {
	int brad = SYSmax(data->myBlurRadA, data->myBlurRadB);
	x1 -= brad;
	y1 -= brad;
	x2 += brad;
	y2 += brad;
    }

    context.setImageBounds(x1,y1,x2,y2);
}

void
COP2_MultiInputWipe::getInputDependenciesForOutputArea(
			    COP2_CookAreaInfo &output_area,
			    const COP2_CookAreaList &input_areas,
			    COP2_CookAreaList &needed_areas)
{
    cop2_MultiInputWipeData *cdata;
    COP2_Context	*context;
    COP2_CookAreaInfo	*area;

    // If we're bypassed then we don't depend on any of the other inputs.
    if (getBypass())
    {
	area = makeOutputAreaDependOnMyPlane(0, output_area, input_areas,
					     needed_areas);
	return;
    }

    context = output_area.getNodeContextData();
    cdata = static_cast<cop2_MultiInputWipeData *>(context->data());

    // Add a dependency on the first wipe input.
    if(!cdata->myPassB)
    {
	area = makeOutputAreaDependOnMyPlane(0, output_area, input_areas,
					     needed_areas);
	// The area needs to be expanded by the blur radius for input A
	if(area)
	    area->expandNeededArea( cdata->myBlurRadA, cdata->myBlurRadA,
				    cdata->myBlurRadA, cdata->myBlurRadA);
    }

    // Add a dependency on the second wipe input, and expand its area as well.
    if(!cdata->myPassA)
    {
	area = makeOutputAreaDependOnMyPlane(1, output_area, input_areas,
					     needed_areas);
	if(area)
	    area->expandNeededArea( cdata->myBlurRadB, cdata->myBlurRadB,
				    cdata->myBlurRadB, cdata->myBlurRadB);
    }
}

int
COP2_MultiInputWipe::passThrough(COP2_Context &context,
				 const TIL_Plane *plane, int,
				 int, float t,
				 int xstart, int ystart)
{
    cop2_MultiInputWipeData *data =
	static_cast<cop2_MultiInputWipeData *>(context.data());
    const TIL_Sequence *inputseq = 0;

    // If one of the faders is at 1 and the other at 0, we might be able to
    // pass one of the images through as-is.
    if(data->myPassA)
	inputseq = inputInfo(0);
    else if(data->myPassB)
	inputseq = inputInfo(1);

    if(inputseq)
    {
	// Now check if it's possible to pass the tile through as-is, based on
	// whether the input tile and output tile match exactly.
	const TIL_Plane *inputplane = inputseq->getPlane(plane->getName());
	if(inputplane)
	{
	    int xres,yres;
	    int ixres, iyres;

	    mySequence.getRes(xres,yres);
	    inputseq->getRes(ixres,iyres);

	    if(plane->isCompatible(*inputplane) &&	// planes match
	       ixres == xres && iyres == yres &&	// resolutions match
	       inputseq->getImageIndex(t) != -1 &&	// within frame range
	       isTileAlignedWithInput(0,context, xstart,ystart)) // tiles match
	    {
		return 1;
	    }
	}
    }

    // Otherwise, cook.
    return 0;
}

void
COP2_MultiInputWipe::passThroughTiles(COP2_Context &context,
				      const TIL_Plane *plane, int array_index,
				      float t, int xstart, int ystart,
				      TIL_TileList *&tiles,
				      int block, bool *mask, bool *blocked)
{
    // This method is only called if passThrough() returns non-zero.
    cop2_MultiInputWipeData *data =
	static_cast<cop2_MultiInputWipeData *>(context.data());
    bool iblocked = false;

    // If we're not crossfading, just return one of the images as-is.
    if(data->myPassA)
    {
	tiles = passInputTile(0,context, plane, array_index, t, xstart,
			      ystart, block, &iblocked, mask);
    }
    else if(data->myPassB)
    {
	tiles = passInputTile(1, context, plane, array_index, t, xstart,
			      ystart, block, &iblocked, mask);

    }

    // In the rare case that a tile could not be accessed, check if it was
    // blocked and set our blocked flag, if passed in.
    if(!tiles && iblocked && blocked)
	*blocked = true;
}

OP_ERROR
COP2_MultiInputWipe::cookMyTile(COP2_Context &context, TIL_TileList *tilelist)
{
    cop2_MultiInputWipeData *data =
	static_cast<cop2_MultiInputWipeData *>(context.data());
    TIL_Region		*aregion, *bregion;
    int			 arad, brad;
    TIL_Plane		 fpplane(*context.myPlane);
    bool		 init = false;

    arad = data->myBlurRadA;
    brad = data->myBlurRadB;
 
    // Request 32b FP data from the inputs
    fpplane.setScoped(1);
    fpplane.setFormat(TILE_FLOAT32);

    // Grab a region from input 1, run the operation on it, and release it.
    if(data->myFaderA != 0.0f)
    {
	aregion = inputRegion(0, context, &fpplane, 0, context.getTime(),
			      tilelist->myX1 - arad,
			      tilelist->myY1 - arad,
			      tilelist->myX2 + arad,
			      tilelist->myY2 + arad, TIL_HOLD);
	if(aregion)
	{
	    boostAndBlur(tilelist, aregion, data->myFaderA,
			 data->myBoostA, arad, data->myBlurA, false);
	    init = true;
	    releaseRegion(aregion);
	}
    }

    // Repeat for input 2.
    if(data->myFaderB != 0.0f)
    {
	bregion = inputRegion(1, context, &fpplane, 0, context.getTime(),
			      tilelist->myX1 - brad,
			      tilelist->myY1 - brad,
			      tilelist->myX2 + brad,
			      tilelist->myY2 + brad, TIL_HOLD);
	if(bregion)
	{
	    boostAndBlur(tilelist, bregion, data->myFaderB,
			 data->myBoostB, brad, data->myBlurB, init);
	    init = true;
	    releaseRegion(bregion);
	}
    }

    // If neither input was processed (A=0, B=0), clear the tiles to constant
    // black.
    if(!init)
	tilelist->clearToBlack();
    
    return error();
}

void
COP2_MultiInputWipe::boostAndBlur(TIL_TileList *tiles, TIL_Region *input,
				float fade, float boost, int rad, float blur,
				bool add)
{
    int ti, x,y, i,j, idx;
    int w,h;
    int stride;
    TIL_Tile *itr;
    float *src, *scan;
    float vedge;
    float sum, hsum;
    float *dest = NULL;
    bool alloced = false;

    // Set up some constants for the blurring
    const float iblur = fade / ((1.0f + blur) * (1.0f + blur));
    const float edge = 1.0f - (rad - blur * 0.5f);

    // set up tile dimensions and the stride of the input region.
    w = tiles->myX2 - tiles->myX1 + 1;
    h = tiles->myY2 - tiles->myY1 + 1;

    stride = w + rad * 2;

    // Boost the input region. Unlike input tiles, you can modify data in
    // input regions, as long you have not explicitly requested a shared region.
    if(boost != 0.0f)
    {
	for(i=0; i<PLANE_MAX_VECTOR_SIZE; i++)
	{
	    src = (float *) input->getImageData(i);
	    if(src)
		for(y=0; y<(h+rad*2) * stride; y++)
		    *src++ += boost;
	}
    }

    // When assigning values on the first pass, it is not necessary to fetch the
    // image data from the tile. We won't be using it and it'll be overwritten.
    // So, the dest array must be allocated, as getTileInFP() won't be called
    // to allocate it for us.
    if(!add)
    {
	dest = new float[w*h];
	alloced = true;
    }

    // Iterate over each component in the tilelist.
    FOR_EACH_UNCOOKED_TILE(tiles, itr, ti)
    {
	// Grab the image data from the region. It is FP32, as requested in
	// cookMyTile().
	src = ((float *) input->getImageData(ti)) + rad;

	if(add)
	{
	    // Grab each tile in FP format. 'dest' may be allocated; if so, we
	    // need to free it, but it can be reused by getTileInFP for the next
	    // component.
	    if(getTileInFP(tiles, dest, ti))
		alloced = true;
	}
	else
	{
	    // It is being assigned, ignoring the previous tile data, so just
	    // zero out the array.
	    memset(dest, 0, sizeof(float)*w*h);
	}

	// Process each pixel in the tile.
	for(idx=0, y=0; y<h; y++)
	{
	    for(x=0; x<w; x++, idx++)
	    {
		sum = 0.0f;
		
		// for each pixel, blur the surroundings
		scan = src+x;
		for(i=-rad; i<=rad; i++)
		{
		    vedge = (i == -rad || i == rad) ? edge : 1.0f;

		    hsum  = scan[-rad] * edge;
		    if(rad)
			hsum += scan[rad] * edge;
		    
		    for(j=-rad+1; j<rad; j++)
			hsum += scan[j];

		    sum += hsum * vedge;
		    scan += stride;
		}

		// Assign explicitly or add to the previous result, depending
		// on the pass.
		dest[idx] += sum * iblur;
	    }
	    
	    src += stride;
	}

	// Write the FP result to the tile.
	writeFPtoTile(tiles, dest, ti);
    }

    // If dest was allocated by getTileInFP() or by this method, free it. 
    // (If getTileInFP() returned false,  it is likely that the tile was 
    // already a FP32 tile, and freeing dest would crash as this is a direct 
    // pointer to the tile data.)
    if(alloced)
	delete [] dest;
}

void
newCop2Operator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator("hdk_multiwipe",
				       "HDK Multi Input Wipe",
				       COP2_MultiInputWipe::myConstructor,
				       &COP2_MultiInputWipe::myTemplatePair,
				       2, 
				       2,
				       &COP2_MultiInputWipe::myVariablePair,
				       0, // not generator
				       COP2_MultiInputWipe::myInputLabels));
}
