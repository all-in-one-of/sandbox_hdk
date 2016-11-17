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
 * The BlindData SOP.  This SOP simply saves some additional information to the
 * HIP file.  It's cook method is simply copies the source geometry.
 */

#include "SOP_BlindData.h"

#include <GU/GU_Detail.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_SaveFlags.h>
#include <PRM/PRM_Include.h>

#include <UT/UT_CPIO.h> // For saving/loading need CPIO packets
#include <UT/UT_DSOVersion.h>
#include <UT/UT_StringStream.h>

using namespace HDK_Sample;

void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        "hdk_blinddata",
        "BlindData",
        SOP_BlindData::myConstructor,
        SOP_BlindData::myTemplateList,
        1,
        1,
        0));
}

PRM_Template
SOP_BlindData::myTemplateList[] = {
    PRM_Template(),
};


OP_Node *
SOP_BlindData::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_BlindData(net, name, op);
}

SOP_BlindData::SOP_BlindData(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op)
{
    // This SOP does nothing to the input geometry, so it might as
    // well not bump all data IDs when cooking.
    mySopFlags.setManagesDataIDs(true);
}

SOP_BlindData::~SOP_BlindData() {}

OP_ERROR
SOP_BlindData::cookMySop(OP_Context &context)
{
    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    duplicateSource(0, context);

    return error();
}

const char *
SOP_BlindData::inputLabel(unsigned) const
{
    return "Source Geometry";
}

static const char *theExtension = "mydata";

OP_ERROR
SOP_BlindData::save(std::ostream &os, const OP_SaveFlags &flags,
                    const char *path_prefix)
{
    UT_CPIO packet;

    clearErrors();

    // First, let the base class save its stuff.
    if (SOP_Node::save(os, flags, path_prefix) >= UT_ERROR_ABORT)
        return error();

    UT_OStringStream ts;
    ts << path_prefix << getName() << "." << theExtension << std::ends;
    packet.open(os, ts.str().buffer());
    savePrivateData(os, flags.getBinary());
    packet.close(os);

    return error();
}

bool
SOP_BlindData::load(UT_IStream &is, const char *extension, const char *path)
{
    if (!strcmp(extension, theExtension))
    {
        // Here's my blind data!
        loadPrivateData(is);
        return (error() < UT_ERROR_ABORT);
    }
    return SOP_Node::load(is, extension, path);
}

int
SOP_BlindData::loadPrivateData(UT_IStream &is)
{
    UT_String data;
    bool result = data.load(is);
    return !result ? 0 : 1;
}

int
SOP_BlindData::savePrivateData(std::ostream &os, int binary)
{
    UT_String data;
    data = "This is my private data";
    data.save(os, binary);
    return !os ? 0 : 1;
}
