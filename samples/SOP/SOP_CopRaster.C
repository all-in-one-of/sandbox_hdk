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
 * This SOP reads a raster image and generates a point from each pixel in the
 * image.  The point is colored based on the pixel value of the image.
 *              
 * NOTE:  You must turn on the "Points" display option (this is not on by
 * default) to see the generated points in the Houdini viewport.  Because
 * Houdini does not use point colors when it displays points, you will need to
 * attach another SOP (for example, the Particle SOP) to see the effect of the
 * colored pixels. 
 */

#include "SOP_CopRaster.h"

#include <GU/GU_Detail.h>
#include <OP/OP_Director.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <TIL/TIL_CopResolver.h>
#include <TIL/TIL_Raster.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_DSOVersion.h>
#include <stdio.h>

using namespace HDK_Sample;

static PRM_Name copnames[] = {
    PRM_Name("usedisk",     "Use Disk Image"),
    PRM_Name("copframe",    "COP Frame"),
    PRM_Name("file",        "File Name"),
    PRM_Name("copcolor",    "Plane"),
    PRM_Name("coppath",     "COP Path"),
};

static PRM_Default copFrameDefault(0, "$F");


static PRM_ChoiceList colorMenu((PRM_ChoiceListType)
    (PRM_CHOICELIST_EXCLUSIVE |
        PRM_CHOICELIST_REPLACE),
    &SOP_CopRaster::buildColorMenu);

static PRM_Default fileDef(0, "circle.pic");
static PRM_Default colorDef(0, TIL_DEFAULT_COLOR_PLANE);

PRM_Template
SOP_CopRaster::myTemplateList[] = {
    PRM_Template(PRM_TOGGLE,    1, &copnames[0], PRMoneDefaults),
    PRM_Template(PRM_STRING,    PRM_TYPE_DYNAMIC_PATH, 1, &copnames[4],
                            0, 0, 0, 0, &PRM_SpareData::cop2Path),
    PRM_Template(PRM_STRING,    1, &copnames[3], &colorDef, &colorMenu),
    PRM_Template(PRM_FLT_J,     1, &copnames[1], &copFrameDefault),
    PRM_Template(PRM_PICFILE,   1, &copnames[2], &fileDef,
                            0, 0, 0, &PRM_SpareData::fileChooserModeRead),

    PRM_Template()
};

OP_Node *
SOP_CopRaster::myConstructor(OP_Network *dad, const char *name, OP_Operator *op)
{
    return new SOP_CopRaster(dad, name, op);
}

SOP_CopRaster::SOP_CopRaster(OP_Network *dad, const char *name, OP_Operator *op)
    : SOP_Node(dad, name, op)
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

SOP_CopRaster::~SOP_CopRaster() {}

bool
SOP_CopRaster::updateParmsFlags()
{
    int state;
    bool changed = SOP_Node::updateParmsFlags();

    // Here, we disable parameters which we don't care about...
    state = (USEDISK()) ? 0 : 1;
    changed |= enableParm("coppath", state);
    changed |= enableParm("copcolor", state);
    changed |= enableParm("copframe", state);
    changed |= enableParm("file", !state);

    return changed;
}

//
//  This is a static method which builds a menu of all the planes in the COP.
//
void
SOP_CopRaster::buildColorMenu(void *data, PRM_Name *theMenu, int theMaxSize,
			      const PRM_SpareData *, const PRM_Parm *)
{
    SOP_CopRaster	*me = (SOP_CopRaster *)data;
    UT_ValArray<char *>	 items;
    UT_String		 relpath, fullpath, netpath, nodepath;
    int			 i, useflag = 0;

    me->COPPATH(relpath, 0.0F);
    me->getFullCOP2Path(relpath, fullpath, useflag);
    me->splitCOP2Path(fullpath, netpath, nodepath);

    TIL_CopResolver::buildColorMenu(netpath, nodepath, items);

    for (i = 0; i < items.entries() && i < theMaxSize; i++)
    {
	theMenu[i].setToken( items(i) );
	theMenu[i].setLabel( items(i) );

	free ( items(i) );
    }
    theMenu[i].setToken(0);	// Need a null terminater
}

