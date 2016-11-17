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
 */

#include "SOP_CustomBrush.h"

#include <GA/GA_Handle.h>
#include <GA/GA_Types.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_SaveFlags.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_CPIO.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_IStream.h>
#include <UT/UT_Map.h>
#include <UT/UT_OStream.h>
#include <UT/UT_Undo.h>
#include <UT/UT_UndoManager.h>
#include <UT/UT_Vector3.h>
#include <SYS/SYS_Types.h>
#include <stddef.h>

// undo for CustomBrush
namespace HDK_Sample {
class SOP_UndoCustomBrushData : public UT_Undo
{
public:
    SOP_UndoCustomBrushData(SOP_CustomBrush *sop,
			     GA_Size oldnumpts,
			     GA_Size numpts,
			     UT_Array<SOP_CustomBrushData> &olddata,
			     UT_Array<SOP_CustomBrushData> &data);

    virtual void undo();
    virtual void redo();

private:
    int mySopId;
    GA_Size myOldNumPts;
    GA_Size myNumPts;
    UT_Array<SOP_CustomBrushData> myOldData;
    UT_Array<SOP_CustomBrushData> myData;
};
} // End HDK_Sample namespace
using namespace HDK_Sample;

SOP_UndoCustomBrushData::SOP_UndoCustomBrushData(
    SOP_CustomBrush *sop,
    GA_Size oldnumpts,
    GA_Size numpts,
    UT_Array<SOP_CustomBrushData> &olddata,
    UT_Array<SOP_CustomBrushData> &data) :
	mySopId(sop->getUniqueId()),
	myOldNumPts(oldnumpts),
	myNumPts(numpts),
	myOldData(olddata),
	myData(data)
{
    // include the memory allocated by the UT_RefArrays to accurately compute
    // the total memory used by this undo
    addToMemoryUsage(olddata.getMemoryUsage() + data.getMemoryUsage());
}

void
SOP_UndoCustomBrushData::undo()
{
    SOP_CustomBrush *node = (SOP_CustomBrush *)OP_Node::lookupNode(mySopId);
    node->updateData(myOldNumPts, myOldData);
}

void
SOP_UndoCustomBrushData::redo()
{
    SOP_CustomBrush *node = (SOP_CustomBrush *)OP_Node::lookupNode(mySopId);
    node->updateData(myNumPts, myData);
}

void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator("proto_custombrush", "Custom Brush",
				       SOP_CustomBrush::myConstructor,
				       SOP_CustomBrush::myTemplateList,
				       1, 1));
}

static PRM_Name sopOriginName("origin", "Origin");
static PRM_Name sopDirectionName("direction", "Direction");
static PRM_Name sopRadiusName("radius", "Radius");
static PRM_Name sopColorName("color", "Color");
static PRM_Name sopAlphaName("alpha", "Alpha");
static PRM_Name sopOperationName("operation", "Operation");
static PRM_Name sopEventName("event", "Event");
static PRM_Name sopClearAllName("clearall", "Clear All");

enum SOP_CustomBrushOperation
{
    SOP_CUSTOMBRUSHOPERATION_PAINT,
    SOP_CUSTOMBRUSHOPERATION_ERASE
};

static PRM_Name sopOperationMenuNames[] =
{
    PRM_Name("paint", "Paint"),
    PRM_Name("erase", "Erase"),
    PRM_Name(0)
};
static PRM_ChoiceList sopOperationMenu(PRM_CHOICELIST_SINGLE, sopOperationMenuNames);
static PRM_Default sopOperationDefault(SOP_CUSTOMBRUSHOPERATION_PAINT);

enum SOP_CustomBrushEvent
{
    SOP_CUSTOMBRUSHEVENT_BEGIN,
    SOP_CUSTOMBRUSHEVENT_ACTIVE,
    SOP_CUSTOMBRUSHEVENT_END,
    SOP_CUSTOMBRUSHEVENT_NOP
};

static PRM_Name sopEventMenuNames[] =
{
    PRM_Name("begin", "Begin Stroke"),
    PRM_Name("active", "Active Stroke"),
    PRM_Name("end", "End Stroke"),
    PRM_Name("nop", "No-op"),
    PRM_Name(0)
};
static PRM_ChoiceList sopEventMenu(PRM_CHOICELIST_SINGLE, sopEventMenuNames);
static PRM_Default sopEventDefault(SOP_CUSTOMBRUSHEVENT_NOP);

