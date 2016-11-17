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
 */

#include "SHOP_POVMaterial.h"
#include <UT/UT_DSOVersion.h>
#include <OP/OP_OperatorTable.h>
#include <SHOP/SHOP_Operator.h>
#include <PRM/PRM_Include.h>

using namespace HDK_Sample;

static PRM_Name	pigmentName("pigment", "Pigment");

// Obviously, there could be more sophisticated controls, but this should be
// sufficient for the example.
static PRM_Template	theTemplates[] = {
    PRM_Template(PRM_RGB, 3, &pigmentName, PRMoneDefaults),
    PRM_Template()	// Terminator
};

// When we construct the base class SHOP_Node we indicate that this represents
// a "surface" shader.
SHOP_POVMaterial::SHOP_POVMaterial(OP_Network *parent,
	const char *name, OP_Operator *entry)
    : SHOP_Node(parent, name, entry, SHOP_SURFACE)
{
}

SHOP_POVMaterial::~SHOP_POVMaterial()
{
}

void
SHOP_POVMaterial::pigment(UT_Vector3 &clr, fpreal time)
{
    // Evaluate the pigment parameter
    for (int i = 0; i < 3; ++i)
	clr(i) = evalFloat("pigment", i, time);
}

bool
SHOP_POVMaterial::buildShaderString(UT_String &result,
	fpreal now, const UT_Options *options,
	OP_Node *obj, OP_Node *sop, SHOP_TYPE interpret_type)
{
    UT_String		renderer;
    UT_WorkBuffer	tmp;
    UT_Vector3		clr;

    if (!options || !options->importOption(SHOP_RENDERTYPE_OPTION, renderer))
	renderer = "POV";

    // Check that we're being asked to render for POV
    // To test this, in an hscript textport (i.e. hbatch), run:
    //   echo `shopstring("/shop/pov1", "POV")`
    //   echo `shopstring("/shop/pov1", "VMantra")`
    //   echo `shopstring("/shop/pov1", "RIB")`
    if (renderer == "POV")
    {
	pigment(clr, now);
	tmp.sprintf("texture { pigment { color rgb <%g, %g, %g> } }",
		clr(0), clr(1), clr(2));
	
    }
    else if (renderer == "VMantra")
    {
	tmp.sprintf("plastic diff ( %g %g %g )", clr(0), clr(1), clr(2));
    }
	tmp.stealIntoString(result);

    return result.isstring();
}

bool
SHOP_POVMaterial::buildShaderData(SHOP_ReData &data, fpreal now,
	const UT_Options *options,
	OP_Node *obj, OP_Node *sop, SHOP_TYPE interpret_type)
{
    // Some renderers (OpenGL) require non-string data, so, just let the base
    // class evaluate for other renderers.
    return SHOP_Node::buildShaderData(data, now, options,
	    obj, sop, interpret_type);
}

OP_ERROR
SHOP_POVMaterial::cookMe(OP_Context &context)
{
    // Evaluate parameters to check for errors
    UT_Vector3	clr;
    pigment(clr, context.getTime());
    return SHOP_Node::cookMe(context);
}

static OP_Node *
theConstructor(OP_Network *net, const char *name, OP_Operator *entry)
{
    return new SHOP_POVMaterial(net, name, entry);
}

void
newShopOperator(OP_OperatorTable *table)
{
    SHOP_Operator	*shop;
    SHOP_OperatorInfo	*info;

    shop = new SHOP_Operator("pov", "POV Material",
	    theConstructor,
	    theTemplates,
	    0, 0,
	    SHOP_Node::myVariableList,
	    OP_FLAG_GENERATOR,
	    SHOP_AUTOADD_NONE);
    shop->setIconName("SHOP_material");

    info = UTverify_cast<SHOP_OperatorInfo *>(shop->getOpSpecificData());
    info->setShaderType(SHOP_SURFACE);

    // Only support the "OGL" and the "POV" renderers.  Add VMantra so the
    // material will show up in Houdini by default.  For full customization,
    // you may need to customize $HH/Renderers etc.
    //
    // As an alternative, you could set the rendermask to "*" and try to
    // support *all* renderers.
    info->setRenderMask("OGL POV VMantra");

    table->addOperator(shop);
}