void
SOP_CopRaster::splitCOP2Path(const char *path, UT_String &net,
			     UT_String &node)
{
    // We split the path into the network and node portion.
    OP_Node	*node_ptr, *parent_ptr;
    UT_String	 fullpath;

    node_ptr = findNode(path);
    if (!node_ptr)	// Failed to find the node
    {
	net = "";
	node = "";
	return;
    }

    parent_ptr = node_ptr->getCreator();
    if (!parent_ptr)
	net = "";
    else
	parent_ptr->getFullPath(net);

    node_ptr->getFullPath(fullpath);
    if (net.isstring())
    {
	// The relative path from the net to the fullpath is our node path.
	node.getRelativePath(net, fullpath);
    }
    else
	node.harden(fullpath);
}

// This builds an absolute path out of the provided relative path by
// expanding to the node and doing a getFullPath.
// It also changes net into nodes by diving into the render pointer.
int
SOP_CopRaster::getFullCOP2Path(const char *relpath, UT_String &fullpath,
			       int &flagdependent)
{
    OP_Node	*node;

    fullpath = "";
    flagdependent = 0;

    node = findNode(relpath);
    if (!node)
	return -1;

    if (node->getOpTypeID() != COP2_OPTYPE_ID)
    {
	// Not the right type.  Check to see if its child is the right type.
	// If so, get the render pointer...
	if (((OP_Network *)node)->getChildTypeID() == COP2_OPTYPE_ID)
	{
	    node = ((OP_Network *)node)->getRenderNodePtr();
	    flagdependent = 1;
	}
    }

    // The following call will return NULL if this is not a COP2 node.
    node = (OP_Node *) CAST_COP2NODE(node);
    if (!node)
	return -1;

    node->getFullPath(fullpath);

    // Success!
    return 0;
}

//
//  Here's the method which will update our raster.  It will load from a
//	disk file or from the specified COP.
//
//  Returns: 1 if new raster, 0 if old raster, -1 if no raster
//
int
SOP_CopRaster::updateRaster(fpreal t)
{
    UT_String		fname;
    int			rcode;

    // We don't have to do this, but for the example, we only care about 8
    //	bit rasters.
    myRaster.setRasterDepth(myRaster.UT_RASTER_8);

    rcode = -1;
    if (USEDISK())
    {
	// Loading from a disk is easy.  We simply do so.
	FNAME(fname, t);
    	if (myCurrentName == fname)
	{
	    rcode = 0;
	}
	else
	{
	    if (!myRaster.load(fname))
	    {
		addCommonError(UT_CE_FILE_ERROR, (const char *)fname);
		rcode = -1;
	    }
	    else
	    {
		myCurrentName.harden(fname);
		rcode = 1;
	    }
	}
    }
    else
    {
	UT_String	 relpath, fullpath;
	OP_Node		*cop = 0;
	int		 id, useflag = 0;

	// We need a cop resolver to be able to grab the raster from the node.
	TIL_CopResolver	*cr = TIL_CopResolver::getResolver();

	// Clear out the filename, so that if the user changes our method,
	//	we will reload the file from disk...
	myCurrentName.harden("");
	COPPATH(relpath, t);	// Find the relative path to the node
	getFullCOP2Path(relpath, fullpath, useflag);

	// We use the cop resolver to find the unique ID of the node, given
	// the full path.  This ID can then be used to get at the node.
	id = TIL_CopResolver::getNodeId(fullpath);
	if (id >= 0)
	    cop = OP_Node::lookupNode(id);

        if (cop)
        {
	    TIL_Raster	*r = 0;
	    float	 frame;
	    UT_String	 cplane;

	    // Here we have a valid COP.  Now, we have to add interests in
	    // the COP.  For example, if it re-cooks, ir the COP parameters
	    // change, we have to know about it...
	    addExtraInput(cop, OP_INTEREST_DATA);

	    // If we used the flag to resolve this cop, we need to know
	    // if that flag changes as well.
	    if (useflag)
		addExtraInput(cop, OP_INTEREST_FLAG);

	    frame = COPFRAME(t);
	    CPLANE(cplane, t);

	    r = cr->getNodeRaster(fullpath, cplane, TIL_NO_PLANE, true,
				  (int)frame, TILE_INT8);

	    // Now we need to make a local copy of this raster.
	    if (r)
	    {
		myRaster.size((short) r->getXres(), (short) r->getYres());
		memcpy(myRaster.getRaster(), r->getPixels(), r->getSize());
		rcode = 1;
	    }
	    else
		rcode = -1;

	    // One last thing.  If the COP is time dependent or dependent on
	    // channels for cooking, we have to flag ourselves as time
	    // dependent.  Otherwise, we won't get the correct changes
	    // passed thru.
	    if (cop->flags().timeDep)
		flags().timeDep = 1;
	}
	else rcode = -1;
    }
    return rcode;
}



