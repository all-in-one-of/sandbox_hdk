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
 * NAME:	GUI_PrimFramework.C (GR Library, C++)
 *
 * COMMENTS:
 */
#include "GUI_PrimVolumeVelField.h"

#include <GU/GU_PrimVolume.h>

#include <GT/GT_GEOPrimitive.h>
#include <GR/GR_DisplayOption.h>
#include <GR/GR_GeoRender.h>
#include <GR/GR_OptionTable.h>
#include <GR/GR_UserOption.h>
#include <GR/GR_Utils.h>
#include <GR/GR_Utils.h>
#include <GR/GR_PickSelection.h>

#include <DM/DM_RenderTable.h>

#include <RE/RE_Geometry.h>
#include <RE/RE_Render.h>

#include <UT/UT_DSOVersion.h>

#define LINE_DRAW_GROUP  1
#define POINT_DRAW_GROUP 2

using namespace HDK_Sample;

// install the hook.
void newRenderHook(DM_RenderTable *dm_table)
{
    GA_PrimitiveTypeId type(GA_PRIMVOLUME);
    const int priority = 0;

    // The COLLECT_PRIMS flag is set because this needs 3 volume primitives
    // to complete the set.
    dm_table->registerGEOHook(new GUI_PrimVolumeVelFieldHook(),
			      type, priority, GUI_HOOK_FLAG_COLLECT_PRIMS);
}


// -------------------------------------------------------------------------
// The render hook itself. It is reponsible for creating new GUI_PrimFramework
// primitives when request by the viewport.

GUI_PrimVolumeVelFieldHook::GUI_PrimVolumeVelFieldHook()
    : GUI_PrimitiveHook("Volume Velocity Field HDK Example")
{
}


GUI_PrimVolumeVelFieldHook::~GUI_PrimVolumeVelFieldHook()
{
}


GR_Primitive *
GUI_PrimVolumeVelFieldHook::createPrimitive(const GT_PrimitiveHandle &gtprimh,
					    const GEO_Primitive	*prim,
					    const GR_RenderInfo	*info,
					    const char		*cache_name,
					    GR_PrimAcceptResult	&processed)
{
    // This looks for 3 component volumes of 'v' - v.x, v.y, v.z. The names
    // are stored in a name string attribute by Houdini. All 3 volumes must
    // share the same resolution.
    
    if(prim->getTypeId().get() == GA_PRIMVOLUME)
    {
	const GEO_Detail *detail = prim->getParent();
	GA_ROHandleS name(detail->findPrimitiveAttribute("name"));

	if(name.isValid())
	{
            UT_String vol_name(name.get(prim->getMapOffset()));
	    if (vol_name.isstring())
	    {
		if(vol_name == "v.x" || vol_name == "v.y" || vol_name == "v.z")
		{
		    processed = GR_PROCESSED;
		    
		    GUI_PrimVolumeVelField *field =
			new GUI_PrimVolumeVelField(info, cache_name);

		    field->acceptPrimitive(GT_GEO_PRIMITIVE,
					   GA_PRIMVOLUME,
					   gtprimh, prim);
		    return field;
		}
	    }
	}
    }

    return NULL;
}



// -------------------------------------------------------------------------
// The decoration rendering code for the primitive.

GUI_PrimVolumeVelField::GUI_PrimVolumeVelField(const GR_RenderInfo *info,
					       const char *cache_name)
    : GR_Primitive(info, cache_name, GEO_PrimTypeCompat::GEOPRIMVOLUME)
{
    myField = NULL;
    myVectorScale = 1.0;
    myPrimSelected = false;
    myObjectSelected = false;
    myVX = GA_INVALID_OFFSET;
    myVY = GA_INVALID_OFFSET;
    myVZ = GA_INVALID_OFFSET;
}


GUI_PrimVolumeVelField::~GUI_PrimVolumeVelField()
{
    delete myField;
}


void
GUI_PrimVolumeVelField::resetPrimitives()
{
    // clear out our stashed primitive offsets, ready for a new round of
    // acceptPrimitive() to be called.
    myVX = GA_INVALID_OFFSET;
    myVY = GA_INVALID_OFFSET;
    myVZ = GA_INVALID_OFFSET;
}


