/*
 * PROPRIETARY INFORMATION.  This software is proprietary to
 * Side Effects Software Inc., and is not to be reproduced,
 * transmitted, or disclosed in any way without written permission.
 *
 * Produced by:
 *	Side Effects Software Inc
 *	123 Front Street West, Suite 1401
 *	Toronto, Ontario
 *	Canada   M5J 2M2
 *	416-504-9876
 *
 * NAME:	GUI_PolySoupBox.C (GR Library, C++)
 *
 * COMMENTS:
 */
#include "GUI_PolySoupBox.h"

#include <GEO/GEO_PrimPolySoup.h>

#include <GT/GT_GEOPrimitive.h>
#include <GR/GR_GeoRender.h>
#include <GR/GR_DisplayOption.h>
#include <GR/GR_OptionTable.h>
#include <GR/GR_UserOption.h>
#include <GR/GR_Utils.h>
#include <DM/DM_RenderTable.h>

#include <RE/RE_Geometry.h>
#include <RE/RE_Render.h>

#include <UT/UT_DSOVersion.h>

using namespace HDK_Sample;

const char *BBOX_OPTION = "psoup_bbox";
const char *BARY_OPTION = "psoup_bary";

const int BARY_DRAW_GROUP = 0;
const int BBOX_DRAW_GROUP = 1;

// install the hook.
void newRenderHook(DM_RenderTable *table)
{
    // the priority only matters if multiple hooks are assigned to the same
    // primitive type. If this is the case, the hook with the highest priority
    // (largest priority value) is processed first.
    const int priority = 0;

    // register the actual hook
    table->registerGEOHook(new GUI_PolySoupBoxHook(),
			   GA_PrimitiveTypeId(GA_PRIMPOLYSOUP),
			   priority,
			   GUI_HOOK_FLAG_AUGMENT_PRIM);
    
    // register custom display options for the hook
    table->installGeometryOption(BBOX_OPTION, "PolySoup BBox");
    table->installGeometryOption(BARY_OPTION, "PolySoup Barycenter");
}


// -------------------------------------------------------------------------
// The render hook itself. It is reponsible for creating new GUI_PolySoupBox
// primitives when request by the viewport.

GUI_PolySoupBoxHook::GUI_PolySoupBoxHook()
    : GUI_PrimitiveHook("PolySoup Box")
{
}

GUI_PolySoupBoxHook::~GUI_PolySoupBoxHook()
{
}

GR_Primitive *
GUI_PolySoupBoxHook::createPrimitive(const GT_PrimitiveHandle   &gt_prim,
				     const GEO_Primitive	*geo_prim,
				     const GR_RenderInfo	*info,
				     const char			*cache_name,
				     GR_PrimAcceptResult	&processed)
{
    if(geo_prim->getTypeId().get() == GA_PRIMPOLYSOUP)
    {
	// We're going to process this prim and prevent any more lower-priority
	// hooks from hooking on it. Alternatively, GR_PROCESSED_NON_EXCLUSIVE
	// could be passed back, in which case any hooks of lower priority would
	// also be called.
	processed = GR_PROCESSED;

	// In this case, we aren't doing anything special, like checking attribs
	// to see if this is a flagged native primitive we want to hook on.
	return new GUI_PolySoupBox(info, cache_name, geo_prim);
    }

    return NULL;
}



// -------------------------------------------------------------------------
// The decoration rendering code for the primitive.

GUI_PolySoupBox::GUI_PolySoupBox(const GR_RenderInfo *info,
				 const char *cache_name,
				 const GEO_Primitive *prim)
    : GR_Primitive(info, cache_name, GA_PrimCompat::TypeMask(0))
{
    myGeometry = NULL;
}

GUI_PolySoupBox::~GUI_PolySoupBox()
{
    delete myGeometry;
}


GR_PrimAcceptResult
GUI_PolySoupBox::acceptPrimitive(GT_PrimitiveType t,
				int geo_type,
				const GT_PrimitiveHandle &ph,
				const GEO_Primitive *prim)
{
    if(geo_type == GA_PRIMPOLYSOUP)
	return GR_PROCESSED;
    
    return GR_NOT_PROCESSED;
}


