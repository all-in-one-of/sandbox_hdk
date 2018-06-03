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
 * BlindData SOP.  This example shows how to save private data to a HIP file.
 */


#ifndef __SOP_BlindData_h__
#define __SOP_BlindData_h__

#include <SOP/SOP_Node.h>
#include <UT/UT_IStream.h>

namespace HDK_Sample {
class SOP_BlindData : public SOP_Node
{
public:
	     SOP_BlindData(OP_Network *net, const char *name, OP_Operator *op);
    virtual ~SOP_BlindData();

    static PRM_Template		 myTemplateList[];
    static OP_Node		*myConstructor(OP_Network*, const char *,
							    OP_Operator *);

protected:
    virtual const char          *inputLabel(unsigned idx) const;
    virtual OP_ERROR		 cookMySop(OP_Context &context);

    virtual OP_ERROR		 save(std::ostream &os, const OP_SaveFlags &flags,
				      const char *path_prefix,
				      const UT_String &name_override = UT_String());
    virtual bool		 load(UT_IStream &is, const char *extension,
				      const char *path);
private:

    int			loadPrivateData(UT_IStream &is);
    int			savePrivateData(std::ostream &os, int binary);
};
} // End HDK_Sample namespace

#endif
