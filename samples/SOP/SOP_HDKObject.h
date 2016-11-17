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
 * This is an example of an implementation of the Object Merge SOP to
 * demonstrate using DYNAMIC_PATHs, MULTIPARMS, and proper cooking of other
 * SOPs.
 */

#ifndef __SOP_HDKObject_h__
#define __SOP_HDKObject_h__

#include <CH/CH_ExprLanguage.h>
#include <SOP/SOP_Node.h>

namespace HDK_Sample {
class SOP_HDKObject : public SOP_Node
{
public:
	    SOP_HDKObject(OP_Network *net, const char *name, 
			  OP_Operator *entry);
    virtual ~SOP_HDKObject();

    virtual bool		 updateParmsFlags();
    static OP_Node		*myConstructor(OP_Network *net, const char *name, OP_Operator *entry);
    static PRM_Template		 myTemplateList[];
    static PRM_Template		 myObsoleteList[];

    // Because the object merge can reference an op chain which has a 
    // subnet with different D & R we must follow all our node's
    // d & r status
    virtual int			 getDandROpsEqual();
    virtual int			 updateDandROpsEqual(int = 1) 
				 { return getDandROpsEqual(); }

    int 	NUMOBJ() { return evalInt("numobj", 0, 0.0f); }
    void	setNUMOBJ(int num_obj)
		    { setInt("numobj", 0, 0.0f, num_obj); }

    int		ENABLEMERGE(int i)
		{
		    return evalIntInst("enable#", &i, 0, 0.0f);
		}
    void	setENABLEMERGE(int i, int val)
		{
		    setIntInst(val, "enable#", &i, 0, 0.0f);
		}

    void	SOPPATH(UT_String &str, int i, fpreal t)
		{
		    evalStringInst("objpath#", &i, str, 0, t);
		}
    void	setSOPPATH( UT_String &str, CH_StringMeaning meaning,
			    int i, fpreal t)
		{
		    setStringInst(str, meaning, "objpath#", &i, 0, t);
		}

    void	XFORMPATH(UT_String &str, fpreal t)
		{
		    evalString(str, "xformpath", 0, t);
		}
protected:
    virtual OP_ERROR		 cookMySop(OP_Context &context);
};
} // End HDK_Sample namespace

#endif
