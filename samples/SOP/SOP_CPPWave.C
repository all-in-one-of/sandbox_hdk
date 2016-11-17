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
 */

/// This is the pure C++ implementation of the wave SOP.
/// @see @ref HOM/SOP_HOMWave.py, @ref HOM/SOP_HOMWaveNumpy.py, @ref HOM/SOP_HOMWaveInlinecpp.py, @ref HOM/SOP_HOMWave.C, @ref SOP/SOP_VEXWave.vfl

#include "SOP_CPPWave.h"

#include <GU/GU_Detail.h>
#include <GA/GA_Handle.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Director.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>
#include <SYS/SYS_Math.h>

using namespace HDK_Sample;

void
newSopOperator(OP_OperatorTable *table)
{
     table->addOperator(new OP_Operator(
        "cpp_wave",
        "CPP Wave ",
        SOP_CPPWave::myConstructor,
        SOP_CPPWave::myTemplateList,
        1,
        1,
        0));
}

PRM_Template
SOP_CPPWave::myTemplateList[] = {
    PRM_Template(),
};


OP_Node *
SOP_CPPWave::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_CPPWave(net, name, op);
}

SOP_CPPWave::SOP_CPPWave(OP_Network *net, const char *name, OP_Operator *op)
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

SOP_CPPWave::~SOP_CPPWave()
{
}

OP_ERROR
SOP_CPPWave::cookMySop(OP_Context &context)
{
    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    // Duplicate input geometry
    duplicateSource(0, context);

    // Flag the SOP as being time dependent (i.e. cook on time changes)
    flags().timeDep = 1;

    fpreal frame = OPgetDirector()->getChannelManager()->getSample(context.getTime());
    frame *= 0.03;

    // NOTE: If you are only interested in the P attribute, use gdp->getP(),
    //       or don't bother making a new GA_RWHandleV3, and just use
    //       gdp->getPos3(ptoff) and gdp->setPos3(ptoff, Pvalue).
    //       This just gives an example supplying an attribute name.
    GA_RWHandleV3 Phandle(gdp->findAttribute(GA_ATTRIB_POINT, "P"));
    GA_Offset ptoff;
    GA_FOR_ALL_PTOFF(gdp, ptoff)
    {
        UT_Vector3 Pvalue = Phandle.get(ptoff);
        Pvalue.y() = sin(Pvalue.x()*.2 + Pvalue.z()*.3 + frame);
        Phandle.set(ptoff, Pvalue);
    }
    // If we've modified an attribute, and we're managing our own data IDs,
    // we must bump the data ID for that attribute.
    Phandle.bumpDataId();

    return error();
}
