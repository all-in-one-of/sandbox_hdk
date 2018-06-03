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
 * The NURBS SOP
 */

#include "SOP_NURBS.h"
#include <GU/GU_Detail.h>
#include <GU/GU_PrimNURBSurf.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>
#include <SYS/SYS_Math.h>
#include <limits.h>
#include <stddef.h>

using namespace HDK_Sample;

///
/// newSopOperator is the hook that Houdini grabs from this dll
/// and invokes to register the SOP.  In this case we add ourselves
/// to the specified operator table.
///
void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        "hdk_nurbs",                    // Internal name
        "NURBS",                        // UI name
        SOP_NURBS::myConstructor,       // How to build the SOP
        SOP_NURBS::myTemplateList,      // My parameters
        0,                              // Min # of sources
        0,                              // Max # of sources
        0,                              // Local variables
        OP_FLAG_GENERATOR));            // Flag it as generator
}

PRM_Template
SOP_NURBS::myTemplateList[] = {
    PRM_Template(PRM_INT,			// Integer parameter.
		PRM_Template::PRM_EXPORT_TBX,	// Export to top of viewer
		2, &PRMdivName, PRMfourDefaults),
    PRM_Template(PRM_INT,
		PRM_Template::PRM_EXPORT_TBX,	// Export to top of viewer
		2, &PRMorderName, PRMfourDefaults,	
		NULL, &PRMorderRange),
    PRM_Template()
};

OP_Node *
SOP_NURBS::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_NURBS(net, name, op);
}

SOP_NURBS::SOP_NURBS(OP_Network *net, const char *name, OP_Operator *op)
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

SOP_NURBS::~SOP_NURBS() {}

OP_ERROR
SOP_NURBS::cookMySop(OP_Context &context)
{
    fpreal now = context.getTime();

    int nu = SYSmax(COLS(now), 2); // Number of columns
    int nv = SYSmax(ROWS(now), 2); // Number of rows
    int uorder = SYSmin(UORDER(now), nu);
    int vorder = SYSmin(VORDER(now), nv);

    if (gdp->getNumPrimitives() == 1)
    {
        // NOTE: This SOP only ever generates a GEO_PrimNURBSurf,
        //       so we don't strictly *need* to check that it is
        //       one if the detail wasn't cleared, but it's best
        //       to check, just in case.
        GA_Primitive *prim = gdp->getPrimitiveByIndex(0);
        if (prim->getTypeId() == GA_PRIMNURBSURF)
        {
            GEO_PrimNURBSurf *surf = (GEO_PrimNURBSurf *)prim;
            if (surf->getNumRows() != nv || surf->getNumCols() != nu)
            {
                // Just delete and start over if wrong rows/cols.
                surf = NULL;
            }
            if (surf && surf->getUOrder() != uorder)
            {
                // Just 
                GA_NUBBasis *basis = new GA_NUBBasis(0, 1, nu+uorder, uorder, true);
                if (surf->setUBasis(basis) != 0)
                {
                    delete basis;
                    surf = NULL;
                }
                else
                {
                    // We successfully modified a primitive, so we must
                    // bump the primitive list data ID.
                    gdp->getPrimitiveList().bumpDataId();
                }
            }
            if (surf && surf->getVOrder() != vorder)
            {
                GA_NUBBasis *basis = new GA_NUBBasis(0, 1, nv+vorder, vorder, true);
                if (surf->setVBasis(basis) != 0)
                {
                    delete basis;
                    surf = NULL;
                }
                else
                {
                    // We successfully modified a primitive, so we must
                    // bump the primitive list data ID.
                    gdp->getPrimitiveList().bumpDataId();
                }
            }
            if (surf)
            {
                // Nothing to more do for existing NURBS surface.
                return error();
            }
        }
    }

    // In addition to destroying everything except the empty P
    // and topology attributes, this bumps the data IDs for
    // those remaining attributes, as well as the primitive list
    // data ID.
    gdp->clearAndDestroy();

    // Create a NURBS surface.
    GEO_PrimNURBSurf *surf = GU_PrimNURBSurf::build(gdp, nv, nu, uorder, vorder);

    /// @see GEO_TPSurf for basis access methods.

    for (int u = 0; u < nu; u++)
    {
        fpreal s = SYSfit((fpreal)u, u, (fpreal)nu-1, 0, 1);
        for (int v = 0; v < nv; v++)
        {
            fpreal t = SYSfit((fpreal)v, v, (fpreal)nv-1, 0, 1);

            UT_Vector4 P(s, 0, t, 1);
            GA_Offset ptoff = surf->getPointOffset(v, u); // row, column
            gdp->setPos4(ptoff, P);
        }
    }

    // We don't need to bump any data IDs here, because they were already
    // bumped in clearAndDestroy().

    return error();
}
