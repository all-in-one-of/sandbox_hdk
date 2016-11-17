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
 * Constant SampleGenerator COP
 */
#ifndef __COP2_SAMPLEGENERATOR_H__
#define __COP2_SAMPLEGENERATOR_H__

#include <UT/UT_Vector3.h>
#include <UT/UT_Vector4.h>
#include <COP2/COP2_Generator.h>

namespace HDK_Sample {

/// @brief Simple COP generator example for the HDK

/// This HDK example demonstrates how to generate image data in COPs. It 
/// generates random white noise.
///
class COP2_SampleGenerator : public COP2_Generator
{
public:
    static OP_Node		*myConstructor(OP_Network*, const char *,
					       OP_Operator *);
    /// *{
    /// Static lists to define parameters and local variables 
    static OP_TemplatePair	 myTemplatePair;
    static OP_VariablePair	 myVariablePair;
    static PRM_Template		 myTemplateList[];
    static CH_LocalVariable	 myVariableList[];
    /// *}
    
    /// Determine frame range, image composition and other sequence info
    virtual TIL_Sequence *cookSequenceInfo(OP_ERROR &error);
    
protected:
    /// Evaluate parms and stash data for cooking in a COP2_ContextData object
    virtual COP2_ContextData	*newContextData(const TIL_Plane *, int,
						float t, int xres, int yres,
						int thread,
						int max_threads);

    /// Create the image data for a single tile list - multithreaded call
    virtual OP_ERROR		generateTile(COP2_Context &context,
					     TIL_TileList *tilelist);

    virtual	~COP2_SampleGenerator();
private:
		 COP2_SampleGenerator(OP_Network *parent, const char *name,
			    OP_Operator *entry);

    /// *{
    /// Parameter evaluation methods
    int		SEED(fpreal t)
		{ return evalInt("seed",0,t); }

    void	AMP(UT_Vector4 &amp, fpreal t)
		{ amp.x() =  evalFloat("ampl",0,t);
		  amp.y() =  evalFloat("ampl",1,t);
		  amp.z() =  evalFloat("ampl",2,t);
		  amp.w() =  evalFloat("ampl",3,t); }
    /// *}
};

/// @brief Data class to hold parm values and data for COP2_SampleGenerator

/// This class is used to hold the evaluated parms and data needed for the 
/// cook. Because the cook method generateTiles() is threaded and called
/// multiple times, this class caches any data needed once and is used by many
/// tiles and threads, to reduce redundancy.
class cop2_SampleGeneratorData : public COP2_ContextData
{
public:
	     cop2_SampleGeneratorData() : myAmp(0.0f,0.0f,0.0f), mySeed(0) { }
    
    virtual ~cop2_SampleGeneratorData() { ; }

    UT_Vector4	myAmp;
    int		mySeed;
};

} // End HDK_Sample namespace

#endif
