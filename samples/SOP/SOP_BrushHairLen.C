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
 * This SOP demonstrates how to override the BrushBase SOP to paint custom
 * attributes, and then use those attributes in the resulting geometry.
 */

#include "SOP_BrushHairLen.h"

#include <SOP/SOP_Guide.h>
#include <GU/GU_Detail.h>
#include <GEO/GEO_PolyCounts.h>
#include <GEO/GEO_PrimPoly.h>
#include <GA/GA_ElementWrangler.h>
#include <GA/GA_Iterator.h>
#include <GA/GA_Range.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_ChoiceList.h>
#include <UT/UT_DSOVersion.h>

using namespace HDK_Sample;

#define PRM_MENU_CHOICES	(PRM_ChoiceListType)(PRM_CHOICELIST_EXCLUSIVE |\
						     PRM_CHOICELIST_REPLACE)

// Define the new sop operator...
void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
    "proto_brushhairlen",
    "Brush Hair Length",
    SOP_BrushHairLen::myConstructor,
    SOP_BrushHairLen::myTemplateList,
    1,
    1));
}

static PRM_Name	sop_names[] = {
    PRM_Name("group", "Group"),
    PRM_Name("op", "Operation"),
    PRM_Name("flen", "FL"),
    PRM_Name("blen", "BL"),
    PRM_Name("radius", "Radius"),
    PRM_Name("uvradius", "UV Radius"),
    PRM_Name(0)
};

static PRM_Name		sopOpMenuNames[] = {
    PRM_Name("paint",	"Paint"),
    PRM_Name("eyedrop",	"Eye Dropper"),
    PRM_Name("smoothattrib", "Smooth"),
    PRM_Name("callback", "Callback"),
    PRM_Name("erase",	"Erase Changes"),
    PRM_Name(0)
};
static PRM_ChoiceList	sopOpMenu(PRM_MENU_CHOICES,	sopOpMenuNames);

PRM_Template
SOP_BrushHairLen::myTemplateList[]=
{
    // Primitive group to allow painting on...
    PRM_Template(PRM_STRING,	1, &sop_names[0], 0, &SOP_Node::primGroupMenu,
				0, 0, SOP_Node::getGroupSelectButton(
						GA_GROUP_PRIMITIVE)),

    // This is the choice of operations...
    PRM_Template((PRM_Type) PRM_ORD,
		    PRM_Template::PRM_EXPORT_MAX,
		    1, &sop_names[1], 0, &sopOpMenu),
    // Foreground hair length (LMB)
    PRM_Template(PRM_FLT_J, PRM_Template::PRM_EXPORT_TBX,
		    1, &sop_names[2], PRMoneDefaults),
    // Background hair length (MMB)
    PRM_Template(PRM_FLT_J, PRM_Template::PRM_EXPORT_TBX,
		    1, &sop_names[3], PRMzeroDefaults),
    // Radius
    PRM_Template(PRM_FLT_J, PRM_Template::PRM_EXPORT_TBX,
		    1, &sop_names[4], PRMpointOneDefaults),
    // UV Radius
    PRM_Template(PRM_FLT_J, PRM_Template::PRM_EXPORT_TBX,
		    1, &sop_names[5], PRMpointOneDefaults),
    PRM_Template()
};

OP_Node *
SOP_BrushHairLen::myConstructor(OP_Network *net,const char *name,OP_Operator *entry)
{
    return new SOP_BrushHairLen(net, name, entry);
}


SOP_BrushHairLen::SOP_BrushHairLen(OP_Network *net, const char *name, OP_Operator *entry)
    : SOP_BrushBase(net, name, entry)
{
    // This indicates that this SOP manually manages its data IDs,
    // so that Houdini can identify what attributes may have changed,
    // e.g. to reduce work for the viewport, or other SOPs that
    // check whether data IDs have changed.
    // By default, (i.e. if this line weren't here and if not for SOP_GDT),
    // all data IDs would be bumped after the SOP cook, to indicate that
    // everything might have changed.
    // If some data IDs don't get bumped properly, the viewport
    // may not update, or SOPs that check data IDs
    // may not cook correctly, so be *very* careful!
    // NOTE: SOP_GDT already set this, so all subclasses have it set
    //       by default, unless they set it to false,
    //       but it's here for clarity.
    mySopFlags.setManagesDataIDs(true);

    // We initialize our values to safe starting values.  The most important
    // is setting myEvent to SOP_BRUSHSTROKE_NOP as that will cause
    // the processBrushOp to ignore most everything else.
    myRayOrient = 0.0f;
    myRayHit = 0.0f;
    myRayHitU = 0.0f;
    myRayHitV = 0.0f;
    myRayHitW = 0.0f;
    myRayHitPressure = 1.0f;
    myPrimHit = -1;
    myEvent = SOP_BRUSHSTROKE_NOP;
    myUseFore = true;
    myStrokeChanged = false;
}

