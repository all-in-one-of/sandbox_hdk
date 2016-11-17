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
 * COMMENTS:
 *	Simulates spring-like action on an input channel.
 */

#ifndef __CHOP_Spring__
#define __CHOP_Spring__

#include <CHOP/CHOP_Realtime.h>

// First Page
#define ARG_SPR_SPRING_CONSTANT		(myParmBase + 0)
#define ARG_SPR_MASS 			(myParmBase + 1)
#define ARG_SPR_DAMPING_CONSTANT	(myParmBase + 2)
#define ARG_SPR_METHOD			(myParmBase + 3)
#define ARG_SPR_GRAB_INITIAL		(myParmBase + 4)
#define ARG_SPR_INITIAL_DISPLACEMENT	(myParmBase + 5)
#define ARG_SPR_INITIAL_VELOCITY	(myParmBase + 6)

class UT_Interrupt;

namespace HDK_Sample {

class CHOP_Spring : public CHOP_Realtime
{
public:

    static OP_Node		*myConstructor(OP_Network  *, 
	    				       const char  *,
					       OP_Operator *);
    static OP_TemplatePair	 myTemplatePair;
    static OP_VariablePair	 myVariablePair;
    static PRM_Template		 myTemplateList[];
    static CH_LocalVariable	 myVariableList[];

    OP_ERROR			 cookMyChop(OP_Context &context);
    OP_ERROR			 cookMySlice(OP_Context &context,
					     int start, int end);
    
    virtual bool		 updateParmsFlags();

    virtual bool		 evalVariableValue(
				    fpreal &val, int index, int thread);
    // Add virtual overload that delegates to the super class to avoid
    // shadow warnings.
    virtual bool		 evalVariableValue(
				    UT_String &v, int i, int thread)
				 {
				     return evalVariableValue(v, i, thread);
				 }

    virtual int          	 usesScope() const { return 1; };
    virtual int          	 usesSampleMatch() const { return 0; };
    virtual int          	 usesUnits() { return getInput(0) ? 0 : 1; };

    virtual int			 isSteady() const;
    virtual const char *	 getTimeSliceExtension() { return "spring"; }

protected:

				 CHOP_Spring(OP_Network	 *net, 
					     const char  *name,
					     OP_Operator *op);
    virtual			~CHOP_Spring();
    
    virtual ut_RealtimeData *	 newRealtimeDataBlock(const char *name,
						      const CL_Track *track,
						      fpreal t);

private:
    
    // First Page
    int		METHOD()
		{ return evalInt(ARG_SPR_METHOD, 0, 0); }

    fpreal	SPRING_CONSTANT(fpreal t)
		{ return evalFloat(ARG_SPR_SPRING_CONSTANT, 0, t); }

    fpreal	MASS(fpreal t)
		{ return evalFloat(ARG_SPR_MASS, 0, t); }

    void	SET_MASS(fpreal t, fpreal f)
		{ setFloat(ARG_SPR_MASS, 0, t, f); }

    fpreal	DAMPING_CONSTANT(fpreal t)
		{ return evalFloat(ARG_SPR_DAMPING_CONSTANT, 0, t); }

    bool	GRAB_INITIAL()
		{ return (bool) evalInt(ARG_SPR_GRAB_INITIAL, 0, 0); }

    fpreal	INITIAL_DISPLACEMENT(fpreal t)
		{ return evalFloat(ARG_SPR_INITIAL_DISPLACEMENT, 0, t); }

    fpreal	INITIAL_VELOCITY(fpreal t)
		{ return evalFloat(ARG_SPR_INITIAL_VELOCITY, 0, t); }

    int		my_NC;
    int		my_C;
    int		myChannelDependent;
    int		mySteady;
};

} // End HDK_Sample namespace

#endif
