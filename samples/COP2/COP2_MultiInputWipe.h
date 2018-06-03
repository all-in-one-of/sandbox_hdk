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
 * Sample multi-input wipe COP
 */
#ifndef _COP2_COP2_MultiInputWipe_H_
#define _COP2_COP2_MultiInputWipe_H_

#include <COP2/COP2_MultiBase.h>

namespace HDK_Sample {

class COP2_MultiInputWipe : public COP2_MultiBase
{
public:
    // For the output area (an area of a plane belonging to this node)
    // and a set of input areas, determine which input areas and which
    // parts of these areas are needed to cook the output area.
    virtual void	 getInputDependenciesForOutputArea(
			    COP2_CookAreaInfo &output_area,
			    const COP2_CookAreaList &input_areas,
			    COP2_CookAreaList &needed_areas);

    static OP_Node		*myConstructor(OP_Network*, const char *,
					       OP_Operator *);
    static OP_TemplatePair	 myTemplatePair;
    static OP_VariablePair	 myVariablePair;
    static PRM_Template		 myTemplateList[];
    static CH_LocalVariable	 myVariableList[];
    static const char *		 myInputLabels[];

protected:
    virtual ~COP2_MultiInputWipe();
    virtual COP2_ContextData	*newContextData(const TIL_Plane *p,
						int array_index,
						float t,
						int xres, int yres,
						int thread,
						int max_threads);
    
    virtual void	 computeImageBounds(COP2_Context &context);
    virtual OP_ERROR	cookMyTile(COP2_Context &context,TIL_TileList *tiles);

    
    virtual void	 passThroughTiles(COP2_Context &context,
					  const TIL_Plane *plane,
					  int array_index,
					  float t,
					  int xstart, int ystart,
					  TIL_TileList *&tile,
					  int block = 1,
					  bool *mask = 0,
					  bool *blocked = 0);

    virtual int		 passThrough(COP2_Context &context,
				     const TIL_Plane *plane, int comp_index,
				     int array_index, float t,
				     int xstart, int ystart);

private:
 
		COP2_MultiInputWipe(OP_Network *parent, const char *name,
				    OP_Operator *entry);

    void	boostAndBlur(TIL_TileList *tiles, TIL_Region *input,
			     float fade, float dip, int rad, float blur,
			     bool add);
};
    
} // End HDK_Sample namespace
#endif