PRM_Template
SOP_CustomBrush::myTemplateList[] = {
    PRM_Template(PRM_STRING, 1, &PRMgroupName, 0, &SOP_Node::pointGroupMenu,
				0, 0, SOP_Node::getGroupSelectButton(
						GA_GROUP_POINT)),
    PRM_Template(PRM_XYZ_J, 3, &sopOriginName),
    PRM_Template(PRM_XYZ_J, 3, &sopDirectionName),
    PRM_Template(PRM_FLT_J, 1, &sopRadiusName, PRMoneDefaults),
    PRM_Template(PRM_RGB_J, 3, &sopColorName, PRMoneDefaults),
    PRM_Template(PRM_FLT_J, 1, &sopAlphaName, PRMpointOneDefaults),
    PRM_Template(PRM_ORD, 1, &sopOperationName, &sopOperationDefault,
			  &sopOperationMenu),
    PRM_Template(PRM_ORD, 1, &sopEventName, &sopEventDefault, &sopEventMenu),
    PRM_Template(PRM_CALLBACK, 1, &sopClearAllName, 0, 0, 0, &clearAllStatic),
    PRM_Template()
};

OP_Node *
SOP_CustomBrush::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_CustomBrush(net, name, op);
}

SOP_CustomBrush::SOP_CustomBrush(OP_Network *net, const char *name, OP_Operator *op) :
    SOP_Node(net, name, op),
    myGroup(0),
    myNumPts(0),
    myOldNumPts(0)
{
    // This indicates that this SOP manually manages its data IDs,
    // so that Houdini can identify what attributes may have changed,
    // e.g. to reduce work for the viewport, or other SOPs that
    // check whether data IDs have changed.
    // By default, (i.e. if this line weren't here), all data IDs
    // would be bumped after the SOP cook, to indicate that
    // everything might have changed.
    // If some data IDs don't get bumped properly, the viewport
    // may not update, or SOPs that check data IDs
    // may not cook correctly, so be *very* careful!
    mySopFlags.setManagesDataIDs(true);
}

SOP_CustomBrush::~SOP_CustomBrush()
{
}

OP_ERROR
SOP_CustomBrush::cookInputGroups(OP_Context &context, int alone)
{
    // The SOP_Node::cookInputPointGroups() provides a good default
    // implementation for just handling a point selection.
    return cookInputPointGroups(
        context, // This is needed for cooking the group parameter, and cooking the input if alone.
        myGroup, // The group (or NULL) is written to myGroup if not alone.
        alone,   // This is true iff called outside of cookMySop to update handles.
                 // true means the group will be for the input geometry.
                 // false means the group will be for gdp (the working/output geometry).
        true,    // (default) true means to set the selection to the group if not alone and the highlight flag is on.
        0,       // (default) Parameter index of the group field
        -1,      // (default) Parameter index of the group type field (-1 since there isn't one)
        true,    // (default) true means that a pointer to an existing group is okay; false means group is always new.
        false,   // (default) false means new groups should be unordered; true means new groups should be ordered.
        true,    // (default) true means that all new groups should be detached, so not owned by the detail;
                 //           false means that new point and primitive groups on gdp will be owned by gdp.
        0        // (default) Index of the input whose geometry the group will be made for if alone.
    );
}