SOP_BrushHairLen::~SOP_BrushHairLen()
{
}


SOP_BrushOp
SOP_BrushHairLen::OP()
{
    switch (evalInt("op", 0, 0))
    {
	case 0: return SOP_BRUSHOP_PAINT;
	case 1: return SOP_BRUSHOP_EYEDROP;
	case 2: return SOP_BRUSHOP_SMOOTHATTRIB;
	case 3: return SOP_BRUSHOP_CALLBACK;
	case 4: default: return SOP_BRUSHOP_ERASE;
    }
}

void
SOP_BrushHairLen::setBrushOp(SOP_BrushOp op)
{
    int iop;
    switch (op)
    {
	case SOP_BRUSHOP_EYEDROP:	iop = 1; break;
	case SOP_BRUSHOP_SMOOTHATTRIB:	iop = 2; break;
	case SOP_BRUSHOP_CALLBACK:	iop = 3; break;
	case SOP_BRUSHOP_ERASE:		iop = 4; break;
	case SOP_BRUSHOP_PAINT:
	default:			iop = 0; break;
    }
    setInt("op", 0, 0, iop);
}

void
SOP_BrushHairLen::doErase()
{
    // We want to do attribute erase, as we are an attribute style brush.
    // NOTE: Both of these operations bump the affected attributes' data IDs.
    myBrush.eraseAttributes(myPermanentDelta, myCurrentDelta);
    if (myBrush.doVisualize())
	myBrush.applyVisualizeStencil(gdp);
}

bool
SOP_BrushHairLen::hasStyleChanged(fpreal t)
{
    // When do we have to apply a new sculpting operation:
    return	isParmDirty(1, t) ||
		isParmDirty(2, t);
}

const GU_Detail *
SOP_BrushHairLen::getIsectGdp(fpreal t)
{
    OP_Context context(t);

    // We always want our first input...  We change our own topology,
    // so it would be a bad thing to use ourselves.
    SOP_Node *sop = CAST_SOPNODE(getInput(0));

    return sop->getCookedGeo(context);
}

