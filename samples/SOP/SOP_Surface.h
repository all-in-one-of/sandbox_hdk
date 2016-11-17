/*
 * Copyright (c) 2009
 *    Side Effects Software Inc.  All rights reserved.
 *
 * Redistribution and use of Houdini Development Kit samples in source and
 * binary forms, with or without modification, are permitted provided 
that the
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
 * Surface SOP
 */


#ifndef __SOP_Surface_h__
#define __SOP_Surface_h__

#include <SOP/SOP_Node.h>

namespace HDK_Sample {
class SOP_Surface : public SOP_Node
{
public:
         SOP_Surface(OP_Network *net, const char *name, OP_Operator *op);
    virtual ~SOP_Surface();

    static PRM_Template    myTemplateList[];
    static OP_Node        *myConstructor(OP_Network*, const char *,
                                OP_Operator *);

protected:
    virtual const char      *inputLabel(unsigned idx) const;

    /// Method to cook geometry for the SOP
    virtual OP_ERROR         cookMySop(OP_Context &context);

private:
    fpreal  ISO(fpreal t)   { return evalFloat("iso", 0, t); }
    bool BUILDPOLYSOUP(fpreal t) { return evalInt("buildpolysoup", 0, t) != 0; }
};
} // End HDK_Sample namespace

#endif
