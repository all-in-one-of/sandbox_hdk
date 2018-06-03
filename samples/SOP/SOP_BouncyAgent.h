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
 *
 * The BouncyAgent SOP
 *
 * Demonstrates example for creating a procedural agent primitive.
 *
 */


#ifndef __SOP_BOUNCYAGENT_H_INCLUDED__
#define __SOP_BOUNCYAGENT_H_INCLUDED__

#include <SOP/SOP_Node.h>
#include <PRM/PRM_Template.h>
#include <GU/GU_AgentDefinition.h>
#include <UT/UT_Array.h>
#include <SYS/SYS_Types.h>


class GU_PrimPacked;


namespace HDK_Sample
{

class SOP_BouncyAgent : public SOP_Node
{
public:
			     SOP_BouncyAgent(
				    OP_Network *net,
				    const char *name,
				    OP_Operator *op);
    virtual		    ~SOP_BouncyAgent();

    static PRM_Template	     myTemplateList[];
    static OP_Node	    *myConstructor(
				    OP_Network*, const char*, OP_Operator*);

protected:

    /// Method to provide input labels
    virtual const char	    *inputLabel(unsigned input_index) const;

    /// Method to cook geometry for the SOP
    virtual OP_ERROR	     cookMySop(OP_Context &context);

private:

    GU_AgentDefinitionPtr    createDefinition(fpreal t) const;

    void		     AGENTNAME(UT_String &s, fpreal t) const
				    { evalString(s, "agentname", 0, t); }
    fpreal		     HEIGHT(fpreal t) const
				    { return evalFloat("height", 0, t); }
    fpreal		     CLIPLENGTH(fpreal t) const
				    { return evalFloat("cliplength", 0, t); }
    fpreal		     CLIPOFFSET(fpreal t) const
				    { return evalFloat("clipoffset", 0, t); }

    static int		     onReload(void *data, int index, fpreal t,
				      const PRM_Template *tplate);

private:
    GU_AgentDefinitionPtr    myDefinition;
    UT_Array<GU_PrimPacked*> myPrims;
};

} // End HDK_Sample namespace

#endif // __SOP_BOUNCYAGENT_H_INCLUDED__
