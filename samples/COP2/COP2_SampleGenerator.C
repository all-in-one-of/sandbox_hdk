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
 * Constant SampleGenerator COP
 */
#include <UT/UT_DSOVersion.h>

#include <SYS/SYS_Math.h>
#include <UT/UT_SysClone.h>

#include <OP/OP_OperatorTable.h>

#include <PRM/PRM_Include.h>
#include <PRM/PRM_Parm.h>

#include <TIL/TIL_Plane.h>
#include <TIL/TIL_Tile.h>
#include "COP2_SampleGenerator.h"

using namespace HDK_Sample;

COP_GENERATOR_SWITCHER(2, "HDK Sample Generator");

static PRM_Name names[] =
{
    PRM_Name("seed",		"Seed"),
    PRM_Name("ampl",		"Amplitude"),
};

PRM_Template
COP2_SampleGenerator::myTemplateList[] =
{
    PRM_Template(PRM_SWITCHER,	3, &PRMswitcherName, switcher),

    PRM_Template(PRM_INT_J,	TOOL_PARM, 1, &names[0], PRMoneDefaults),
    PRM_Template(PRM_RGB_J,	TOOL_PARM, 4, &names[1], PRMoneDefaults),
		 
    PRM_Template(),
};

OP_TemplatePair COP2_SampleGenerator::myTemplatePair(
    COP2_SampleGenerator::myTemplateList,
    &COP2_Generator::myTemplatePair );

OP_VariablePair COP2_SampleGenerator::myVariablePair(0,
    &COP2_Node::myVariablePair );


OP_Node *
COP2_SampleGenerator::myConstructor(	OP_Network	*net,
					const char	*name,
					OP_Operator	*op)
{
    return new COP2_SampleGenerator(net, name, op);
}

COP2_SampleGenerator::COP2_SampleGenerator(OP_Network *parent,
			       const char *name,
			       OP_Operator *entry)
    : COP2_Generator(parent, name, entry)
{
}

COP2_SampleGenerator::~COP2_SampleGenerator()
{
}

// -----------------------------------------------------------------------

/// This method computes the resolution, frame range and planes & formats
/// used during the cook. All of these parms are non-animatable. It is called
/// first during a cook, or when the information is requested from a node.
/// Just because cookSequenceInfo is called, it does not mean that a cook is
/// about to start.
TIL_Sequence *
COP2_SampleGenerator::cookSequenceInfo(OP_ERROR & error)
{
    // If you want the controls in the Image/Sequence pages to
    // affect planes, format, res, range, use this parent class method.
    // You can always override them after the call.
    COP2_Generator::cookSequenceInfo(error);

    //// ***** Other examples ***********
    //// Manual setup of sequences
     
    //// 1. Clear out any previous values.
    // mySequence.reset();
    
    //// (optional) To copy an input's sequence information:
    // const TIL_Sequence *seq = inputInfo ( 0 ); // input #
    // if(seq)
    //     mySequence = *seq;
    // else
    //     { error = UT_ERROR_ABORT; return &mySequence; } // should pop an err
    
    //// 2. Set up frame range ****************

    //// Do you want a single image only? ie, time invariant.
    // mySequence.setSingleImage(1);
    
    //// otherwise, animations look like (sample values only):
    // mySequence.setSingleImage(0);
    // mySequence.setStart(1);
    // mySequence.setLength(300);
    // mySequence.setFrameRate(30);

    
    //// 3. Set up the Resolution **************

    //// Fairly straightforward - X Y dimensions. Must be at least 1. Don't
    //// worry about reduced cooking res's - they are handled automatically.
    // mySequence.setRes(500, 400);
    // mySequence.setAspectRatio(1.0f); // x/y (ie, 2: pixel is twice as wide as
				        //      it is tall)

    //// 4. Planes & Data formats ****************************
    //// Set up the actual planes we'll use. We can create up to 4000, but each
    //// must have a unique plane name.

    //// if you'd like to call COP2_Generator to setup the res, range, etc.,
    //// but want complete control over the planes yourself, call:
    
    // mySequence.clearAllPlanes();
    
    //// which will remove any planes added by COP2_Generator
    

    //// To add planes, use:
    
    // mySequence.addPlane("name_of_plane", TILE_[format], "name_of_comp",...)
    
    //// plane names recognized by Halo:
    ////		getColorPlaneName()
    ////		getAlphaPlaneName()
    ////		getMaskPlaneName()
    ////		getDepthPlaneName()
    ////		getLumPlaneName()
    ////		getBumpPlaneName()
    ////		getPointPlaneName()
    ////		getNormalPlaneName()
    ////		getVelocityPlaneName()
    //// Other names can be used as well, but other Nodes won't recognize it
    //// as anything special.

    //// format can be TILE_INT8, TILE_INT16, TILE_INT32, TILE_FLOAT32.
    //// To use black/white points for int formats, call setBlackWhitePoints()
    //// on the returned plane from addPlane(). FP does not support B/W points.

    //// components - can be anything descriptive (r,g,b, x,y,z,  u,v).
    //// Halo doesn't care about what the component are names at all.
    //// The number of strings defines the vector size of the plane (up to 4).
    //// if no components are specified, the plane is assumed to be Scalar.

    //TIL_Plane *plane;

    //// FP color plane, vector size 3 with components named r,g,b.
    //plane = mySequence.addPlane( getColorPlaneName(), TILE_FLOAT32,
    //				   "r","g","b");

    //// 8 bit alpha plane, scalar.
    //plane = mySequence.addPlane( getAlphaPlaneName(), TILE_INT8 );
    // plane->setBlackWhitePoints(16, 240); // for 8 bit with a bit of
					    // head/foot room, for example

    //// Some other plane examples:
    
    // plane = mySequence.addPlane( getBumpPlaneName(), TILE_INT16, "u","v");
    // plane->setBlackWhitePoints(32768, 65535); // dynamic range ~= -1 to 1
    
    // mySequence.addPlane( getDepthPlaneName(), TILE_FLOAT32);
    
    // mySequence.addPlane( getPointPlaneName(), TILE_FLOAT32, "x","y","z");

    //// ****** End examples *******

    // done.
    return &mySequence;
}


