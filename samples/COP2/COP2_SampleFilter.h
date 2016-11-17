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
 * This operator does a simple pixel modification on its input.
 */
#ifndef _COP2_SAMPLEFILTER_H_
#define _COP2_SAMPLEFILTER_H_

#include <COP2/COP2_Context.h>
#include <COP2/COP2_MaskOp.h>

namespace HDK_Sample {

/// @brief Simple example of a kernel filter.

/// This is an HDK example of a 3x3 kernel filter which uses template classes
/// to abstract the operation for various data formats. It also demonstrates
/// how to deal with fetching input areas larger than a tile, and how to
/// enlarging the canvas for the COP.
class COP2_SampleFilter : public COP2_MaskOp
{
public:
    /// All nodes are instantiated via a myConstructor method.
    static OP_Node		*myConstructor(OP_Network*, const char *,
					       OP_Operator *);
    
    /// @{
    /// Parameters, local variables and input labals.
    static OP_TemplatePair	 myTemplatePair;
    static OP_VariablePair	 myVariablePair;
    static PRM_Template		 myTemplateList[];
    static CH_LocalVariable	 myVariableList[];
    static const char		*myInputLabels[];
    /// @}
    
    /// Given an area of the image to cook, indicate which parts of the input's
    /// image are required
    virtual void	 getInputDependenciesForOutputArea(
			    COP2_CookAreaInfo &output_area,
			    const COP2_CookAreaList &input_areas,
			    COP2_CookAreaList &needed_areas);
protected:
    /// This operation expands the canvas bounds by 1 pixel in all directions.
    /// computeImageBounds() announces this to the COP engine.
    virtual void	 computeImageBounds(COP2_Context &context);

    /// Returns a new context instance with the parameters for the filter in
    /// it, to be used by the many threads on the tiles.
    virtual COP2_ContextData	*newContextData(const TIL_Plane *p,
						int array_index,
						float t,
						int xres, int yres,
						int thread,
						int max_threads);

    /// COP2_MaskOp defines lots of nice mask blending operations in
    /// cookMyTile(), and defines doCookMyTile for us to override instead.
    /// This is where the actual image operation is performed.
    virtual OP_ERROR	doCookMyTile(COP2_Context &context,
				     TIL_TileList *tilelist);

    /// Returns a description of the operation for the node info popup.
    virtual const char  *getOperationInfo();
    
    virtual	~COP2_SampleFilter();
private:
		COP2_SampleFilter(OP_Network *parent, const char *name,
			     OP_Operator *entry);

    /// @{
    /// Parameter evaluation method; can call evalFloat directly as well.
    fpreal	LEFT(fpreal t)
		{ return evalFloat("left",0,t); }
    
    fpreal	RIGHT(fpreal t)
		{ return evalFloat("right",0,t); }
    
    fpreal	TOP(fpreal t)
		{ return evalFloat("top",0,t); }
    
    fpreal	BOTTOM(fpreal t)
		{ return evalFloat("bottom",0,t); }
    /// @}
};

/// Storage class for our parameters and the kernel
class cop2_SampleFilterContext : public COP2_ContextData
{
public:
		 cop2_SampleFilterContext()
		     : myLeft(0.0f), myRight(0.0f), myTop(0.0f), myBottom(0.0f),
		       myKernel(NULL)
		 {}
    
    virtual	~cop2_SampleFilterContext() { delete [] myKernel; }

    /// When true, this context data object is recreated for each plane.
    virtual bool createPerPlane() const { return false; }

    /// When true, this context data object is recreated for each resolution
    /// cooked.
    virtual bool createPerRes() const   { return true; }

    /// When true, this context data object is recreated for each frame. In most
    /// cases, this returns true since parameters can be animated.
    virtual bool createPerTime() const	{ return true; }

    /// When true, each thread gets its own version of a context data. This is
    /// useful if the context data contains allocated memory to be used for 
    /// intermediate steps.
    virtual bool createPerThread() const{ return false; }

    /// @{
    /// Cached Parameters
    float	myLeft;
    float	myRight;
    float	myTop;
    float	myBottom;
    /// @}
    
    /// Kernel filter derived from parameters
    float      *myKernel;
};

} // End HDK_Sample namespace

#endif