GR_PrimAcceptResult
GUI_PrimVolumeVelField::acceptPrimitive(GT_PrimitiveType gt_type,
					int geo_type,
					const GT_PrimitiveHandle &ph,
					const GEO_Primitive *prim)
{
    if(GAisValid(myVX) &&
       GAisValid(myVY) &&
       GAisValid(myVZ))
    {
	// finished, got all 3 components of v.
	return GR_NOT_PROCESSED;
    }

    // This hook is only interested in volumes with the names v.x, v.y or v.z.
    // They all must be the same resolution.
    
    if(geo_type == GA_PRIMVOLUME)
    {
	const GEO_Detail *detail = prim->getParent();

	// make sure that if we already have at least 1 volume, the volume we
	// are attempting to add is the same resolution.
	GA_Offset off = GA_INVALID_OFFSET;

	if(GAisValid(myVX))
	    off = myVX;
	else if(GAisValid(myVY))
	    off = myVY;
	else if(GAisValid(myVZ))
	    off = myVZ;

	if(GAisValid(off))
	{
	    UT_Vector3i res, vres;
	    const GU_PrimVolume *v1 = UTverify_cast<const GU_PrimVolume*>(prim);
	    const GU_PrimVolume *v0 = static_cast<const GU_PrimVolume *>
		(detail->getGEOPrimitive(off));

	    v0->getRes(vres.x(), vres.y(), vres.z());
	    v1->getRes(res.x(), res.y(), res.z());

	    if(res != vres)
		return GR_NOT_PROCESSED;
	}

	// check for a name attribute identifying this volume as v.[xyz]
	GA_ROHandleS name(detail->findPrimitiveAttribute("name"));
	if(name.isValid())
	{
	    UT_String vol_name(name.get(prim->getMapOffset()));
	    if (vol_name.isstring())
	    {
		if(vol_name == "v.x")
		{
		    myVX = prim->getMapOffset();
		    myVXIdx = prim->getMapIndex();
		    return GR_PROCESSED;
		}
		
		if(vol_name == "v.y")
		{
		    myVY = prim->getMapOffset();
		    myVYIdx = prim->getMapIndex();
		    return GR_PROCESSED;
		}

		if(vol_name == "v.z")
		{
		    myVZ = prim->getMapOffset();
		    myVZIdx = prim->getMapIndex();
		    return GR_PROCESSED;
		}
	    }
	}
    }
    
    return GR_NOT_PROCESSED;
}


GR_Primitive::GR_DispOptChange
GUI_PrimVolumeVelField::displayOptionChange(const GR_DisplayOption &opts,
					    bool first_init)
{
    GR_DispOptChange changed = DISPLAY_UNCHANGED;
    
    fpreal scale = opts.common().vectorScale();

    if(!first_init)
    {
	// since the vector scale is baked into the geometry, if this changes,
	// the primitive must update.
	if(scale != myVectorScale)
	    changed = DISPLAY_CHANGED;
    }
    
    myVectorScale = scale;
	
    return changed;
}


