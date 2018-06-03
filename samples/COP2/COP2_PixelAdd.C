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
 * Addness Op.
 */
#include <UT/UT_DSOVersion.h>
#include <UT/UT_WorkBuffer.h>
#include <CH/CH_Manager.h>
#include <OP/OP_Context.h>
#include <OP/OP_OperatorTable.h>

#include <PRM/PRM_Include.h>
#include <PRM/PRM_Parm.h>

#include <RU/RU_PixelFunctions.h>

#include "COP2_PixelAdd.h"

using namespace HDK_Sample;

COP_PIXEL_OP_SWITCHER(1, "Sample Pixel Add");

static PRM_Name names[] =
{
    PRM_Name("addend",	"Add Value"),
};


static PRM_Range  addRange(PRM_RANGE_UI, -1, PRM_RANGE_UI, 1);
PRM_Template
COP2_PixelAdd::myTemplateList[] =
{
    PRM_Template(PRM_SWITCHER,	3, &PRMswitcherName, switcher),

    PRM_Template(PRM_RGB_J,	TOOL_PARM, 4, &names[0], PRMoneDefaults,0,
		 &addRange),
    
    PRM_Template(),
};

OP_TemplatePair COP2_PixelAdd::myTemplatePair( COP2_PixelAdd::myTemplateList,
					     &COP2_PixelOp::myTemplatePair );

OP_VariablePair COP2_PixelAdd::myVariablePair( 0,	&COP2_MaskOp::myVariablePair);

const char *	COP2_PixelAdd::myInputLabels[] =
{
    "Image to Add to",
    "Mask Input",
    0
};
     
OP_Node *
COP2_PixelAdd::myConstructor(OP_Network	*net,
			   const char	*name,
			   OP_Operator	*op)
{
    return new COP2_PixelAdd(net, name, op);
}

COP2_PixelAdd::COP2_PixelAdd(OP_Network *parent,
			 const char *name,
			 OP_Operator *entry)
    : COP2_PixelOp(parent, name, entry)
{
}

COP2_PixelAdd::~COP2_PixelAdd() { }



// this is the pixel function, which does the work.
namespace HDK_Sample {
class cop2_AddFunc : public RU_PixelFunction
{ 
public:
			 cop2_AddFunc(float r,float g, float b, float a) 
	{ myAddend[0] = r; myAddend[1] = g; myAddend[2] = b; myAddend[3] = a; }
protected:

    // the operation differs per component.
    virtual bool	 eachComponentDifferent() const { return true; }

    // we don't need to process all the compoents together, as a vector.
    virtual bool	 needAllComponents() const { return false; }

    
    // Here's the heart of the pixel function - it simply adds our addend to
    // the passed in val for the given component. This is the scalar version.
    static float	 add(RU_PixelFunction *pf, float val, int comp)
		 	 { return val +((cop2_AddFunc*)pf)->myAddend[comp]; }

    // we return the static function above as our scalar function.
    virtual RUPixelFunc  getPixelFunction() const { return add; }

    // --or-- 

    // These are the methods you would use for a vector function. They are
    // not used in this case, since needAllComponents is false.
    // You can define a function with both a scalar and a vector function,
    // and switch between the two based on parameters.
    
    static void			addvec(RU_PixelFunction *f, float **vals,
				       const bool *scope)
	{
	    cop2_AddFunc *pf = (cop2_AddFunc *) f;
	    
	    if(vals[0] && scope[0]) *vals[0] = *vals[0] + pf->myAddend[0];
	    if(vals[1] && scope[1]) *vals[1] = *vals[1] + pf->myAddend[1];
	    if(vals[2] && scope[2]) *vals[2] = *vals[2] + pf->myAddend[2];
	    if(vals[3] && scope[3]) *vals[3] = *vals[3] + pf->myAddend[3];
	}
    
    // we return the static function above as our vector function.
    virtual RUVectorFunc	getVectorFunction() const { return addvec; }
    
private:
    float	myAddend[4];
};
}



// This is where we create and return our a new pixel function, defined above.
// Parms can be evaluated in this method.

RU_PixelFunction *
COP2_PixelAdd::addPixelFunction(const TIL_Plane*plane,int,float t, int,int,int)
{
    // The frame scope effect is only used if parms on the frame scope page
    // are altered.
    int   index = mySequence.getImageIndex(t);
    float effect = getFrameScopeEffect(index);
    float r,g,b,a;

    // Note that we treat the alpha plane differently than other planes.
    if(plane->isAlphaPlane())
    {
	// use the alpha value for the alpha plane.
	r = g = b = a = ADD(3, t) * effect;
    }
    else
    {
	// all other planes, use comps 0-3.
	r = ADD(0,t) * effect;
	g = ADD(1,t) * effect;
	b = ADD(2,t) * effect;
	a = ADD(3,t) * effect;
    }

    return new cop2_AddFunc(r,g,b,a);
}

const char *
COP2_PixelAdd::getOperationInfo()
{
    // return a small string describing what this function does in the info
    // popup.
    fpreal t = CHgetEvalTime();
    int   index = mySequence.getImageIndex(t);
    float effect = getFrameScopeEffect(index);
    
    static UT_WorkBuffer info;

    info.sprintf("Add (%g, %g, %g, %g)",
		 ADD(0,t)*effect, ADD(1,t)*effect,
		 ADD(2,t)*effect, ADD(3,t)*effect);
    
    return info.buffer();
}

void
newCop2Operator(OP_OperatorTable *table)
{
     table->addOperator(new OP_Operator("hdk_samplepixadd",
					"HDK Sample Pixel Add",
					&COP2_PixelAdd::myConstructor,
					&COP2_PixelAdd::myTemplatePair,
					1,
					2,  // optional mask input
					&COP2_PixelAdd::myVariablePair));
}

