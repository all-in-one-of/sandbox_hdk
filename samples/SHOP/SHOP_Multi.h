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
 */

#ifndef __SHOP_Multi__
#define __SHOP_Multi__

#include <SHOP/SHOP_Node.h>

namespace HDK_Sample {
/// Typically a SHOP represents a single shader (i.e. surface, displacement,
/// etc.).  This example shows how to create a material that encompasses
/// multiple shaders (including rendering properties)
///
/// For example, if you want to have a material similar to the principled
/// shader that has surface and displacement shaders along with rendering
/// properties, this example will help.
class SHOP_Multi : public SHOP_Node
{
public:
    SHOP_Multi(OP_Network *parent, const char *name, OP_Operator *entry);
    virtual ~SHOP_Multi();

    static void	install(OP_OperatorTable *table);

    // Create a shader string
    virtual bool	 buildShaderString(UT_String &result, fpreal now,
				    const UT_Options *options,
				    OP_Node *obj=0, OP_Node *sop=0,
				    SHOP_TYPE interpret_type = SHOP_INVALID);

    // Find a SHOP of the desired type
    virtual SHOP_Node	*findShader(SHOP_TYPE type, fpreal now,
				    const UT_Options *options);
    // Test whether we match a given shader type
    virtual bool	 matchesShaderType(SHOP_TYPE type);

protected:
    virtual OP_ERROR	 cookMe(OP_Context &);
};

}
#endif
