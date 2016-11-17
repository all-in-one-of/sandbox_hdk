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
*/

#ifndef __VOP_CustomContext_h__
#define __VOP_CustomContext_h__

#include <SOP/SOP_Node.h>
#include <VOP/VOP_Node.h>
#include <OP/OP_OperatorTable.h>
#include <SYS/SYS_Types.h>

/// @file
/// This example contains code which shows how to create a custom VOP SOP in
/// the HDK. The custom VOP nodes within can dynamically change their inputs
/// and outputs. For simplicity, the Custom VOP SOP does not generate any
/// geometry.
namespace HDK_Sample {

class SOP_CustomVopOperatorFilter : public OP_OperatorFilter
{
public:
    virtual bool allowOperatorAsChild(OP_Operator *op);
};

/// C++ SOP node to provide the custom VOP context
class SOP_CustomVop : public SOP_Node
{
public:
    /// Creates an instance of this node with the given name in the given
    /// network.
    static OP_Node *		myConstructor(
				    OP_Network *net,
				    const char *name,
				    OP_Operator *entry);

    /// Our parameter templates.
    static PRM_Template		myTemplateList[];

    /// We override these to specify that our child network type is VOPs.
    // @{
    virtual const char *	getChildType() const;
    virtual OP_OpTypeId		getChildTypeID() const;
    // @}

    /// Override this to provide custom behaviour for what is allowed in the
    /// tab menu.
    virtual OP_OperatorFilter *	getOperatorFilter()
				    { return &myOperatorFilter; }

    /// Override this to do VOP network evaluation
    virtual OP_ERROR		cookMySop(OP_Context &context);

protected:
				SOP_CustomVop(
				    OP_Network *net,
				    const char *name,
				    OP_Operator *entry);
    virtual			~SOP_CustomVop();

private:
    OP_OperatorTable *		createAndGetOperatorTable();

private:
    SOP_CustomVopOperatorFilter	myOperatorFilter;
};

/// C++ VOP node to select one of its inputs and feed it into the output.
class VOP_CustomVop : public VOP_Node
{
public:
    /// Creates an instance of this node with the given name in the given
    /// network.
    static OP_Node *		myConstructor(
				    OP_Network *net,
				    const char *name,
				    OP_Operator *entry);

    /// Our parameter templates.
    static PRM_Template		myTemplateList[];

    /// This function is called to run the creation script (if there is one).
    /// The return value is false if the node was deleted while running the
    /// script. In this case obviously the node pointer becomes invalid and
    /// should not be used any more. It returns true if the node still exists.
    ///
    /// We override this in order to perform initial creation we are created
    /// created for the first time in a .hip file. This function is NOT called
    /// when being loaded from .hip files.
    virtual bool		runCreateScript();

    /// Provides the labels to appear on input and output buttons.
    // @{
    virtual const char *	inputLabel(unsigned idx) const;
    virtual const char *	outputLabel(unsigned idx) const;
    // @}

    /// Controls the number of input/output buttons visible on the node tile.
    // @{
    virtual unsigned		getNumVisibleInputs() const;
    virtual unsigned		getNumVisibleOutputs() const;
    // @}

    /// Returns the index of the first "unordered" input, meaning that inputs
    /// before this one should not be collapsed when they are disconnected.
    /// In this example, we don't need to implement this.
    //virtual unsigned		orderedInputs() const;

protected:
				VOP_CustomVop(
				    OP_Network *net,
				    const char *name,
				    OP_Operator *entry);
    virtual			~VOP_CustomVop();

    /// Returns the internal name of the supplied input. This input name is
    /// used when the code generator substitutes variables return by getCode.
    virtual void		getInputNameSubclass(
				    UT_String &name,
				    int idx) const;
    /// Reverse mapping of internal input names to an input index.
    virtual int			getInputFromNameSubclass(
				    const UT_String &name) const;
    /// Fills in the info about the vop type connected to the idx-th input.
    virtual void		getInputTypeInfoSubclass(
				    VOP_TypeInfo &type_info, int idx);

    /// Returns the internal name of the supplied output. This name is used
    /// when the code generator substitutes variables return by getCode.
    virtual void		getOutputNameSubclass(
				    UT_String &out,
				    int idx) const;
    /// Fills out the info about data type of each output connector.
    virtual void		getOutputTypeInfoSubclass(
				    VOP_TypeInfo &type_info, int idx);

    /// This methods should be overriden in most VOP nodes. It should fill in
    /// voptypes vector with vop types that are allowed to be connected to this
    /// node at the input at index idx. Note that 
    /// this method should return valid vop types even
    /// when nothing is connected to the corresponding input.
    virtual void		getAllowedInputTypeInfosSubclass(
				    unsigned idx,
				    VOP_VopTypeInfoArray &type_infos);

private:
    static void			nodeEventHandler(
				    OP_Node *caller,
				    void *callee,
				    OP_EventType type,
				    void *data);

    void			handleParmChanged(int parm_index);

private:

};

} // End of HDK_Sample namespace

#endif // __VOP_CustomContext_h__