void
GUI_PrimVolumeVelField::update(RE_Render		  *r,
			       const GT_PrimitiveHandle  &primh,
			       const GR_UpdateParms      &p)
{
    // Need all 3 vector fields for this example.
    if(!GAisValid(myVX) || !GAisValid(myVY) || !GAisValid(myVZ))
    {
	delete myField;
	myField = NULL;
	
	return;
    }

    GU_DetailHandleAutoReadLock	rlock(p.geometry);
    // fetch the volume primitives
    const GU_PrimVolume *vx = static_cast<const GU_PrimVolume *>
	(rlock->getGEOPrimitive(myVX));
    const GU_PrimVolume *vy = static_cast<const GU_PrimVolume *>
	(rlock->getGEOPrimitive(myVY));
    const GU_PrimVolume *vz = static_cast<const GU_PrimVolume *>
	(rlock->getGEOPrimitive(myVZ));

    // if we've come this far, we should have all 3 volume prims.
    UT_ASSERT(vx && vy && vz);

    // If the geometry itself requires an update, rebuild it
    if(!myField || (p.reason & (GR_GEO_CHANGED |
				GR_GEO_TOPOLOGY_CHANGED |
				GR_DISPLAY_OPTIONS_CHANGED)))
    {
	UT_Vector3i res;
	int npoints;

	// Create a new geometry with enough points for 1 line/voxel
	vx->getRes(res.x(), res.y(), res.z());

	npoints = (res.x() * res.y() * res.z()) * 2;

	if(myField)
	    myField->setNumPoints( npoints );
	else
	    myField = new RE_Geometry( npoints );

	// build a position and color array for the lines. Color the lines
	// so that blue is near-zero velocity, and red is vel > 1 (after
	// applying the velocity scale).
	
	UT_Vector3FArray pos(npoints, npoints);
	UT_Vector3FArray col(npoints, npoints);
	fpreal vscale = p.dopts.common().vectorScale();
	int idx=0;
	
	for(int x=0; x<res.x(); x++)
	    for(int y=0; y<res.y(); y++)
		for(int z=0; z<res.z(); z++, idx+=2)
		{
		    UT_Vector3 p;
		    UT_Vector3 v;
		    fpreal speed;
		    UT_Color c;

		    vx->indexToPos(x,y,z, p);
		    pos(idx) = p;

		    v.assign( vx->getValueAtIndex(x,y,z),
			      vy->getValueAtIndex(x,y,z),
			      vz->getValueAtIndex(x,y,z) );

		    pos(idx+1) = p + v * vscale;
		    
		    // color by the magnitude of the vel, from blue->red.
		    speed = v.length() * vscale;

		    c.setHSL( SYSclamp(1.0-speed, 0.0,1.0) * 240.0,
			      1.0, 0.25);
		    
		    col(idx) = c.rgb();
		    c.setHSL( SYSclamp(1.0-speed, 0.0,1.0) * 240.0,
			      0.5, 0.15);
		    
		    col(idx+1) = c.rgb();
		    
		}

	// GL3 uses named attributes.
	myField->createAttribute(r, "P", RE_GPU_FLOAT32, 3,
				 pos.array()->data());
	myField->createAttribute(r, "Cd", RE_GPU_FLOAT32, 3,
				 col.array()->data());

	if(p.reason & (GR_GEO_CHANGED | GR_GEO_TOPOLOGY_CHANGED))
	{
	    // build connectivity:
	    myField->resetConnectedPrims();

	    // lines (connect group 1)
	    myField->connectAllPrims(r, LINE_DRAW_GROUP, RE_PRIM_LINES);

	    // every vector origin (connect group 2)
	    myField->connectSomePrims(r, POINT_DRAW_GROUP,
				       RE_PRIM_POINTS, 0, npoints, 2);
	}
    }

    if(p.reason & GR_GEO_SELECTION_CHANGED)
	myPrimSelected = GR_Utils::inPrimitiveSelection(p, myVX);

    myObjectSelected = p.object_selected;
    
    if(p.reason & (GR_GEO_CHANGED |
		   GR_GEO_TOPOLOGY_CHANGED |
		   GR_INSTANCE_PARMS_CHANGED |
		   GR_INSTANCE_SELECTION_CHANGED))
    {
	// This builds the instancing array, or assigns a constant identity
	// or per-primitive matrix to the attrib 'mat4 instmat'
	GR_Utils::buildInstanceObjectMatrix(r, primh, p, myField,
					       p.instance_version);
    }
}