OP_ERROR
SOP_BrushHairLen::cookMySop(OP_Context &context)
{
    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    fpreal t = context.getTime();

    // There are two different methods here.  BUILD_HAIR will create
    // hair geometry in the gdp.  This requires it to do a duplicateSource
    // and rebuild everything every frame.
    // The non BUILD_HAIR method merely updates the hairlen point attribute.
    // One could then use the guide geometry to display the hair.  This is
    // more efficient, as the brush code can avoid duplicating the incoming
    // geometry, but just rollback its changes.  This method should be
    // used if you are not doing any processing of the gdp post-processBrushOp.
    const bool BUILD_HAIR = false;

    bool changed_input;
    bool changed_group;
    if (BUILD_HAIR)
    {
        changed_input = true;
        changed_group = true;
        // Duplicate the incoming source, overwriting everything as we will
        // be messing with geometry.
        duplicateSource(0, context);
    }
    else
    {
        changed_input = checkChangedSource(0, context);
        changed_group = isParmDirty(SOP_GDT_GRP_IDX, context.getTime());

        if (changed_input)
            duplicateChangedSource(0, context, 0, true);
    }

    // Find the hairlen attribute...
    GA_RWHandleF attrib(gdp->findFloatTuple(GA_ATTRIB_POINT, "hairlen"));

    // If it doesn't exist, create it.
    if (attrib.isInvalid())
        attrib = GA_RWHandleF(gdp->addFloatTuple(GA_ATTRIB_POINT, "hairlen", 1));

    // Having created the attribute, we can also create a local variable
    // HAIRLEN which will map to it:
    // NOTE: This bumps the data ID of the varmap attribute.
    gdp->addVariableName("hairlen", "HAIRLEN");

    // Default to false to trigger a findFloatTuple if necessary in the callback.
    myHairlenFound = false;
    myTime = t;

    // Now, process any of the brush changes that may have occurred since
    // our last cook...
    // We inform it that we have changed both the input & group, as it
    // should not rely on them as we have rebuilt them.
    processBrushOp(context, changed_input, changed_group);

    // Bump the data ID for hairlen if it was modified in the callback
    // below.  Bumping the data ID once for each point might be slow,
    // so it's done just once here.
    if (myHairlenFound)
        attrib.bumpDataId();

    // We now clear out our myStrokeChanged as it is no longer changed...
    myStrokeChanged = false;

    // For each point, add a hair of the proper length, if there are any.
    if (BUILD_HAIR && gdp->getNumPoints() > 0)
    {
        GA_Size n = gdp->getNumPoints();
        GA_Offset startnewptoff = gdp->appendPointBlock(n);

        // We've added points, so all point attribute data IDs must be bumped.
        gdp->getAttributes().bumpAllDataIds(GA_ATTRIB_POINT);

        // We want to copy all standard attributes (except P) and groups
        GA_AttributeFilter filter = GA_AttributeFilter::selectOr(
            GA_AttributeFilter::selectStandard(gdp->getP()),
            GA_AttributeFilter::selectGroup());
        GA_PointWrangler ptwrangler(*gdp, filter);

        // GEO_PrimPoly::buildBlock takes an array of integers that are
        // really offsets relative to some lower-bound offset.  In this case,
        // it's fine to just have a lower-bound of GA_Offset(0), even if
        // that offset isn't occupied, but we could use
        // gdp->pointOffset(GA_Index(0)) to have a tigher bound in
        // some cases where the input wasn't defragmented.
        // It mostly helps in cases where the span of points used by the
        // polygons is very small compared to the total.
        GA_Offset relativetooffset = GA_Offset(0);

        GEO_PolyCounts polygonsizes;
        polygonsizes.append(2, n);
        UT_IntArray polygonpointnumbers(2*n, 2*n);
        exint i = 0;
        for (GA_Iterator it(GA_Range(gdp->getPointMap(),GA_Offset(0),startnewptoff)); !it.atEnd(); ++it, ++i)
        {
            GA_Offset oldptoff = *it;
            // appendPointBlock guarantees a contiguous block of offsets, so we can just add i.
            GA_Offset newptoff = startnewptoff + i;
            UT_Vector3 pos = gdp->getPos3(oldptoff);
            // Add hair length to y value.
            pos.y() += attrib.get(oldptoff);
            gdp->setPos3(newptoff, pos);

            // Copy attributes (except P) and groups
            if (ptwrangler.getNumAttributes() > 0)
                ptwrangler.copyAttributeValues(newptoff, oldptoff);

            // Create a polygon to loft them.
            polygonpointnumbers(2*i    ) = int(oldptoff - relativetooffset);
            polygonpointnumbers(2*i + 1) = int(newptoff - relativetooffset);
        }

        // Build the actual polygons.  This will be in parallel if there are enough.
        // npoints just needs to be an upper bound on the maximum offset used + 1 - relative offset.
        GEO_PrimPoly::buildBlock(gdp, relativetooffset, gdp->getNumPointOffsets() - relativetooffset, polygonsizes, polygonpointnumbers.array(), false);

        // We've added primitives and vertices, so all primitive and
        // vertex attribute data IDs must be bumped.
        gdp->getAttributes().bumpAllDataIds(GA_ATTRIB_PRIMITIVE);
        gdp->getAttributes().bumpAllDataIds(GA_ATTRIB_VERTEX);

        // The primitive list's data ID also needs to be bumped.
        gdp->getPrimitiveList().bumpDataId();
    }

    return error();
}

void
SOP_BrushHairLen::brushOpCallback(
    GA_Offset ptoff,
    const UT_Array<GA_Offset> * /*ptneighbour*/,
    GA_Offset /*vtx*/,
    const UT_Array<GA_Offset> * /*vtxneighbour*/,
    float alpha,
    GEO_Delta *delta)
{
    // Unused here is the ptneighbour and vtxneighbour.  These are a list
    // of all the points or vertices connected to this point by at least
    // one edge.  Each point will show up only once in the list, regardless
    // of the number of times it is connected.
    
    // We first determine the attribute index if not already known.
    // This is called once per point, so we want to minimize the attribute
    // lookups, but on the other hand, we don't want to cache to early
    // as if new attributes are created it would be invalid.
    if (!myHairlenFound)
    {
        myHairlenFound = true;
        myHairlenHandle = GA_RWHandleF(gdp->findFloatTuple(GA_ATTRIB_POINT, "hairlen"));
    }

    // If no hairlen, do nothing.
    if (myHairlenHandle.isInvalid())
        return;

    // Here we actually change all our attributes.  Note that we should:
    // 1) NOT create any new attributes in here, as it will confuse the GDT.
    // 2) Open & close the GDT for writing using beginPointAttributeChange
    //    or beginPointPositionChanged followed by endChange().
    // 3) Use the alpha as a blend value for our effect.
    // 4) One of point or vertex will be non-null, depending on if this
    //    is a vertex paint or point paint.  Currently only point paint
    //    is supported.

    float newhair = (myUseFore ? FGR(myTime) : BGR(myTime));

    if (GAisValid(ptoff))
    {
        if (delta) delta->beginPointAttributeChange(*gdp, ptoff);

        // Do all our attribute tweaking here...
        float oldhair = myHairlenHandle.get(ptoff);

        // simple alpha blending...  Alpha of 1 means newhair, 0 means oldhair.
        myHairlenHandle.set(ptoff, SYSlerp(oldhair, newhair, alpha));

        if (delta) delta->endChange();
    }
}
