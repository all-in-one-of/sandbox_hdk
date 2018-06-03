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
 * This is an example of an implementation of the Object Merge SOP to
 * demonstrate using DYNAMIC_PATHs, MULTIPARMS, and proper cooking of other
 * SOPs.
 */

#include "SOP_HDKObject.h"

#include <GU/GU_Detail.h>
#include <OP/OP_Director.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <PRM/PRM_Parm.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_DirUtil.h>

using namespace HDK_Sample;

// Define the new sop operator...
void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        "proto_objectmerge",
        "HDK Object Merge",
        SOP_HDKObject::myConstructor,
        SOP_HDKObject::myTemplateList,
        0, 0, 0, OP_FLAG_GENERATOR));
}

static PRM_Name objnames[] =
{
    PRM_Name("numobj",	"Number of Objects"),
    PRM_Name("objpath#","Object #"),
    PRM_Name("xformpath","Transform Object"),
    PRM_Name("enable#", "Enable Merge #"),
    PRM_Name(0)
};

static PRM_Template	theObjectTemplates[] = {
    PRM_Template(PRM_TOGGLE, 1, &objnames[3], PRMoneDefaults),
    PRM_Template(PRM_STRING,	PRM_TYPE_DYNAMIC_PATH, 1,
		 &objnames[1], 0, 0,
		 0, 0, &PRM_SpareData::sopPath),
    PRM_Template()
};

PRM_Template
SOP_HDKObject::myTemplateList[] = 
{
    PRM_Template(PRM_MULTITYPE_LIST, theObjectTemplates, 2, &objnames[0],
		    PRMoneDefaults),
    PRM_Template(PRM_STRING, PRM_TYPE_DYNAMIC_PATH, 1, &objnames[2],
		0, 0, 0, 0, &PRM_SpareData::objPath),
    PRM_Template()
};

OP_Node *
SOP_HDKObject::myConstructor(OP_Network *net, const char *name,
			    OP_Operator *entry)
{
    return new SOP_HDKObject(net, name, entry);
}


SOP_HDKObject::SOP_HDKObject(OP_Network *net, const char *name, OP_Operator *entry)
    : SOP_Node(net, name, entry)
{
    // NOTE: GEO_Detail::copy(), when GEO_COPY_ONCE is used, will copy all
    //       data IDs from the source, and GEO_Detail::transform() will
    //       bump data IDs of any transformed attributes, as well as the
    //       primitive list data ID if there are any transforming primitives
    //       that were transformed.
    mySopFlags.setManagesDataIDs(true);
}
SOP_HDKObject::~SOP_HDKObject() {}

bool
SOP_HDKObject::updateParmsFlags()
{
    bool changed = false;

    int n = NUMOBJ();
    for (int i = 1; i <= n; i++)
    {
        changed |= enableParmInst(objnames[1].getToken(), &i, ENABLEMERGE(i));
    }

    return changed;
}

int
SOP_HDKObject::getDandROpsEqual()
{
    // don't do anything if we're locked
    if (flags().getHardLocked())
        return 1;

    int numobj = NUMOBJ();

    // Determine if any of our SOPs are evil.
    for (int objindex = 1; objindex <= numobj; objindex++)
    {
        if (!ENABLEMERGE(objindex)) // Ignore disabled ones.
            continue;

        UT_String objname;
        SOPPATH(objname, objindex, 0.0f);

        OP_Network *objptr = (OP_Network *)findNode((const char *)objname);
        if (!objptr)
            continue;

        // Self-referential nodes are assumed to have equal DandR ops,
        // as it doesn't matter as they will be ignored and flagged
        // as errors during cook anyways.
        if (objptr == this)
        {
            continue;
        }

        if (!objptr->getDandROpsEqual())
        {
            return 0;
        }
    }
    // None of our interests was unequal, thus we are equal!
    return 1;
}