void
GUI_PrimVolumeVelField::render(RE_Render	      *r,
			       GR_RenderMode	       render_mode,
			       GR_RenderFlags	       flags,
			       GR_DrawParms	       dp)
{
    if(!myField)
	return;
    
    if(render_mode == GR_RENDER_BEAUTY ||
       render_mode == GR_RENDER_MATERIAL_WIREFRAME ||
       render_mode == GR_RENDER_WIREFRAME ||
       render_mode == GR_RENDER_HIDDEN_LINE ||
       render_mode == GR_RENDER_GHOST_LINE)
    {
	// All of these modes draw in the same manner.
	
	// $HH/glsl/basic/GL32/const_color.prog
	r->pushShader(GR_Utils::getColorShader(r));

	if(dp.draw_instanced)
	    myField->drawInstanceGroup(r, LINE_DRAW_GROUP, dp.instance_group);
	else
	    myField->draw(r, LINE_DRAW_GROUP);

	// $HH/glsl/basic/GL32/wire_color.prog
	r->bindShader(GR_Utils::getWireShader(r));

	if(myPrimSelected)
	{
	    fpreal32 col[4];

	    dp.opts->common().selPrimColor().getRGB(col,col+1,col+2);
	    col[3] = 1.0;

	    r->pushUniformData(RE_UNIFORM_WIRE_COLOR, col);
	}

	// Draw points so that zero-vel fields appear
	r->pushPointSize((myObjectSelected || myPrimSelected) ? 4.0 : 2.0);
	if(dp.draw_instanced)
	    myField->drawInstanceGroup(r, POINT_DRAW_GROUP, dp.instance_group);
	else
	    myField->draw(r, POINT_DRAW_GROUP);
	r->popPointSize();

	if(myPrimSelected)
	    r->popUniformData(RE_UNIFORM_WIRE_COLOR);
	    
	r->popShader();
    }
    else if(render_mode == GR_RENDER_OBJECT_PICK)
    {
	// just draw, object pick shader is already pushed
	draw(r, dp.draw_instanced ? dp.instance_group : -1, 3.0);
    }
    else if(render_mode == GR_RENDER_MATTE)
    {
	// fetch $HH/glsl/basic/GL32/matte.prog
	RE_Shader *sh = GR_Utils::getMatteShader(r);

	r->pushLineWidth(3.0);
	r->pushShader(sh);
	draw(r, dp.draw_instanced ? dp.instance_group : -1, 3.0);
	r->popShader();
	r->popLineWidth();
    }
}

void
GUI_PrimVolumeVelField::draw(RE_Render *r,
			     int instance_group,
			     fpreal point_size)
{
    // Draw vectors
    if(instance_group >= 0)
	myField->drawInstanceGroup(r, LINE_DRAW_GROUP, instance_group);
    else
	myField->draw(r, LINE_DRAW_GROUP);

    // Draw points so that zero-vel fields appear
    r->pushPointSize(2.0);
    if(instance_group >= 0)
	myField->drawInstanceGroup(r, POINT_DRAW_GROUP, instance_group);
    else
	myField->draw(r, POINT_DRAW_GROUP);
    r->popPointSize();
}



int
GUI_PrimVolumeVelField::renderPick(RE_Render *r,
				   const GR_DisplayOption *opt,
				   unsigned int pick_type,
				   GR_PickStyle pickstyle,
				   bool has_pick_map)
{
    if(!myField || pick_type != GR_PICK_PRIMITIVE)
	return 0;

    // upload our pick ID. Must +1 as 0 is a reserved value.
    UT_Vector3i		 pick_id(myVXIdx+1, 0, 0);
    r->assignUniformData(RE_UNIFORM_PICK_COMPONENT_ID, pick_id.data());

    int			 npicks = 0;
    RE_Geometry		*pick_buffer = NULL;

    // ensure pick buffer is large enough to hold volume lines
    if((pickstyle & GR_PICK_MULTI_FLAG) != 0)
	pick_buffer = createPickBuffer(r, myField->getNumPoints()/2, 1);

    // Pick the lines.
    GR_PickRender	 picker(r, opt, myInfo, myField);
    npicks = picker.renderLinePrims(LINE_DRAW_GROUP, 1,
				GR_PICK_CONSTANT_ID,
				pickstyle, has_pick_map, true,
				GR_SELECT_PRIM_FULL, -1,
				pick_buffer);

    // npicks will only be non-zero for frustum picking, not GR_PICK_SINGLE.
    // GR_PICK_SINGLE renders to an ivec4 framebuffer, which will be queried
    // later.
    if(npicks > 0)
	return accumulatePickIDs(r, npicks);
    
    return 0;
}

