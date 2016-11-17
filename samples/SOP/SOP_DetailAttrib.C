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
 * DetailAttrib SOP
 */

#include "SOP_DetailAttrib.h"

#include <GU/GU_Detail.h>
#include <GA/GA_Handle.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>

using namespace HDK_Sample;
void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        "hdk_detailattrib",
        "DetailAttrib",
        SOP_DetailAttrib::myConstructor,
        SOP_DetailAttrib::myTemplateList,
        1,
        1));
}


static PRM_Name		sop_names[] = {
    PRM_Name("attribname",     	"Attribute"),
    PRM_Name("value",      	"Value"),
};

static PRM_Default	sop_valueDefault(0.1);
static PRM_Range	sop_valueRange(PRM_RANGE_RESTRICTED,0,PRM_RANGE_UI,1);

PRM_Template
SOP_DetailAttrib::myTemplateList[]=
{
    PRM_Template(PRM_STRING,	1, &sop_names[0], 0),
    PRM_Template(PRM_FLT_J,	1, &sop_names[1], &sop_valueDefault, 0,
				   &sop_valueRange),
    PRM_Template()
};


OP_Node *
SOP_DetailAttrib::myConstructor(OP_Network *net,const char *name,OP_Operator *entry)
{
    return new SOP_DetailAttrib(net, name, entry);
}


SOP_DetailAttrib::SOP_DetailAttrib(OP_Network *net, const char *name, OP_Operator *entry)
    : SOP_Node(net, name, entry)
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

SOP_DetailAttrib::~SOP_DetailAttrib()
{
}


OP_ERROR
SOP_DetailAttrib::cookMySop(OP_Context &context)
{
    fpreal t = context.getTime();

    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    duplicateSource(0, context);

    UT_String aname;
    ATTRIBNAME(aname, t);

    // Try with a float
    GA_RWHandleF attrib(gdp->findFloatTuple(GA_ATTRIB_DETAIL, aname));

    // Not present, so create the detail attribute:
    if (!attrib.isValid())
        attrib = GA_RWHandleF(gdp->addFloatTuple(GA_ATTRIB_DETAIL, aname, 1));

    if (attrib.isValid())
    {
        // Store the value in the detail attribute
        // NOTE: The detail is *always* at GA_Offset(0)
        attrib.set(GA_Offset(0), VALUE(t));
        attrib.bumpDataId();
    }

    return error();
}

