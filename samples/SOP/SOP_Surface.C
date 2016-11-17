/*
 * Copyright (c) 2009
 *    Side Effects Software Inc.  All rights reserved.
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
 * The Surface SOP.  This SOP Surfaces the volume into polygons.
 */

#include "SOP_Surface.h"

#include <SOP/SOP_Guide.h>
#include <GU/GU_Detail.h>
#include <GU/GU_Surfacer.h>
#include <GU/GU_PrimPoly.h>
#include <GU/GU_PrimVolume.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_VoxelArray.h>

using namespace HDK_Sample;

void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        "hdk_surface",
        "Surface",
        SOP_Surface::myConstructor,
        SOP_Surface::myTemplateList,
        1,
        1,
        0));
}

static PRM_Name names[] = {
    PRM_Name("iso",          "Iso Value"),
    PRM_Name("buildpolysoup","Build Polygon Soup"),
};

PRM_Template
SOP_Surface::myTemplateList[] = {
    PRM_Template(PRM_FLT,    1, &names[0], PRMzeroDefaults),
    PRM_Template(PRM_TOGGLE, 1, &names[1], PRMzeroDefaults),
    PRM_Template(),
};


OP_Node *
SOP_Surface::myConstructor(OP_Network *net, const char *name, 
OP_Operator *op)
{
    return new SOP_Surface(net, name, op);
}

SOP_Surface::SOP_Surface(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op)
{
}

SOP_Surface::~SOP_Surface() {}

OP_ERROR
SOP_Surface::cookMySop(OP_Context &context)
{
    fpreal t = context.getTime();

    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    const float iso = ISO(t);
    const bool makepolysoup = BUILDPOLYSOUP(t);

    // Erase our gdp but keep it around for reuse
    // This is the same as clearAndDestroy() in terms of the result
    // but keeps a list of all the primitives around so that
    // if you immediately recreate the same thing it can be done
    // very fast.
    gdp->stashAll();

    // Get first input
    const GU_Detail *volgdp = inputGeo(0);

    const GEO_Primitive *prim = 0;
    const GEO_PrimVolume *vol = 0;
    if (volgdp->getNumPrimitives() > 0)
    {
        GA_Offset primoff = volgdp->primitiveOffset(GA_Index(0));
	prim = volgdp->getGEOPrimitive(primoff);
	if (prim->getTypeId() == GA_PRIMVOLUME)
	    vol = (const GEO_PrimVolume *)prim;
    }

    if (vol)
    {
	UT_VoxelArrayReadHandleF vox(vol->getVoxelHandle());

	// This is only a rough approximation of the volume size.
	// It does not take into account the rotation of the box.
	UT_BoundingBox bbox;
	vol->getBBox(&bbox);
	UT_Vector3 pos = bbox.minvec();
	UT_Vector3 size = bbox.size();
	int rx, ry, rz;
	vol->getRes(rx, ry, rz);

	GU_Surfacer surfacer(*gdp, pos, size, rx, ry, rz, makepolysoup);
	
	for (int z = 0; z < rz; z++)
	{
	    for (int y = 0; y < ry; y++)
	    {
		for (int x = 0; x < rx; x++)
		{
		    bool isless = false;
		    bool ismore = false;

		    // The surfacer wants the eight corner points
		    // of the cube to surface.
		    fpreal density[8];
		    for (int d = 0; d < 8; d++)
		    {
			density[d] = vox->getValue(x + ((d>>0) & 1),
			                           y + ((d>>1) & 1),
			                           z + ((d>>2) & 1));
			density[d] -= iso;
			if (density[d] < 0.0)
			    isless = true;
			else
			    ismore = true;
		    }
		    // We don't need to surface voxels that don't have
		    // a crossing point.
		    // If your data structure supports sparse volumes,
		    // this can be used to avoid even reading the values.
		    if (isless && ismore)
		    {
			surfacer.addCell(x, y, z, density, 0);
		    }
		}
	    }
	}
    }

    // free anything not reused
    gdp->destroyStashed();

    return error();
}

const char *
SOP_Surface::inputLabel(unsigned) const
{
    return "Geometry to Surface";
}