void
GUI_PolySoupBox::update(RE_Render		  *r,
			const GT_PrimitiveHandle  &primh,
			const GR_UpdateParms      &p)
{
    if(p.reason & (GR_GEO_CHANGED |
		   GR_GEO_TOPOLOGY_CHANGED |
		   GR_INSTANCE_PARMS_CHANGED))
    {
	// Fetch the GEO primitive from the GT primitive handle
	const GT_GEOPrimitive *prim =
	    static_cast<const GT_GEOPrimitive *>(primh.get());
	const GEO_PrimPolySoup *ps =
	    static_cast<const GEO_PrimPolySoup *>(prim->getPrimitive(0));

	UT_BoundingBox box;
	UT_Vector3 barycenter = ps->baryCenter();
	bool new_geo = false;
    
	box.initBounds();
	ps->getBBox(&box);

	if(!myGeometry)
	{
	    myGeometry = new RE_Geometry(8 + 1);
	    new_geo = true;
	}

	UT_Vector3FArray pos(9,9);
	
	pos(0) = barycenter;
		
	pos(1) = UT_Vector3F(box.xmin(), box.ymin(), box.zmin());
	pos(2) = UT_Vector3F(box.xmax(), box.ymin(), box.zmin());
	pos(3) = UT_Vector3F(box.xmax(), box.ymax(), box.zmin());
	pos(4) = UT_Vector3F(box.xmin(), box.ymax(), box.zmin());
	
	pos(5) = UT_Vector3F(box.xmin(), box.ymin(), box.zmax());
	pos(6) = UT_Vector3F(box.xmax(), box.ymin(), box.zmax());
	pos(7) = UT_Vector3F(box.xmax(), box.ymax(), box.zmax());
	pos(8) = UT_Vector3F(box.xmin(), box.ymax(), box.zmax());

	myGeometry->createAttribute(r, "P", RE_GPU_FLOAT32, 3,
				    pos.array()->data());

	// build the instancing array (instmat) if point instancing is used,
	// otherwise assign a constant attribute value (normally identity)
	GR_Utils::buildInstanceObjectMatrix(r, primh, p, myGeometry,
					       p.instance_version);

	if(new_geo)
	{
	    // connectivity remains constant so it only needs to be updated
	    // when the RE_Geometry object is created.

	    // single point for barycenter
	    myGeometry->connectSomePrims(r, BARY_DRAW_GROUP,
					  RE_PRIM_POINTS, 0, 1);

	    // lines for the bounding box
	    const unsigned line_connect[24] = { 1,2, 2,3, 3,4, 4,1,
						5,6, 6,7, 7,8, 8,5,
						1,5, 2,6, 3,7, 4,8 };
	    
	    myGeometry->connectIndexedPrims(r, BBOX_DRAW_GROUP,
					     RE_PRIM_LINES, 24, line_connect);
	}
    }
}

void
GUI_PolySoupBox::renderDecoration(RE_Render *r,
				  GR_Decoration decor,
				  const GR_DecorationParms &p)
{
    if(decor >= GR_USER_DECORATION)
    {
	int index = decor - GR_USER_DECORATION;
	const GR_UserOption *user = GRgetOptionTable()->getOption(index);

	// for user-installed options, the option name defines the type.
	if(!strcmp(user->getName(), BBOX_OPTION))
	{
	    renderBBox(r, p.opts);
	}
	else if(!strcmp(user->getName(), BARY_OPTION))
	{
	    renderBary(r, p.opts);
	}
    }
}


void
GUI_PolySoupBox::renderBBox(RE_Render *r,
			    const GR_DisplayOption *opts)
{
    // enable smooth lines if the scene AA is on
    if(opts->common().getSceneAntialias() > 0)
    {
	r->pushSmoothLines();
	r->smoothBlendLines(RE_SMOOTH_ON);
    }
    
    // make the lines a bit thicker than a normal wire.
    r->pushLineWidth(opts->common().wireWidth() * 2.0);
    
    r->pushShader( GR_Utils::getWireShader(r) );

    // temporarily change the wire color to the crtAuxColor (hull color)
    fpreal32 col[4] = {0,0,0,1};
    opts->common().crtAuxColor().getRGB(col,col+1,col+2);
    r->pushUniformData(RE_UNIFORM_WIRE_COLOR, col);
	
    myGeometry->draw(r, BBOX_DRAW_GROUP);

    r->popUniformData(RE_UNIFORM_WIRE_COLOR);
    r->popShader();

    r->popLineWidth();
    
    if(opts->common().getSceneAntialias() > 0)
	r->popSmoothLines();
}

void
GUI_PolySoupBox::renderBary(RE_Render *r,
			    const GR_DisplayOption *opts)
{
    // make the point a bit more visible than a normal point.
    r->pushPointSize(opts->common().pointSize() * 2.0);
    
    // A simple wire shader that draws a line with RE_UNIFORM_WIRE_COLOR.
    // It also supports instancing.
    r->pushShader( GR_Utils::getWireShader(r) );
    
    myGeometry->draw(r, BARY_DRAW_GROUP);
    
    r->popShader();
    
    r->popPointSize();
}

void
GUI_PolySoupBox::render(RE_Render	      *r,
			GR_RenderMode	       render_mode,
			GR_RenderFlags	       flags,
			const GR_DisplayOption *opt,
			const RE_MaterialList  *materials)
{
    // The native Houdini primitive for polysoups will do the rendering. 
}

void
GUI_PolySoupBox::renderInstances(RE_Render	       *r,
				 GR_RenderMode		render_mode,
				 GR_RenderFlags		flags,
				 const GR_DisplayOption *opt,
				 const RE_MaterialList  *materials,
				 int instance_group)
{
    // The native Houdini primitive for polysoups will do the rendering. 
}

int
GUI_PolySoupBox::renderPick(RE_Render *r,
			    const GR_DisplayOption *opt,
			    unsigned int pick_type,
			    GR_PickStyle pick_style,
			    bool has_pick_map)
{
    // The native Houdini primitive for polysoups will do the rendering. 
    return 0;
}

