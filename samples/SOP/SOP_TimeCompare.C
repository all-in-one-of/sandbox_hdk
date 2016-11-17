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
 * The TimeCompare SOP.  This SOP TimeCompares the geometry onto a plane.
 */

#include "SOP_TimeCompare.h"

#include <SOP/SOP_Error.h>
#include <GU/GU_Detail.h>
#include <GA/GA_AIFMath.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_Director.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>

using namespace HDK_Sample;

void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        "hdk_timecompare",
        "Time Compare",
        SOP_TimeCompare::myConstructor,
        SOP_TimeCompare::myTemplateList,
        2,  // Must have at least 2 inputs
        2,  // Max 2 inputs
        0));
}

static PRM_Default frameDefault(0, "$FF");

static PRM_Name names[] = {
    PRM_Name("attrib",      "Comparison Point Attribute"),
    PRM_Name("resultattrib","Result Attribute"),
    PRM_Name("frame",       "Second Input Frame"),
};

PRM_Template
SOP_TimeCompare::myTemplateList[] = {
    PRM_Template(PRM_STRING,    1, &PRMgroupName, 0, &SOP_Node::pointGroupMenu,
				0, 0, SOP_Node::getGroupSelectButton(
						GA_GROUP_POINT)),
    PRM_Template(PRM_STRING,	1, &names[0]),
    PRM_Template(PRM_STRING,	1, &names[1]),
    PRM_Template(PRM_FLT_J,	1, &names[2], &frameDefault),
    PRM_Template(),
};


OP_Node *
SOP_TimeCompare::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_TimeCompare(net, name, op);
}

SOP_TimeCompare::SOP_TimeCompare(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op), myGroup(0)
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

SOP_TimeCompare::~SOP_TimeCompare() {}

OP_ERROR
SOP_TimeCompare::cookInputGroups(OP_Context &context, int alone)
{
    // The SOP_Node::cookInputPointGroups() provides a good default
    // implementation for just handling a point selection.
    return cookInputPointGroups(
        context, // This is needed for cooking the group parameter, and cooking the input if alone.
        myGroup, // The group (or NULL) is written to myGroup if not alone.
        alone,   // This is true iff called outside of cookMySop to update handles.
                 // true means the group will be for the input geometry.
                 // false means the group will be for gdp (the working/output geometry).
        true,    // (default) true means to set the selection to the group if not alone and the highlight flag is on.
        0,       // (default) Parameter index of the group field
        -1,      // (default) Parameter index of the group type field (-1 since there isn't one)
        true,    // (default) true means that a pointer to an existing group is okay; false means group is always new.
        false,   // (default) false means new groups should be unordered; true means new groups should be ordered.
        true,    // (default) true means that all new groups should be detached, so not owned by the detail;
                 //           false means that new point and primitive groups on gdp will be owned by gdp.
        0        // (default) Index of the input whose geometry the group will be made for if alone.
    );
}

OP_ERROR
SOP_TimeCompare::cookMySop(OP_Context &context)
{
    fpreal t = context.getTime();

    // Note we only lock our first input here as it is the one we want
    // to evaluate at context.getTime().  OP_AutoLockInputs will
    // automatically unlock any locked inputs when we return.
    // NOTE: Don't call unlockInput or unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lockInput(0, context) >= UT_ERROR_ABORT)
        return error();

    // Now compute our second input's time.
    OP_Context secondinput_context = context;

    // This is the frame number we want to evaluate
    fpreal secondinput_t = FRAME(t);

    // The context takes seconds, not frame, so we convert.
    secondinput_t = OPgetDirector()->getChannelManager()->getTime(secondinput_t);

    // Adjust our cooking context to reflect this
    secondinput_context.setTime(secondinput_t);

    // And lock our second input at this specified time.
    // Note if we fail we still have to unlock our first input before
    // returning, but this is done automatically by OP_AutoLockInputs.
    if (inputs.lockInput(1, secondinput_context) >= UT_ERROR_ABORT)
        return error();

    // Duplicate our incoming geometry
    duplicateSource(0, context);

    // Fetch our attribute names
    UT_String attribname;
    ATTRIB(attribname, t);
    UT_String resultname;
    RESULTATTRIB(resultname, t);

    // We do some simple validation here.  If no attribute names
    // are specified, we treat it as a no-op.
    if (!attribname.isstring() || !resultname.isstring())
        return error();

    // Attributes need to follow certain naming conventions for
    // compatibility with other houdini tools.  
    attribname.forceValidVariableName();
    resultname.forceValidVariableName();

    // Find the specified attribute in our source geometry
    const GU_Detail *agdp = inputGeo(0);
    const GU_Detail *bgdp = inputGeo(1);
    const GA_Attribute *ah = agdp->findAttribute(GA_ATTRIB_POINT, attribname);
    const GA_Attribute *bh = bgdp->findAttribute(GA_ATTRIB_POINT, attribname);

    // If source attribute doesn't exist, error.
    if (!ah || !bh)
    {
        addError(SOP_ATTRIBUTE_INVALID, (const char *)attribname);
        return error();
    }

    // Create a destination attribute on our own gdp.
    // First see if it already exists.
    GA_Attribute *dsth = gdp->findAttribute(GA_ATTRIB_POINT, resultname);
    const GA_AIFMath *math = dsth ? dsth->getAIFMath() : NULL;
    if (!math)
    {
        // Clone our source attribute
        dsth = gdp->getAttributes().cloneAttribute(
            GA_ATTRIB_POINT, resultname, *ah, true);

        const GA_AIFMath *math = dsth ? dsth->getAIFMath() : NULL;
        if (!math)
        {
            addError(SOP_ATTRIBUTE_INVALID, dsth ? (const char *)attribname
                                                 : (const char *)resultname);
            return error();
        }
    }

    if (error() >= UT_ERROR_ABORT)
        return error();

    // Here we determine which groups we have to work on.  We only
    // handle point groups.
    if (cookInputGroups(context) >= UT_ERROR_ABORT)
        return error();

    GA_Offset ptoff;
    GA_FOR_ALL_GROUP_PTOFF(gdp, myGroup, ptoff)
    {
        GA_Index ptind = gdp->pointIndex(ptoff);
        // Fetch the corresponding point from our inputs.  We know
        // we have a corresponding point in agdp since we duplicated
        // from it, but bgdp might have fewer points.  We just
        // ignore them in this case.
        GA_Offset aptoff = agdp->pointOffset(ptind);
        if (ptind >= bgdp->getNumPoints())
            continue;
        GA_Offset bptoff = bgdp->pointOffset(ptind);

        math->sub(*dsth, ptoff, *ah, aptoff, *bh, bptoff);
    }

    // We've modified dsth, so we must bump its data ID.
    dsth->bumpDataId();

    return error();
}
