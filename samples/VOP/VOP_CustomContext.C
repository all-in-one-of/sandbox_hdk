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
*/

#include <UT/UT_DSOVersion.h>
#include "VOP_CustomContext.h"

#include <VOP/VOP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_Input.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <CH/CH_Manager.h>
#include <UT/UT_Assert.h>
#include <UT/UT_WorkBuffer.h>
#include <SYS/SYS_Types.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


using namespace HDK_Sample;


//////////////////////////////////////////////////////////////////////////////
//
// SOP_CustomVop Node Registration
//

void
newSopOperator(OP_OperatorTable *table)
{
    OP_Operator *   op;

    op = new OP_Operator(
		"hdk_customvop",		    // internal name
		"Custom VOP",			    // UI label
		SOP_CustomVop::myConstructor,	    // class factory
		SOP_CustomVop::myTemplateList,	    // parm definitions
		SOP_CustomVop::theChildTableName,
		0,				    // min # of inputs
		0 				    // max # of inputs
		);
    table->addOperator(op);
}

const char *SOP_CustomVop::theChildTableName = VOP_TABLE_NAME;

///////////////////////////////////////////////////////////////////////////////
//
// SOP_CustomVop Implementation
//

OP_Node *
SOP_CustomVop::myConstructor(
	OP_Network *net, const char *name, OP_Operator *entry)
{
    return new SOP_CustomVop(net, name, entry);
}

PRM_Template
SOP_CustomVop::myTemplateList[] = 
{
    PRM_Template()	// List terminator
};

SOP_CustomVop::SOP_CustomVop(
	OP_Network *net, const char *name, OP_Operator *entry)
    : SOP_Node(net, name, entry)
{
    createAndGetOperatorTable();
}

SOP_CustomVop::~SOP_CustomVop()
{
}

const char *
SOP_CustomVop::getChildType() const
{
    return VOP_OPTYPE_NAME;
}

OP_OpTypeId
SOP_CustomVop::getChildTypeID() const
{
    return VOP_OPTYPE_ID;
}

// Create dummy VOP_Operator subclass so that we can tag it as operators
// created by us by type. We also take this chance to conveniently simplify
// it's construction as well. Notice that besides the large range of inputs
// that we give it, we also give it an "invalid" vopnet mask so that it won't
// appear in any of the other VOP context networks.
class sop_CustomVopOperator : public VOP_Operator
{
public:
    sop_CustomVopOperator(const char *name, const char *label)
	: VOP_Operator(
		name,				// internal name
		label,				// UI label
		VOP_CustomVop::myConstructor,   // How to create one
		VOP_CustomVop::myTemplateList,  // parm definitions
		SOP_CustomVop::theChildTableName,
		0,				// Min # of inputs
		VOP_VARIABLE_INOUT_MAX,		// Max # of inputs
		"invalid",			// vopnet mask
		NULL,				// Local variables
		OP_FLAG_UNORDERED,		// Special flags
		VOP_VARIABLE_INOUT_MAX)		// # of outputs
    {
    }
};

OP_OperatorTable *
SOP_CustomVop::createAndGetOperatorTable()
{
    // We chain our custom VOP operators onto the default VOP operator table.
    OP_OperatorTable &table = *OP_Network::getOperatorTable(VOP_TABLE_NAME);

    // Procedurally create some simple operator types for illustrative purposes.
    table.addOperator(new sop_CustomVopOperator("hdk_inout11_", "In-Out 1-1"));
    table.addOperator(new sop_CustomVopOperator("hdk_inout21_", "In-Out 2-1"));
    table.addOperator(new sop_CustomVopOperator("hdk_inout12_", "In-Out 1-2"));
    table.addOperator(new sop_CustomVopOperator("hdk_inout22_", "In-Out 2-2"));

    // Notify observers of the operator table that it has been changed.
    table.notifyUpdateTableSinksOfUpdate();

    return &table;
}

