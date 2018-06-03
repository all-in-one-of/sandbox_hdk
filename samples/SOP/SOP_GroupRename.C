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
 * The GroupRename SOP.  This SOP renames groups.
 */

#include "SOP_GroupRename.h"

#include <GU/GU_Detail.h>
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
        "hdk_grouprename",
        "GroupRename",
        SOP_GroupRename::myConstructor,
        SOP_GroupRename::myTemplateList,
        1,
        1,
        0));
}

static PRM_Name names[] = {
    PRM_Name("oldname", "Old Name"),
    PRM_Name("newname", "New Name"),
};

PRM_Template
SOP_GroupRename::myTemplateList[] = {
    PRM_Template(PRM_STRING,    1, &names[0], 0, &SOP_Node::groupMenu),
    PRM_Template(PRM_STRING,    1, &names[1]),
    PRM_Template(),
};


OP_Node *
SOP_GroupRename::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_GroupRename(net, name, op);
}

SOP_GroupRename::SOP_GroupRename(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op)
{
    // This SOP is a special case for data IDs.  It's important to note
    // that the name is *not* part of the data that the ID is identifying.
    // That way, attributes with different names can be identified as
    // having the same data.
    mySopFlags.setManagesDataIDs(true);
}

SOP_GroupRename::~SOP_GroupRename() {}

OP_ERROR
SOP_GroupRename::cookMySop(OP_Context &context)
{
    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    // Duplicate incoming geometry.
    duplicateSource(0, context);

    fpreal now = context.getTime();
    UT_String oldname;
    OLDNAME(oldname, now);
    UT_String newname_str;
    NEWNAME(newname_str, now);

    // Rename all matching groups.
    UT_StringHolder newname(newname_str);
    gdp->getElementGroupTable(GA_ATTRIB_PRIMITIVE).renameGroup(oldname,newname);
    gdp->getElementGroupTable(GA_ATTRIB_POINT).renameGroup(oldname, newname);

    // NOTE: No data IDs need to be bumped when just renaming an attribute,
    //       because the data is considered to exclude the name.

    return error();
}

const char *
SOP_GroupRename::inputLabel(unsigned) const
{
    return "Geometry to Rename Groups in";
}
