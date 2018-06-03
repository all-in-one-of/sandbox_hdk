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
 * The Tetra SOP
 */

#include "SOP_Tetra.h"
#include "GEO_PrimTetra.h"

#include <GU/GU_Detail.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <CH/CH_LocalVariable.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_Vector3.h>
#include <SYS/SYS_Types.h>
#include <limits.h>
#include <stddef.h>

using namespace HDK_Sample;

//
// Help is stored in a "wiki" style text file.  The text file should be copied
// to $HOUDINI_PATH/help/nodes/sop/tetra.txt
//
// See the sample_install.sh file for an example.
//


///
/// newSopOperator is the hook that Houdini grabs from this dll
/// and invokes to register the SOP.  In this case we add ourselves
/// to the specified operator table.
///
void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        "hdk_tetra",                // Internal name
        "Tetra",                    // UI name
        SOP_Tetra::myConstructor,   // How to build the SOP
        SOP_Tetra::myTemplateList,  // My parameters
        0,                          // Min # of sources
        0,                          // Max # of sources
        0,                          // Local variables
        OP_FLAG_GENERATOR));        // Flag it as generator
}

PRM_Template
SOP_Tetra::myTemplateList[] = {
    PRM_Template(PRM_FLT, 1, &PRMradiusName, PRMoneDefaults),
    PRM_Template(PRM_XYZ, 3, &PRMcenterName),
    PRM_Template()
};


OP_Node *
SOP_Tetra::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_Tetra(net, name, op);
}

SOP_Tetra::SOP_Tetra(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op)
{
    // This indicates that this SOP manually manages its data IDs,
    // so that Houdini can identify what attributes may have changed,
    // e.g. to reduce work for the viewport, or other SOPs that
    // check whether data IDs have changed.
    // By default, (i.e. if this line weren't here), all data IDs
    // would be bumped after the SOP cook, to indicate that
    // everything might have changed.
    // If some data IDs don't get bumped properly, the viewport
    // may not update, or SOPs that check data IDs
    // may not cook correctly, so be *very* careful!
    mySopFlags.setManagesDataIDs(true);
}

SOP_Tetra::~SOP_Tetra() {}

OP_ERROR
SOP_Tetra::cookMySop(OP_Context &context)
{
    fpreal now = context.getTime();

    // Since we don't have inputs, we don't need to lock them.

    float rad = RADIUS(now);
    UT_Vector3 translate(CENTERX(now), CENTERY(now), CENTERZ(now));

    // Try to reuse the existing tetrahedron, if the detail
    // hasn't been cleared.
    GEO_PrimTetra *tet = NULL;
    if (gdp->getNumPrimitives() == 1)
    {
        GA_Primitive *prim = gdp->getPrimitiveByIndex(0);

        // This type check is not strictly necessary, since
        // this SOP only ever makes a GEO_PrimTetra, but it's
        // here just in case, and as an example.
        if (prim->getTypeId() == GEO_PrimTetra::theTypeId())
            tet = (GEO_PrimTetra *)prim;
    }

    if (tet == NULL)
    {
        // In addition to destroying everything except the empty P
        // and topology attributes, this bumps the data IDs for
        // those remaining attributes, as well as the primitive list
        // data ID.
        gdp->clearAndDestroy();

        // Build a tetrahedron
        tet = GEO_PrimTetra::build(gdp, true);
    }

    for (int i = 0; i < 4; i++)
    {
        UT_Vector3 pos(i == 1, i == 2, i == 3);
        pos *= rad;
        pos += translate;
        GA_Offset ptoff = tet->getPointOffset(i);
        gdp->setPos3(ptoff, pos);
    }

    // We've modified P, so we need to bump the data ID,
    // (though in the case where we did clearAndDestroy(), it's
    // redundant, because that already bumped it).
    gdp->getP()->bumpDataId();

    // Set the node selection for this primitive. This will highlight all
    // the tets generated by the node, but only if the highlight flag for this 
    // node is on and the node is selected.
    select(GA_GROUP_PRIMITIVE);

    return error();
}