OP_ERROR
SOP_CustomVop::cookMySop(OP_Context &context)
{
    // Do evaluation of the custom VOP network here.
    return error();
}

///////////////////////////////////////////////////////////////////////////////
//
// VOP_CustomVop
//

OP_Node *
VOP_CustomVop::myConstructor(
	OP_Network *net, const char *name, OP_Operator *entry)
{
    return new VOP_CustomVop(net, name, entry);
}

static PRM_Name	    vopPlugInputs("inputs", "Inputs");
static PRM_Name	    vopPlugInpName("inpplug#", "Input Name #");
static PRM_Default  vopPlugInpDefault(0, "input1");
static PRM_Name	    vopPlugOutputs("outputs", "Outputs");
static PRM_Name	    vopPlugOutName("outplug#", "Output Name #");
static PRM_Default  vopPlugOutDefault(0, "output1");

static PRM_Template
vopPlugInpTemplate[] =
{
    PRM_Template(PRM_ALPHASTRING, 1, &vopPlugInpName, &vopPlugInpDefault),
    PRM_Template() // List terminator
};
static PRM_Template
vopPlugOutTemplate[] =
{
    PRM_Template(PRM_ALPHASTRING, 1, &vopPlugOutName, &vopPlugOutDefault),
    PRM_Template() // List terminator
};

PRM_Template
VOP_CustomVop::myTemplateList[] = 
{
    PRM_Template(PRM_MULTITYPE_LIST, vopPlugInpTemplate, 0, &vopPlugInputs,
		 PRMzeroDefaults, 0, &PRM_SpareData::multiStartOffsetZero),

    PRM_Template(PRM_MULTITYPE_LIST, vopPlugOutTemplate, 0, &vopPlugOutputs,
		 PRMzeroDefaults, 0, &PRM_SpareData::multiStartOffsetZero),

    PRM_Template()		// List terminator
};

VOP_CustomVop::VOP_CustomVop(
	OP_Network *parent, const char *name, OP_Operator *entry)
    : VOP_Node(parent, name, entry)
{
    // Add our event handler.
    addOpInterest(this, &VOP_CustomVop::nodeEventHandler);
}

VOP_CustomVop::~VOP_CustomVop()
{
    // Failing to remove our event handler can cause Houdini to crash so make
    // sure we do it.
    removeOpInterest(this, &VOP_CustomVop::nodeEventHandler);
}

// This function is called to run the creation script (if there is one).
// The return value is false if the node was deleted while running the
// script. In this case obviously the node pointer becomes invalid and
// should not be used any more. It returns true if the node still exists.
//
// We override this in order to perform initial creation we are created
// created for the first time in a .hip file. This function is NOT called
// when being loaded from .hip files.
bool
VOP_CustomVop::runCreateScript()
{
    if (!VOP_Node::runCreateScript())
	return false;

    const UT_String &	type_name = getOperator()->getName();
    fpreal		t = CHgetEvalTime();
    UT_WorkBuffer	plugname;
    int			n;

    // For simplicity, we just initialize our number of inputs/outputs based
    // upon our node type name.
    n = type_name(type_name.length() - 3) - '0';
    setInt(vopPlugInputs.getToken(), 0, t, n);
    for (int i = 0; i < n; i++)
    {
	plugname.sprintf("input%d", i + 1);
	setStringInst(plugname.buffer(), CH_STRING_LITERAL,
		      vopPlugInpName.getToken(), &i, 0, t);
    }

    n = type_name(type_name.length() - 2) - '0';
    setInt(vopPlugOutputs.getToken(), 0, t, n);
    for (int i = 0; i < n; i++)
    {
	plugname.sprintf("output%d", i + 1);
	setStringInst(plugname.buffer(), CH_STRING_LITERAL,
		      vopPlugOutName.getToken(), &i, 0, t);
    }

    return true;
}