// ** newContextData()
// This method is used to create a custom context for your node. If you need
// to evaluate animated parms, do it here and stash the values in your context
// data. All context data classes must derive from COP2_ContextData (see .h
// file for class def)

// If you have all static parms, like menus or toggles, you can evaluate them
// in cookSequenceInfo() instead, if you like.

// The context data is also useful for holding scratch data arrays.

// By default, a context is created for each different time and each different
// resolution. You can also cause it to create them if the plane differs, or
// the thread differs (if a plane or res variable is used in an expression, it
// will automatically create a new context data for you if plane or res
// changes). To controls the creation of contexts, override the virtuals:
//
//    virtual bool createPerPlane() const	{ return false; }
//    virtual bool createPerRes() const		{ return true; }
//    virtual bool createPerTime() const	{ return true; }
//    virtual bool createPerThread() const	{ return false; }

// in your custom COP2_ContextData class.

COP2_ContextData *
COP2_SampleGenerator::newContextData(const TIL_Plane * /*planename*/,
				     int /*arrayindex*/,
				     float t, int /*xsize*/, int /*ysize*/,
				     int /*thread*/, int /*max_num_threads*/)
{
    // create one of my context data objects.
    cop2_SampleGeneratorData *data = new cop2_SampleGeneratorData;

    // stashing some parm values for the cook in my context data.
    AMP(data->myAmp, t);
    data->mySeed = SEED(t);
    
    return data;
}

// ** generateTile()
// This method cooks a tile of the full image. A tile is a rectangular region
// of the image for a specific component of a specific plane. This means that
// all vectors' data is stored non-interleaved, one component per tile. 

// Before proceeding with your cook, you may need to check the:
//   - plane you are being requested to cook (context.myPlane)
//   - the resolution you are being cooked at (context.myXres, context.myYres)
//   - the time you are being cooked at (context.getTime())
//   - the area the tile occupies (tiles->myX1, tiles->myY1) -
//			          (tiles->myX2, tiles->myY2)

// You may not be asked to cook all the tiles in the image, and you may not
// be asked to cook all the planes. Cooking is done on an on-demand basis.
// The order you are asked to cook tiles in is undefined.

// The TIL_TileList passed to you contains 1-4 tiles, depending on the # of
// components in that plane. Some of these components may already be cached,
// so use FOR_EACH_UNCOOKED_TILE(tiles, itr) to iterate through only dirty
// tiles or TILE_VECTOR4(tiles, t1,t2,t3,t4) to assign the dirty tiles to
// TIL_Tile *t1,*t2,*t3,*t4. Non-dirty or non-existant tiles will be assigned
// NULL to the TIL_Tile ptr.

// Tiles are not pre-cleared for you. You must write to each pixel in the tile.

// THREAD WARNING: This method is called simultaneously in different threads.
// Don't do un-threadsafe stuff here, like writing to static buffers.

OP_ERROR
COP2_SampleGenerator::generateTile(COP2_Context &context, TIL_TileList *tiles)
{
    // retrieve my context data created for this plane/res/time/thread
    // by my newContextData method.
    cop2_SampleGeneratorData *data =
	static_cast<cop2_SampleGeneratorData *>( context.data() );

    int i, ti;
    TIL_Tile * itr;
    unsigned seed;
    const float *amp = data->myAmp.data();

    // alloc some temp space for our values. If we know that we're always using
    // FP, we could just write the values into the tiles directly using
    // dest = (float *) itr->getImageData();
    float *dest = new float[tiles->mySize];
    
    FOR_EACH_UNCOOKED_TILE(tiles, itr, ti)
    {
	// initial seed value based on lower left corner position & tile index.
	seed = ((unsigned) data->mySeed * 4 + ti)
		* (context.myXres * context.myYres) +
	    tiles->myX1 + tiles->myY1 * context.myXres;

	// tiles->mySize is the # of pixels in the tile. 
	for(i=0; i<tiles->mySize; i++)
	    dest[i] = SYSrandom(seed) * amp[ti];

	// write the values back to the tile using a convenience method.
	// not necessary if we used dest = (float *) itr->getImageData() above.
	writeFPtoTile(tiles, dest, ti);
    }

    delete dest;
    
    return error();
}


/// install the cop.
void newCop2Operator(OP_OperatorTable *table)
{
    // This operator flags itself as a generator taking zero or one inputs.
    // The optional input is the mask. 
    table->addOperator(new OP_Operator("hdk_samplegen", // node name
				       "HDK Sample Generator", // pretty name
				       COP2_SampleGenerator::myConstructor,
				       &COP2_SampleGenerator::myTemplatePair,
				       0,       // min inputs
				       1,       // max inputs 
				       &COP2_SampleGenerator::myVariablePair,
				       OP_FLAG_GENERATOR));
}