OP_ERROR
SOP_CustomBrush::cookMySop(OP_Context &context)
{
    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    fpreal t = context.getTime();

    // make a copy of input 0's geometry if it different from our last
    // cook
    int changed_input;
    duplicateChangedSource(0, context, &changed_input);

    if (cookInputGroups(context) >= UT_ERROR_ABORT)
        return error();

    GA_Size npts = gdp->getNumPoints();

    if(myData.size() == 0)
    {
        // we have no paint yet, when we apply additional paint we will
        // require there to be 'npts' points
        myNumPts = npts;
    }
    else if(myNumPts != npts)
    {
        // we cannot apply the paint as the point numbers have changed
        addError(SOP_ERR_MISMATCH_POINT);
        return error();
    }

    int event = getEvent(t);
    if(event == SOP_CUSTOMBRUSHEVENT_BEGIN)
    {
        // we are starting a new brush stroke
        myOldData.setSize(0);
        myOldNumPts = myNumPts;
    }
    else if(event == SOP_CUSTOMBRUSHEVENT_ACTIVE)
    {
        // we are in the middle of performing a brush stroke
        UT_Vector3 origin = getOrigin(t);
        UT_Vector3 direction = getDirection(t);
        direction.normalize();
        fpreal radius = getRadius(t);
        fpreal radius2 = radius * radius;
        fpreal alpha = getBrushAlpha(t);
        UT_Vector3 color = getBrushColor(t);
        int operation = getOperation(t);

        // build lookup table from a point number to the paint applied to
        // that point
        UT_Map<GA_Index,exint> table;
        for(exint i = 0; i < myData.size(); ++i)
	    table[myData(i).myPtNum] = i;
        UT_Map<GA_Index,exint> oldtable;
        for(exint i = 0; i < myOldData.size(); ++i)
	    oldtable[myOldData(i).myPtNum] = i;

        GA_Offset ptoff;
        GA_FOR_ALL_GROUP_PTOFF(gdp, myGroup, ptoff)
        {
	    // determine if we should apply paint to this point

	    UT_Vector3 pos = gdp->getPos3(ptoff);

	    UT_Vector3 p = pos - origin;
	    p.normalize();
	    fpreal dot_p_dir = dot(p, direction);
	    if (dot_p_dir <= 0)
                continue;

            UT_Vector3 par = dot_p_dir * direction;
	    UT_Vector3 perp = p - par;

	    fpreal parlen2 = dot_p_dir * dot_p_dir;
	    if (parlen2 <= 0 || perp.length2() >= radius2 * parlen2)
                continue;
		
	    // we will apply paint to this point
	    GA_Index ptnum = gdp->pointIndex(ptoff);

	    // find the current amount of applied paint
            auto it = table.find(ptnum);
            exint index;
	    if (it == table.end())
	    {
	        // no paint has been applied yet
	        index = myData.append(SOP_CustomBrushData(ptnum, 0, 0, 0, 0));
	        table[ptnum] = index;
	    }
            else
                index = it->second;
	    SOP_CustomBrushData &d = myData(index);

            auto oldit = oldtable.find(ptnum);
	    if (oldit == oldtable.end())
	    {
	        // remember the old paint value for undos
	        index = myOldData.append(d);
	        oldtable[ptnum] = index;
	    }

	    // update the paint value
	    fpreal one_minus_alpha = 1 - alpha;
	    switch(operation)
	    {
	        case SOP_CUSTOMBRUSHOPERATION_PAINT:
		    d.myRed = alpha * color.x() + one_minus_alpha * d.myRed;
		    d.myGreen = alpha * color.y() + one_minus_alpha * d.myGreen;
		    d.myBlue = alpha * color.z() + one_minus_alpha * d.myBlue;
		    d.myAlpha = alpha + one_minus_alpha * d.myAlpha;
		    break;

	        case SOP_CUSTOMBRUSHOPERATION_ERASE:
		    d.myRed *= one_minus_alpha;
		    d.myGreen *= one_minus_alpha;
		    d.myBlue *= one_minus_alpha;
		    d.myAlpha *= one_minus_alpha;
		    break;
	    }
        }
    }
    else if(event == SOP_CUSTOMBRUSHEVENT_END)
    {
        // we have finished performing a brush stroke
        UT_UndoManager *man = UTgetUndoManager();
        if(man->willAcceptUndoAddition())
        {
	    // create an undo for the entire brush stroke
	    man->addToUndoBlock(new SOP_UndoCustomBrushData(this, myOldNumPts, myNumPts, myOldData, myData));
	    myOldData.setSize(0);
        }
    }

    // find the color attribute in the original geometry
    const GU_Detail *input0 = inputGeo(0);
    GA_ROHandleV3 input_handle(input0->findFloatTuple(GA_ATTRIB_POINT,
				        GEO_STD_ATTRIB_DIFFUSE, 3));

    // find the color attribute in the current geometry, creating one
    // if necessary
    GA_RWHandleV3 handle(gdp->findFloatTuple(GA_ATTRIB_POINT,
				        GEO_STD_ATTRIB_DIFFUSE, 3));
    if (!handle.isValid())
    {
        handle = GA_RWHandleV3(gdp->addFloatTuple(GA_ATTRIB_POINT,
				        GEO_STD_ATTRIB_DIFFUSE, 3));
    }

    // update the colour for all painted points
    for (exint i = 0; i < myData.size(); ++i)
    {
        SOP_CustomBrushData &data = myData(i);

        fpreal r = data.myRed;
        fpreal g = data.myGreen;
        fpreal b = data.myBlue;
        if (input_handle.isValid())
        {
            fpreal one_minus_alpha = 1 - data.myAlpha;
            GA_Offset ptoff = input0->pointOffset(data.myPtNum);
            UT_Vector3 f = input_handle.get(ptoff);
            r += one_minus_alpha * f.x();
            g += one_minus_alpha * f.y();
            b += one_minus_alpha * f.z();
        }

        GA_Offset ptoff = gdp->pointOffset(data.myPtNum);
        handle.set(ptoff, UT_Vector3(r, g, b));
    }

    if (myData.size() > 0)
        handle.bumpDataId();

    return error();
}

