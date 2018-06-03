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
 */

#include "SHOP_Multi.h"
#include <UT/UT_DSOVersion.h>
#include <OP/OP_OperatorTable.h>
#include <SHOP/SHOP_Operator.h>
#include <PRM/PRM_Include.h>

using namespace HDK_Sample;

static PRM_Default surfaceDefault(0,
	"opdef:/Shop/principledshader?SurfaceVexCode");

#if 0
    static PRM_Default displaceDefault(0,
	    "opdef:/Shop/principledshader?DisplacementVexCode");
#else
    // With no string in the displacement shader, this material won't have a
    // displacement shader, and mantra will be able to render the geometry more
    // efficiently.
    //
    // The displacement bounds likely won't be output either.
    static PRM_Default displaceDefault(0, "");
#endif

// Provide a string that gets passed verbatim to mantra for the surface shader
static PRM_Name	surfaceShader("surface", "Surface Shader");
// Provide a string that gets passed to mantra for the displacement shader
static PRM_Name	displaceShader("displace", "Displacement Shader");

// Here, we create a displacement bound property.  This isn't part of the
// shaders, but instead is a rendering property.  Any of the known rendering
// properties can be added as parameters.  These parameters can be visible or
// invisible to users (see PRM_PRM_INTERFACE_INVISIBLE in PRM_Type.h)
static PRM_Name	vm_displacebound("vm_displacebound", "Displace Bounds");

// The parametes for the SHOP
static PRM_Template	theTemplates[] = {
    PRM_Template(PRM_STRING, 1, &surfaceShader, &surfaceDefault),
    PRM_Template(PRM_STRING, 1, &displaceShader, &displaceDefault),
    PRM_Template(PRM_FLT_J, 1, &vm_displacebound, PRMzeroDefaults),
    PRM_Template()	// Terminator
};

// When we construct the base class SHOP_Node we indicate that this represents
// a "material" shader (see the ::install() method for details)
SHOP_Multi::SHOP_Multi(OP_Network *parent,
	const char *name, OP_Operator *entry)
    : SHOP_Node(parent, name, entry, SHOP_MATERIAL)
{
}

SHOP_Multi::~SHOP_Multi()
{
}

bool
SHOP_Multi::buildShaderString(UT_String &result,
	fpreal now, const UT_Options *options,
	OP_Node *obj, OP_Node *sop, SHOP_TYPE interpret_type)
{
    switch (interpret_type)
    {
	case SHOP_SURFACE:
	    evalString(result, "surface", 0, now);
	    return true;
	case SHOP_DISPLACEMENT:
	    evalString(result, "displace", 0, now);
	    return true;
	default:
	    UT_ASSERT(0 && "This should never be hit");
	    break;
    }
    return false;
}

SHOP_Node *
SHOP_Multi::findShader(SHOP_TYPE type, fpreal now, const UT_Options *options)
{
    // There are three types of SHOPs we want to be able to provide:
    //	- Surface shader:  Evaluate a surface shader for mantra
    //	- Displacement shader:  Provide a displacement shader
    //	- Properties SHOP:  If there's a displacement shader, then we need to
    //	provide the displacement bounds property.  If you have other properties
    //	(i.e. render subdivision surfaces, shading quality etc., you will want
    //	to always return 'this' when the query is for SHOP_PROPERTIES.
    UT_String	tmp;
    switch (type)
    {
	case SHOP_SURFACE:
	    evalString(tmp, "surface", 0, now);
	    return tmp.isstring() ? this : nullptr;
	case SHOP_DISPLACEMENT:
	case SHOP_PROPERTIES:
	    evalString(tmp, "displace", 0, now);
	    return tmp.isstring() ? this : nullptr;
	default:
	    return nullptr;
    }
}

bool
SHOP_Multi::matchesShaderType(SHOP_TYPE type)
{
    return type == SHOP_SURFACE
	|| type == SHOP_DISPLACEMENT
	|| type == SHOP_PROPERTIES;
}

OP_ERROR
SHOP_Multi::cookMe(OP_Context &context)
{
    // Evaluate parameters to check for errors
    UT_String	tmp;
    fpreal	now = context.getTime();
    evalString(tmp, "surface", 0, now);
    evalString(tmp, "displace", 0, now);
    evalFloat("vm_displacebound", 0, now);
    return SHOP_Node::cookMe(context);
}

static OP_Node *
theConstructor(OP_Network *net, const char *name, OP_Operator *entry)
{
    return new SHOP_Multi(net, name, entry);
}

void
SHOP_Multi::install(OP_OperatorTable *table)
{
    SHOP_Operator	*shop;
    SHOP_OperatorInfo	*info;

    shop = new SHOP_Operator("multi", "HDK Multi",
	    theConstructor,
	    theTemplates,
	    nullptr,
	    0, 0,
	    SHOP_Node::myVariableList,
	    OP_FLAG_GENERATOR,
	    SHOP_AUTOADD_NONE);
    shop->setIconName("SHOP_material");

    info = UTverify_cast<SHOP_OperatorInfo *>(shop->getOpSpecificData());

    // Since the SHOP can evaluate multiple shader types, we need to specify
    // the SHOP is a material (combination of multiple shader types)
    info->setShaderType(SHOP_MATERIAL);

    // Currently, only support the "VMantra" renderer.
    //
    // As an alternative, you could set the rendermask to "*" and try to
    // support *all* renderers.
    info->setRenderMask("VMantra");

    table->addOperator(shop);
}

void
newShopOperator(OP_OperatorTable *table)
{
    // Plug-in hook to install the SHOP
    SHOP_Multi::install(table);
}