OP_ERROR
SOP_HDKObject::cookMySop(OP_Context &context)
{
    fpreal t = context.getTime();

    // Get our xform object, if any.
    UT_String objname;
    XFORMPATH(objname, t);
    OP_Network *xformobjptr = (OP_Network *)findNode(objname);
    if (xformobjptr && xformobjptr->getOpTypeID() == SOP_OPTYPE_ID)
    {
        // The user pointed to the SOP.  We silently promote it to the
        // containing object.  This allows the intuitive "." to be used
        // for the path to transform relative to our own op (rather than
        // having to track up an arbitrary number of paths)
        xformobjptr = xformobjptr->getCreator();
    }

    // We must explicitly cast down as OBJ_Node * is unknown here.
    // We also do the cast here instead of in findNode as we want
    // to allow people to reference SOPs.
    xformobjptr = (OP_Network *)CAST_OBJNODE(xformobjptr);

    if (!xformobjptr && objname.isstring())
    {
        // We didn't get an xform object, but they had something typed,
        // badmerge.
        addError(SOP_BAD_SOP_MERGED, objname);
    }

    int numobj = NUMOBJ();

    // Peek as to whether we are a render cook or not.
    int cookrender = getCreator()->isCookingRender();

    bool copiedfirst = false;
    bool copiedlast = false;

    for (int objindex = 1; objindex <= numobj; objindex++)
    {
        if (!ENABLEMERGE(objindex))             // Ignore disabled ones.
            continue;

        UT_String sopname;
        SOPPATH(sopname, objindex, t);

        if (!sopname.isstring())
            continue;                           // Blank means ignore.

        SOP_Node *sopptr = getSOPNode(sopname, 1); // We want extra inputs.

        if (sopptr == this)
        {
            // Self-reference.  Special brand of evil.
            addWarning(SOP_ERR_SELFMERGE);
            continue;
        }

        if (!sopptr)
        {
            // Illegal merge.  Just warn so we don't abort everything.
            addWarning(SOP_BAD_SOP_MERGED, sopname);
            continue;
        }

        // Get the creator, which is our objptr.
        OP_Network *objptr = sopptr->getCreator();

        // Change over so any subnet evaluation will properly track...
        int savecookrender = objptr->isCookingRender();
        objptr->setCookingRender(cookrender);

        // Actually cook...
        const GU_Detail *cookedgdp = sopptr->getCookedGeo(context);

        // Restore the cooking render state.
        objptr->setCookingRender(savecookrender);

        if (!cookedgdp)
        {
            // Something went wrong with the cooking. Warn the hapless user.
            addWarning(SOP_BAD_SOP_MERGED, sopname);
            continue;
        }

        // Now add the extra inputs...
        addExtraInput(objptr, OP_INTEREST_DATA);
        // The sop extra inputs were set by the getSOPNode

        bool firstmerge = !copiedfirst;

        // Choose the best copy method we can
        GEO_CopyMethod copymethod = GEO_COPY_ADD;
        if (!copiedfirst)
        {
            copymethod = GEO_COPY_START;
            copiedfirst = true;
            if (objindex == numobj)
            {
                copymethod = GEO_COPY_ONCE;
                copiedlast = true;
            }
        }
        else if (objindex == numobj)
        {
            copymethod = GEO_COPY_END;
            copiedlast = true;
        }

        // Mark where the new prims and points start
        GA_IndexMap::Marker pointmarker(gdp->getPointMap());
        GA_IndexMap::Marker primmarker(gdp->getPrimitiveMap());

        // Don't copy internal groups!
        // Accumulation of internal groups may ensue.
        gdp->copy(*cookedgdp, copymethod, true, false, GA_DATA_ID_CLONE);

        // Apply the transform.
        if (xformobjptr)
        {
            // GEO_Detail::transform supports double-precision,
            // so we might as well use double-precision transforms.
            UT_Matrix4D xform, xform2;

            if (!objptr->getWorldTransform(xform, context))
                addTransformError(*objptr, "world");
            if (!xformobjptr->getIWorldTransform(xform2, context))
                addTransformError(*xformobjptr, "inverse world");
            xform *= xform2;

            if (firstmerge)
            {
                // The first object we merge we can just do a full gdp transform
                // rather than building the subgroup.
                gdp->transform(xform);
            }
            else
            {
                gdp->transform(xform, primmarker.getRange(), pointmarker.getRange(), false);
            }
        }
    }

    if (xformobjptr)
    {
        addExtraInput(xformobjptr, OP_INTEREST_DATA);
    }

    // Finish & clean up the copy procedure, if not already done.
    if (!copiedfirst)
    {
        gdp->clearAndDestroy();
    }
    else if (!copiedlast)
    {
        GU_Detail blank_gdp;
        gdp->copy(blank_gdp, GEO_COPY_END, true, false, GA_DATA_ID_CLONE);
    }

    // Set the node selection for this primitive. This will highlight all
    // the primitives of the node, but only if the highlight flag for this node
    // is on and the node is selected.
    if (error() < UT_ERROR_ABORT)
	select(GA_GROUP_PRIMITIVE);

    return error();
}
