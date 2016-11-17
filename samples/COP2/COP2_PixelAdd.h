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
 * A sample of a pixel operation. Pixel operations are those OPs which use the
 * values from their plane and pixel location only to determine the output. For
 * example, HSV and Gamma are Pixel Operations, whereas Blur (since it accesses
 * neighbouring pixels) and Premultiply (since it requires both color and alpha
 * planes) are not.
 *
 * Pixel Operations, when placed in series, are combined into 1 operation.
 */
#ifndef _COP2_PIXEL_ADD_H_
#define _COP2_PIXEL_ADD_H_

class RU_PixelFunction;

#include <COP2/COP2_PixelOp.h>

namespace HDK_Sample {

class COP2_PixelAdd : public COP2_PixelOp
{
public:
    
    static OP_Node		*myConstructor(OP_Network*, const char *,
					       OP_Operator *);
    static OP_TemplatePair	 myTemplatePair;
    static OP_VariablePair	 myVariablePair;
    static PRM_Template		 myTemplateList[];
    static CH_LocalVariable	 myVariableList[];
    static const char *		 myInputLabels[];

protected:
    /// This is the only function we need to override for a pixel function.
    /// It returns our pixel function, which must be derived from
    /// RU_PixelFunction.
    virtual RU_PixelFunction	*addPixelFunction(const TIL_Plane *, int,
						  float t, int, int,
						  int thread);
    
private:
		COP2_PixelAdd(OP_Network *parent, const char *name,
			   OP_Operator *entry);
    virtual	~COP2_PixelAdd();
    
    /// An optional method which returns a short description of what this node
    /// does in the OP info popup.
    virtual const char  *getOperationInfo();

    fpreal	ADD(int comp, fpreal t)
		{ return evalFloat("addend",comp,t); }
};

} // End HDK_Sample namespace

#endif