const char *
VOP_CustomVop::inputLabel(unsigned idx) const
{
    static UT_WorkBuffer    theLabel;
    fpreal		    t = CHgetEvalTime();
    int			    i = idx;
    UT_String		    label;

    // Evaluate our label from the corresponding parameter.
    evalStringInst(vopPlugInpName.getToken(), &i, label, 0, t);
    if (label.isstring())
	theLabel.strcpy(label);
    else    
	theLabel.strcpy("<unnamed>");

    return theLabel.buffer();
}

const char *
VOP_CustomVop::outputLabel(unsigned idx) const
{
    static UT_WorkBuffer    theLabel;
    fpreal		    t = CHgetEvalTime();
    int			    i = idx;
    UT_String		    label;

    // Evaluate our label from the corresponding parameter.
    evalStringInst(vopPlugOutName.getToken(), &i, label, 0, t);
    if (label.isstring())
	theLabel.strcpy(label);
    else    
	theLabel.strcpy("<unnamed>");

    return theLabel.buffer();
}

unsigned
VOP_CustomVop::getNumVisibleInputs() const
{
    /// For simplicity, we just use the user specified parameter.
    return evalInt("inputs", 0, CHgetEvalTime());
}

unsigned
VOP_CustomVop::getNumVisibleOutputs() const
{
    /// For simplicity, we just use the user specified parameter.
    return evalInt("outputs", 0, CHgetEvalTime());
}

void
VOP_CustomVop::getInputNameSubclass(UT_String &name, int idx) const
{
    // For simplicity, we just do the same thing as our input/output labels.
    name.harden(inputLabel(idx));
}

int
VOP_CustomVop::getInputFromNameSubclass(const UT_String &name) const
{
    // There are more efficient ways to do this, but just do a dumb reverse
    // lookup here.
    for (int i = 0; i < getNumVisibleInputs(); i++)
    {
	if (name == inputLabel(i))
	    return i;
    }
    return -1;
}

void
VOP_CustomVop::getInputTypeInfoSubclass(VOP_TypeInfo &type_info, int idx)
{
    // For simplicity, just assume everything is a float
    type_info.setType(VOP_TYPE_FLOAT);
}

void	 
VOP_CustomVop::getAllowedInputTypeInfosSubclass( unsigned idx,
				    VOP_VopTypeInfoArray &type_infos)
{
    // For simplicity, just assume everything is a float
    type_infos.append( VOP_TypeInfo( VOP_TYPE_FLOAT ));
}

void
VOP_CustomVop::getOutputNameSubclass(UT_String &name, int idx) const
{
    // For simplicity, we just do the same thing as our input/output labels.
    name.harden(outputLabel(idx));
}

void
VOP_CustomVop::getOutputTypeInfoSubclass(VOP_TypeInfo &type_info, int idx)
{
    // For simplicity, just assume everything is a float
    type_info.setType(VOP_TYPE_FLOAT);
}

void
VOP_CustomVop::nodeEventHandler(
	OP_Node *caller, void *callee, OP_EventType type, void *data)
{
    switch (type)
    {
	case OP_PARM_CHANGED:
	    static_cast<VOP_CustomVop*>(callee)->handleParmChanged((int)(intptr_t)data);
	    break;
	default:
	    break;
    }
}

void
VOP_CustomVop::handleParmChanged(int parm_index)
{
    // On any parameter change for us, trigger a notification that the node's
    // UI needs to update so that the Network Pane will redraw with the changed
    // inputs/outputs.
    triggerUIChanged(OP_UICHANGE_CONNECTIONS);
}


///////////////////////////////////////////////////////////////////////////////
//
// SOP_CustomVopOperatorFilter Implementation
//

bool
SOP_CustomVopOperatorFilter::allowOperatorAsChild(OP_Operator *op)
{
    return (dynamic_cast<sop_CustomVopOperator *>(op) != NULL);
}


