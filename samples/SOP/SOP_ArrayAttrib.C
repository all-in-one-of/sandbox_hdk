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
 * ArrayAttrib SOP
 */

#include "SOP_ArrayAttrib.h"

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
        "hdk_arrayattrib",
        "Array Attribute",
        SOP_ArrayAttrib::myConstructor,
        SOP_ArrayAttrib::myTemplateList,
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
SOP_ArrayAttrib::myTemplateList[]=
{
    PRM_Template(PRM_STRING,	1, &sop_names[0], 0),
    PRM_Template()
};


OP_Node *
SOP_ArrayAttrib::myConstructor(OP_Network *net,const char *name,OP_Operator *entry)
{
    return new SOP_ArrayAttrib(net, name, entry);
}


SOP_ArrayAttrib::SOP_ArrayAttrib(OP_Network *net, const char *name, OP_Operator *entry)
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

SOP_ArrayAttrib::~SOP_ArrayAttrib()
{
}


OP_ERROR
SOP_ArrayAttrib::cookMySop(OP_Context &context)
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

    GA_Attribute	*attrib = gdp->findIntArray(GA_ATTRIB_POINT,
					    aname, 
					    // Allow any tuple size to match
					    -1, -1);
    if (!attrib)
    {
	attrib = gdp->addIntArray(GA_ATTRIB_POINT,
				    aname,
		    // Tuple size one means we group the array into
		    // logical groups of 1.  It does *NOT* affect
		    // the length of the arrays, which are always
		    // measured in ints.
				    1);
    }

    // See if we failed to make the array.
    if (!attrib)
    {
	UT_WorkBuffer		buf;
	buf.sprintf("Failed to create array attributes \"%s\"",
		    (const char *) aname);
	addError(SOP_MESSAGE, buf.buffer());
	return error();
    }

    // Make sure we are an array.  Note tuples do not match to this,
    // nor do GA_*Handle* match!
    // We will match both int and float here, however.
    // (For string, getAIFSharedStringArray)
    const GA_AIFNumericArray *aif = attrib->getAIFNumericArray();
    if (!aif)
    {
	UT_WorkBuffer		buf;
	buf.sprintf("Attribute \"%s\" not a numeric array!",
		    (const char *) aname);
	addError(SOP_MESSAGE, buf.buffer());
	return error();
    }

    // We keep our read/write array outside of the inner loop.
    // This allows this to grow to the maximum size encountered
    // and avoid reallocations.
    UT_IntArray		data;

    GA_Offset		ptoff;
    GA_FOR_ALL_PTOFF(gdp, ptoff)
    {
	// Fetch array value
	aif->get(attrib, ptoff, data);

	// Append our point number if the current length
	// is less than the point number.
	GA_Index	ptidx = gdp->pointIndex(ptoff);
	if (data.entries() < ptidx)
	    data.append(ptidx);

	// Write back
	aif->set(attrib, ptoff, data);
    }


    // Mark as modified.
    attrib->bumpDataId();

    return error();
}

