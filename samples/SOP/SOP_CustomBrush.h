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

#ifndef __SOP_CustomBrush_h__
#define __SOP_CustomBrush_h__

#include <SOP/SOP_Node.h>
#include <UT/UT_Array.h>

namespace HDK_Sample {
enum
{
    // Group parameters
    SOP_CUSTOMBRUSH_GRP_IDX = 0,
    SOP_CUSTOMBRUSH_ORIGIN_IDX,
    SOP_CUSTOMBRUSH_DIRECTION_IDX,
    SOP_CUSTOMBRUSH_RADIUS_IDX,
    SOP_CUSTOMBRUSH_COLOR_IDX,
    SOP_CUSTOMBRUSH_ALPHA_IDX,
    SOP_CUSTOMBRUSH_OPERATION_IDX,
    SOP_CUSTOMBRUSH_EVENT_IDX,
    SOP_CUSTOMBRUSH_CLEARALL_IDX
};

// structure to describe the paint applied to a particular point
// the colors are assumed to be pre-multiplied by the alpha
struct SOP_CustomBrushData
{
    SOP_CustomBrushData() {}
    SOP_CustomBrushData(GA_Index ptnum, fpreal r, fpreal g, fpreal b, fpreal a) : myPtNum(ptnum), myRed(r), myGreen(g), myBlue(b), myAlpha(a) {}

    GA_Index myPtNum;
    fpreal myRed;
    fpreal myGreen;
    fpreal myBlue;
    fpreal myAlpha;
};

class SOP_CustomBrush : public SOP_Node
{
public:
    // our constructor
    SOP_CustomBrush(OP_Network *net, const char *name, OP_Operator *op);
    // our destructor
    virtual ~SOP_CustomBrush();

    static PRM_Template myTemplateList[];
    static OP_Node *myConstructor(OP_Network*, const char *, OP_Operator *);

    virtual OP_ERROR cookInputGroups(OP_Context &context, int alone = 0);

    // used by undo/redo to update the paint
    void updateData(exint numpts, UT_Array<SOP_CustomBrushData> &data);

protected:
    // Method to cook geometry for the SOP
    virtual OP_ERROR cookMySop(OP_Context &context);

    // save the SOP's data (including the applied paint)
    virtual OP_ERROR save(std::ostream &os, const OP_SaveFlags &flags,
			  const char *path_prefix,
			  const UT_String &name_override = UT_String());
    // load the SOP's data (including the applied paint)
    virtual bool load(UT_IStream &is, const char *extension,
		      const char *path);

    // callback used by the "Clear All" parameter
    static int clearAllStatic(void *op, int, fpreal time, const PRM_Template *);
    void clearAll();

private:
   UT_Vector3 evalVector3(int idx, fpreal t)
	{
	    return UT_Vector3(evalFloat(idx, 0, t),
			      evalFloat(idx, 1, t),
			      evalFloat(idx, 2, t));
	}

    UT_Vector3 getOrigin(fpreal t)
	{ return evalVector3(SOP_CUSTOMBRUSH_ORIGIN_IDX, t); }

    UT_Vector3 getDirection(fpreal t)
	{ return evalVector3(SOP_CUSTOMBRUSH_DIRECTION_IDX, t); }

    fpreal getRadius(fpreal t)
	{ return evalFloat(SOP_CUSTOMBRUSH_RADIUS_IDX, 0, t); }

    UT_Vector3 getBrushColor(fpreal t)
	{ return evalVector3(SOP_CUSTOMBRUSH_COLOR_IDX, t); }

    fpreal getBrushAlpha(fpreal t)
	{ return evalFloat(SOP_CUSTOMBRUSH_ALPHA_IDX, 0, t); }

    int getOperation(fpreal t)
	{ return evalInt(SOP_CUSTOMBRUSH_OPERATION_IDX, 0, t); }

    int getEvent(fpreal t)
	{ return evalInt(SOP_CUSTOMBRUSH_EVENT_IDX, 0, t); }

    const GA_PointGroup *myGroup;

    // expected number of points in the input
    GA_Size myNumPts;

    // contains previous expected number of points in the input for undos
    GA_Size myOldNumPts;

    // stores applied paint values
    UT_Array<SOP_CustomBrushData> myData;

    // contains previous paint values for undos
    UT_Array<SOP_CustomBrushData> myOldData;
};
} // End HDK_Sample namespace

#endif