OP_ERROR
SOP_CustomBrush::save(
    std::ostream &os,
    const OP_SaveFlags &saveflags,
    const char *path_prefix)
{
    if(SOP_Node::save(os, saveflags, path_prefix) >= UT_ERROR_ABORT)
	return error();

    // create a new packet for our paint
    UT_CPIO packet;
    UT_WorkBuffer path;
    const char *ext = saveflags.getBinary() ? "bpaint" : "paint";
    path.sprintf("%s%s.%s", path_prefix, (const char *)getName(), ext);
    packet.open(os, path.buffer());
    {
	UT_OStream out(os, saveflags.getBinary());

	out.write(&myNumPts, 1, true);

	exint n = myData.size();
	out.write(&n, 1, true);

	for (exint i = 0; i < n; ++i)
	{
	    SOP_CustomBrushData &data = myData(i);
	    out.write((GA_Size *)&data.myPtNum);
	    out.write<fpreal32>(&data.myRed);
	    out.write<fpreal32>(&data.myGreen);
	    out.write<fpreal32>(&data.myBlue);
	    out.write<fpreal32>(&data.myAlpha, 1, (i == n - 1));
	}
    }
    packet.close(os);

    return error();
}

bool
SOP_CustomBrush::load(UT_IStream &is, const char *ext, const char *path)
{
    // update our paint values if this is a paint packet
    if(strcmp(ext, "bpaint") == 0 || strcmp(ext, "paint") == 0)
    {
	myNumPts = 0;
	myData.setSize(0);
	myOldData.setSize(0);

	if(!is.read(&myNumPts))
	    return false;

	exint n;
	if(!is.read(&n))
	    return false;
	for(exint i = 0; i < n; ++i)
	{
	    GA_Size idx;
	    if(!is.read(&idx))
		return false;
	
	    float r;
	    if(!is.read<fpreal32>(&r))
		return false;

	    float g;
	    if(!is.read<fpreal32>(&g))
		return false;

	    float b;
	    if(!is.read<fpreal32>(&b))
		return false;

	    float a;
	    if(!is.read<fpreal32>(&a))
		return false;

	    myData.append(SOP_CustomBrushData(idx, r, g, b, a));
	}

	return true;
    }

    return SOP_Node::load(is, ext, path);
}

void
SOP_CustomBrush::updateData(exint numpts, UT_Array<SOP_CustomBrushData> &data)
{
    myNumPts = numpts;
    if (myNumPts == 0)
    {
	myData.setSize(0);

	// we make the SOP think the input geometry has changed so it will
	// duplicate the input geometry to reset the attribute values.  This
	// is necessary as our cook method only modified the color attribute
	// of points indicated by myData, which is now empty.
	resetChangedSourceFlags();
    }
    else
    {
	// build lookup table from a point number to the paint applied to
	// that point
	UT_Map<GA_Index,exint> table;
	for (exint i = 0; i < myData.size(); ++i)
	    table[myData(i).myPtNum] = i;

	for (exint i = 0; i < data.size(); ++i)
	{
	    SOP_CustomBrushData &d = data(i);

            UT_Map<GA_Index,exint>::iterator it = table.find(d.myPtNum);
	    if(it != table.end())
	    {
		// we already have paint applied to this point, just update the
		// paint values
		myData(it->second) = d;
	    }
	    else
	    {
		// create a new entry for the paint
		exint index = myData.append(d);
		table[d.myPtNum] = index;
	    }
	}
    }

    // tell our SOP to re-cook as we have changed the paint values
    forceRecook();
}

int
SOP_CustomBrush::clearAllStatic(void *op, int, fpreal, const PRM_Template *)
{
    SOP_CustomBrush *sop = (SOP_CustomBrush *)op;
    sop->clearAll();
    return 1;
}

void
SOP_CustomBrush::clearAll()
{
    UT_AutoUndoBlock undoblock("Clear All", ANYLEVEL);

    GA_Size oldnumpts = myNumPts;
    myNumPts = 0;
    myOldData = myData;
    myData.setSize(0);

    UT_UndoManager *man = UTgetUndoManager();
    if (man->willAcceptUndoAddition())
    {
	man->addToUndoBlock(new SOP_UndoCustomBrushData(this, oldnumpts, myNumPts, myOldData, myData));
	myOldData.setSize(0);
    }

    // we make the SOP think the input geometry has changed so it will
    // duplicate the input geometry to reset the attribute values.  This is
    // necessary as our cook method only modified the color attribute of
    // points indicated by myData, which is now empty.
    resetChangedSourceFlags();
    forceRecook();
}
