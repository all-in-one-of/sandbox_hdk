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
 * The IKSample SOP
 *
 * Demonstrates example use of the Inverse Kinematics (IK) Solver found in the
 * KIN library.
 *
 */


#ifndef __SOP_IKSAMPLE_H_INCLUDED__
#define __SOP_IKSAMPLE_H_INCLUDED__

#include <SOP/SOP_Node.h>
#include <KIN/KIN_Chain.h>
#include <UT/UT_VectorTypes.h>
#include <SYS/SYS_Types.h>


namespace HDK_Sample
{

class SOP_IKSample : public SOP_Node
{
public:
			 SOP_IKSample(
				OP_Network *net,
				const char *name,
				OP_Operator *op);
    virtual		~SOP_IKSample();

    static PRM_Template	 myTemplateList[];
    static OP_Node	*myConstructor(OP_Network*, const char*, OP_Operator*);

protected:

    /// Method to provide input labels
    virtual const char	*inputLabel(unsigned input_index) const;

    /// Method to cook geometry for the SOP
    virtual OP_ERROR	 cookMySop(OP_Context &context);

private:

    bool		 evaluateSolverParms(
				OP_Context &context,
				KIN_InverseParm &parms);

    bool		 setupRestChain();

    void		 GOAL(fpreal t, UT_Vector3R &goal) const
				{       evalFloats("goal", goal.data(), t); }
    fpreal		 TWIST(fpreal t) const
				{ return evalFloat("twist", 0, t); }
    fpreal		 DAMPEN(fpreal t) const
				{ return evalFloat("dampen", 0, t); }
    int			 STRAIGHTEN(fpreal t) const
				{ return   evalInt("straighten", 0, t); }
    fpreal		 THRESHOLD(fpreal t) const
				{ return evalFloat("threshold", 0, t); }

private:
    KIN_Chain		 myRestChain;
};

} // End HDK_Sample namespace

#endif // __SOP_IKSAMPLE_H_INCLUDED__
