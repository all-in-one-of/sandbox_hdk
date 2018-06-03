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
 */

#ifndef __CHOP_Stair__
#define __CHOP_Stair__

#include <UT/UT_ExpandArray.h>

namespace HDK_Sample {

#define ARG_STAIR_NUMBER	"number"
#define ARG_STAIR_HEIGHT	"height"
#define ARG_STAIR_OFFSET	"offset"
#define ARG_STAIR_DIRECTION	"direction"

#define ARG_STAIR_NAME		"channelname"
#define ARG_STAIR_RANGE		CHOP_RangeName.getToken()
#define ARG_STAIR_START		"start"
#define ARG_STAIR_END		"end"
#define ARG_STAIR_RATE		CHOP_SampleRateName.getToken()
#define ARG_STAIR_LEFT		CHOP_ExtendLeftName.getToken()
#define ARG_STAIR_RIGHT		CHOP_ExtendRightName.getToken()
#define ARG_STAIR_DEFAULT	CHOP_DefaultValueName.getToken()

class CHOP_Stair : public CHOP_Node
{
public:


    static OP_Node		*myConstructor(OP_Network*, const char *,
							    OP_Operator *);
    static OP_TemplatePair	 myTemplatePair;
    static OP_VariablePair	 myVariablePair;
    static PRM_Template		 myTemplateList[];
    static CH_LocalVariable	 myVariableList[];

    /// Overridden to generate our channel data
    OP_ERROR			 cookMyChop(OP_Context &context);

    /// Places the handles along the channels
    void			 cookMyHandles(OP_Context &context);

    virtual bool		 updateParmsFlags();

    /// Responds to user changes in any handle
    virtual fpreal		 handleChanged(CHOP_Handle *handle, 
					       CHOP_HandleData *hdata);

    virtual fpreal               shiftStart(fpreal new_offset, fpreal t);

protected:

				 CHOP_Stair(OP_Network  *net, 
					const char *name, OP_Operator *op);
    virtual			~CHOP_Stair();

    /// Returns true because we use the Units parameter.
    virtual int			 usesUnits() { return 1; }

    /// Returns true because we want to use the Scope parameter
    virtual int                  usesScope() const { return 0; }

    /// Stair has some local variables defined, this returns their value.
    virtual bool                 evalVariableValue(fpreal &v, int index,
						   int thread);
    // Add virtual overload that delegates to the super class to avoid
    // shadow warnings.
    virtual bool		 evalVariableValue(UT_String &v, int i,
						   int thread)
				 {
				     return CHOP_Node::evalVariableValue(
								v, i, thread);
				 }

private:

    /// @{
    /// Convenience parameter evaluation functions
    int		NUMBER(fpreal t)	 
    		{ return evalInt(ARG_STAIR_NUMBER, 0, t); }

    void	SET_NUMBER(fpreal t, int v)
    		{ setInt(ARG_STAIR_NUMBER, 0, t, v); }

    fpreal	HEIGHT(fpreal t)
		{ return evalFloat(ARG_STAIR_HEIGHT, 0, t); }

    void	SET_HEIGHT(fpreal t, fpreal v)
		{ setFloat(ARG_STAIR_HEIGHT, 0, t, v); }

    fpreal	OFFSET(fpreal t)
		{ return evalFloat(ARG_STAIR_OFFSET, 0, t); }

    void	SET_OFFSET(fpreal t, fpreal v)
		{ setFloat(ARG_STAIR_OFFSET, 0, t, v); }
    
    int		DIRECTION()	 
    		{ return evalInt(ARG_STAIR_DIRECTION, 0, 0); }

    void        CHAN_NAME(UT_String &label, fpreal t)
		{ evalString(label, ARG_STAIR_NAME, 0, t); }

    int		RANGE(fpreal t)
		{ return evalInt(ARG_STAIR_RANGE, 0, t); }

    void        SET_START(fpreal t, fpreal f)
		{ setFloat(ARG_STAIR_START, 0, t, toUnit(f)); }

    void        SET_END(fpreal t, fpreal f)
		{ setFloat(ARG_STAIR_END, 0, t, toUnit(f, UNITS(), 1)); }

    fpreal      RATE(fpreal t)      { return evalFloat(ARG_STAIR_RATE, 0, t); }

    void        SET_RATE(fpreal t, fpreal v)
		{ setFloat(ARG_STAIR_RATE, 0, t, v); }

    int         LEXTEND()          { return evalInt(ARG_STAIR_LEFT,  0, 0); }
    int         REXTEND()          { return evalInt(ARG_STAIR_RIGHT, 0, 0); }
    fpreal      DEFAULT(fpreal t)  { return evalFloat(ARG_STAIR_DEFAULT,0,t); }
    /// @}
  
    void	getInterval(fpreal t, fpreal *start, fpreal *end);

    /// Our local variable for "currently cooking channel"
    int		my_C;
    ///	Our local variable for "number of channels"
    int		my_NC;
    
    UT_ExpandArray  myExpandArray;
};

}	// End of HDK_Sample namespace

#endif

