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
 * This class implements an object that aligns the inherited orientation to the
 * world axes while preserving the location in space. It also illustrates how to
 * hide all the parameters inherited from the base class.
 *
 */

#ifndef __OBJ_WorldAlign__
#define __OBJ_WorldAlign__

#include <OBJ/OBJ_Geometry.h>

namespace HDK_Sample {
class OBJ_WorldAlign : public OBJ_Geometry
{
public:
    // Standard constructor and destructor.
				 OBJ_WorldAlign(OP_Network *net,
					   const char *name,
					   OP_Operator *op);
    virtual			~OBJ_WorldAlign();

    // Instantiates a new node of the type corresponding to this operator.
    static OP_Node              *myConstructor(OP_Network *net,
					       const char *name,
					       OP_Operator *entry);

    // Constructs a list of the parameters for this operator.
    static OP_TemplatePair	*buildTemplatePair(OP_TemplatePair *prevstuff);


protected:
    // Performs the calculation of the local and the world transformation.
    virtual OP_ERROR		 cookMyObj(OP_Context &context);

private:
};

}	// End HDK_Sample namespace

#endif
