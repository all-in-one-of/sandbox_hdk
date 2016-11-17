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

#ifndef __SOP_PrimVOP__
#define __SOP_PrimVOP__

#include <SOP/SOP_Node.h>
#include <CVEX/CVEX_Value.h>
#include <VOP/VOP_CodeGenerator.h>
#include <VOP/VOP_ExportedParmsManager.h>

class CVEX_RunData;
class CVEX_Context;
class OP_Caller;

namespace HDK_Sample {
class SOP_PrimVOP : public SOP_Node
{
public:
	     SOP_PrimVOP(OP_Network *net, const char *, OP_Operator *entry);
    virtual ~SOP_PrimVOP();

    static OP_Node	*myConstructor(OP_Network  *net, const char *name,
				       OP_Operator *entry);
    static PRM_Template	 myTemplateList[];

    /// Overriding these are what allow us to contain VOPs
    virtual OP_OperatorFilter	*getOperatorFilter();
    virtual const char	*getChildType() const;
    virtual OP_OpTypeId	 getChildTypeID() const;

    virtual VOP_CodeGenerator	*getVopCodeGenerator();
    virtual bool		 hasVexShaderParameter(const char *parm_name);

    /// We need special alerts now that we contain VOPs.
    virtual void	 opChanged(OP_EventType reason, void *data=0);

    /// Code generation variables.
    virtual bool	 evalVariableValue(
				UT_String &value, int index, int thread);
protected:
    virtual OP_ERROR	 cookMySop   (OP_Context &context);

    void		 executeVex(int argc, char **argv,
				fpreal t, OP_Caller &opcaller);

    void		 processVexBlock(CVEX_Context &context,
				    CVEX_RunData &rundata,
				    int argc, char **argv,
				    int *primid, int n,
				    fpreal t);

    int			 VEXSRC(fpreal t)
			 { return evalInt("vexsrc", 0, t); }
    void		 SCRIPT(UT_String &s, fpreal t)
			 { evalString(s, "script", 0, t); }

    void		 SHOPPATH(UT_String &path, fpreal t)
			 { evalString(path, "shoppath", 0, t); }

    /// VOP and VEX functions
    virtual void	 finishedLoadingNetwork(bool is_child_call=false);
    virtual void	 addNode(OP_Node *node, int notify=1, int explicitly=1);
    virtual void	 getNodeSpecificInfoText(OP_Context &context,
					    OP_NodeInfoParms &parms);

    void		 buildScript(UT_String &script, fpreal t);

    /// Every operator that has a VOP code generator needs to define
    /// this method in order for Parameter VOP changes to be reflected
    /// in the VOP network's parameter interface.
    /// 
    /// This method requires <VOP/VOP_CodeGenerator.h> 
    /// and <VOP/VOP_ExportedParmsManager.h> to be included.
    virtual void	 ensureSpareParmsAreUpdatedSubclass()
			 {
			    // Check if the spare parameter templates
			    // are out-of-date.
			    if (getVopCodeGenerator()
				&& eventMicroNode(OP_SPAREPARM_MODIFIED)
				    .requiresUpdate(0.0))
			    {
				// Call into the code generator to update
				// the spare parameter templates.
				getVopCodeGenerator()
				    ->exportedParmsManager()
				    ->updateOwnerSpareParmLayout();
			    }
			 }

    VOP_CodeGenerator	 myCodeGenerator;
};
} // End HDK_Sample namespace

#endif