OP_ERROR
SOP_CopRaster::cookMySop(OP_Context &context)
{
    fpreal t = context.getTime();

    // Update our raster...
    int rstate = updateRaster(t);

    if (rstate < 0)
    {
        // There's no raster, so destroy everything.
        gdp->clearAndDestroy();
    }
    else if (rstate > 0 || gdp->getNumPoints() == 0)
    {
        // Here, we've loaded a new image, or our detail was uncached, so lets change our geometry

        // If we have the same number of points as on the last
        // cook, we don't have to destroy them.
        GA_Offset startptoff;
        int xres = myRaster.Xres();
        int yres = myRaster.Yres();
        exint n = exint(xres)*exint(yres);
        bool samenum = (n == gdp->getNumPoints());
        bool sameres = samenum && (myPrevXRes == xres) && (myPrevYRes == yres);
        myPrevXRes = xres;
        myPrevYRes = yres;
        if (samenum)
        {
            startptoff = gdp->pointOffset(GA_Index(0));
        }
        else
        {
            // NOTE: This will bump the data IDs for remaining attributes:
            //       i.e. P and the topology attributes.
            gdp->clearAndDestroy();

            // For each pixel in the raster, create a point in our geometry
            startptoff = gdp->appendPointBlock(n);
        }

        // Clear the current node selection.  The argument GA_GROUP_POINT shows 
        // that we want a point selection after this routine call.
        clearSelection(GA_GROUP_POINT);

        // Add diffuse color, if not already added on previous cook.
        GA_RWHandleV3 colorh(gdp->findDiffuseAttribute(GA_ATTRIB_POINT));
        if (!colorh.isValid())
            colorh = GA_RWHandleV3(gdp->addDiffuseAttribute(GA_ATTRIB_POINT));

        // Now find out about our raster
        UT_RGBA *rgba = myRaster.getRaster();

        // Copy the colour from each pixel to the corresponding point.
        // This SOP always generates a contiguous block of point offsets,
        // so we can just start at startptoff and increment.
        GA_Offset ptoff = startptoff;
        for (int x = 0; x < xres; ++x)
        {
            for (int y = 0; y < yres; ++y, ++rgba, ++ptoff)
            {
                // We don't need to set the point positions again
                // if the resolution is the same as on the last cook.
                if (!sameres)
                    gdp->setPos3(ptoff, (float)x/(float)xres, (float)y/(float)yres, 0);
                UT_Vector3 clr(rgba->r, rgba->g, rgba->b);
                clr *= 1.0/255.0;
                colorh.set(ptoff, clr);
            }
        }

        // Add the newly created points to the node selection.
        // We could have added them one-by-one with
        // selectPoint(ptoff, true, true), but that would have been slow.
        select(GA_GROUP_POINT);

        // Bump the attribute data IDs of the modified attributes.
        colorh.bumpDataId();
        if (!sameres)
            gdp->getP()->bumpDataId();
    }
    return error();
}

void
newSopOperator(OP_OperatorTable *table)
{
    OP_Operator *op = new OP_Operator(
        "hdk_copraster",                // Internal name
        "COP Raster",                   // UI name
        SOP_CopRaster::myConstructor,   // How to build
        SOP_CopRaster::myTemplateList,  // My parms
        0,                              // Min # of sources
        0,                              // Max # of sources
        0,                              // Local variables
        OP_FLAG_GENERATOR);             // Flag it as generator

    table->addOperator(op);
}
