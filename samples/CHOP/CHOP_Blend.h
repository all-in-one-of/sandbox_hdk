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
 *     Blends inputs 2,3,4... using the weights in input 1.
 */

#ifndef __CHOP_Blend__
#define __CHOP_Blend__

#include <CHOP/CHOP_Node.h>
#include <UT/UT_VectorTypes.h>

#define	 ARG_BLEND_DIFFERENCE	(myParmBase +0)
#define	 ARG_BLEND_FIRST_WEIGHT	(myParmBase +1)

namespace HDK_Sample {

class CHOP_Blend : public CHOP_Node
{
public:


    static OP_Node		*myConstructor(OP_Network*, const char *,
							    OP_Operator *);
    static OP_TemplatePair	 myTemplatePair;
    static OP_VariablePair	 myVariablePair;
    static PRM_Template		 myTemplateList[];
    static CH_LocalVariable	 myVariableList[];

    OP_ERROR			 cookMyChop(OP_Context &context);
    virtual bool		 updateParmsFlags();

protected:

				 CHOP_Blend(OP_Network	 *net, 
					     const char *name,
					     OP_Operator *op);
    virtual			~CHOP_Blend();

private:

    int		GETDIFFERENCE()
		{ return evalInt(ARG_BLEND_DIFFERENCE,0,0); }

    int		FIRST_WEIGHT()
		{ return evalInt(ARG_BLEND_FIRST_WEIGHT,0,0); }
    
    int		findFirstAvailableTracks(OP_Context &context);
    int		findInputClips(OP_Context &context, const CL_Clip *blendclip);

    const CL_Clip			*getCacheInputClip(int j);


    UT_IntArray				myAvailableTracks;
    UT_FprealArray			myTotalArray;
    UT_ValArray<const CL_Clip *>	myInputClip;

};

} // End HDK_Sample namespace
   
#endif

