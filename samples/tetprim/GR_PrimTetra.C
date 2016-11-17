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
 * NAME:	GR_PrimTetra.C (GR Library, C++)
 *
 * COMMENTS:
 */
#include "GR_PrimTetra.h"
#include "GEO_PrimTetra.h"

#include <GT/GT_GEOPrimitive.h>
#include <DM/DM_RenderTable.h>
#include <RE/RE_ElementArray.h>
#include <RE/RE_Geometry.h>
#include <RE/RE_LightList.h>
#include <RE/RE_ShaderHandle.h>
#include <RE/RE_VertexArray.h>

using namespace HDK_Sample;

// See GEO_PrimTetra::registerMyself() for installation of the hook.

GR_PrimTetraHook::GR_PrimTetraHook()
    : GUI_PrimitiveHook("Tetrahedron")
{
}

GR_PrimTetraHook::~GR_PrimTetraHook()
{
}

GR_Primitive *
GR_PrimTetraHook::createPrimitive(const GT_PrimitiveHandle &gt_prim,
				  const GEO_Primitive	*geo_prim,
				  const GR_RenderInfo	*info,
				  const char		*cache_name,
				  GR_PrimAcceptResult  &processed)
{
    return new GR_PrimTetra(info, cache_name, geo_prim);
}

GR_PrimTetra::GR_PrimTetra(const GR_RenderInfo *info,
			   const char *cache_name,
			   const GEO_Primitive *prim)
    : GR_Primitive(info, cache_name, GA_PrimCompat::TypeMask(0))
{
#ifdef TETRA_GR_PRIM_COLLECTION
    myPrimIndices.append( prim->getMapIndex() );
#endif
    
    myID = prim->getTypeId().get();
    myGeo = NULL;
}

GR_PrimTetra::~GR_PrimTetra()
{
    delete myGeo;
}


void
GR_PrimTetra::resetPrimitives()
{
#ifdef TETRA_GR_PRIM_COLLECTION
    myPrimIndices.entries(GA_Index(0));
#else
    // nothing for single primitives - will not be called.
#endif
}

GR_PrimAcceptResult
GR_PrimTetra::acceptPrimitive(GT_PrimitiveType t,
			      int geo_type,
			      const GT_PrimitiveHandle &ph,
			      const GEO_Primitive *prim)
{
    if(geo_type == myID)
    {
#ifdef TETRA_GR_PRIM_COLLECTION
	myPrimIndices.append( prim->getMapIndex() );
#endif
	
	return GR_PROCESSED;
    }
    
    return GR_NOT_PROCESSED;
}


void
GR_PrimTetra::update(RE_Render		       *r,
		     const GT_PrimitiveHandle  &primh,
		     const GR_UpdateParms      &p)
{
    // Fetch the GEO primitive from the GT primitive handle
    const GEO_PrimTetra *tet = NULL;
    
    // GL3 and above requires named vertex attributes, while GL2 and GL1 use
    // the older builtin names.
    
    const char *posname = "P";
    const char *nmlname = "N";
    RE_VertexArray *pos = NULL;
    RE_VertexArray *nml = NULL;
    UT_Vector3FArray pt;

    // Initialize the geometry with the proper name for the GL cache
    if(!myGeo)
	myGeo = new RE_Geometry;
    myGeo->cacheBuffers(getCacheName());

    
    // Enable this codepath in tetra.C to collect multiple GEO_PrimTetras into
    // one GR_PrimTetra.
#ifdef TETRA_GR_PRIM_COLLECTION

    // Multiple GEO_PrimTetra's collected for this GR_Primitive
    const int num_tets = myPrimIndices.entries();
    if(num_tets == 0)
    {
	delete myGeo;
	myGeo = NULL;
	return;
    }

    {
	GU_DetailHandleAutoReadLock georl(p.geometry);
	const GU_Detail *dtl = georl.getGdp();
	
	for(int i=0; i<myPrimIndices.entries(); i++)
	{
	    GA_Offset off = dtl->primitiveIndex(myPrimIndices(i));
	
	    tet = dynamic_cast<const GEO_PrimTetra *>
		( dtl->getGEOPrimitive(off) );
	    if(tet)
	    {
		for(int v=0; v<4; v++)
		    pt.append(dtl->getPos3(dtl->vertexPoint(
						 tet->getVertexOffset(v))));
	    }
	}
    }
    
#else
    // Single GEO_PrimTetra for this GR_Primitive
    
    getGEOPrimFromGT<GEO_PrimTetra>(primh, tet);

    const int num_tets =  tet ? 1 : 0;
    if(num_tets == 0)
    {
	delete myGeo;
	myGeo = NULL;
	return;
    }

    // Extract tetra point positions.
    {
	GU_DetailHandleAutoReadLock georl(p.geometry);
	const GU_Detail *dtl = georl.getGdp();
	
	for(int v=0; v<4; v++)
	    pt.append(dtl->getPos3(dtl->vertexPoint(tet->getVertexOffset(v))));
    }
#endif

    // Initialize the number of points in the geometry.
    myGeo->setNumPoints( 12 * num_tets);

    GR_UpdateParms dp(p);
    const GR_Decoration pdecs[] = { GR_POINT_MARKER,
				    GR_POINT_NUMBER,
				    GR_POINT_NORMAL,
				    GR_POINT_UV,
				    GR_POINT_POSITION,
				    GR_POINT_VELOCITY,
				    GR_PRIM_NORMAL,
 				    GR_PRIM_NUMBER,
				    GR_VERTEX_MARKER,
				    GR_VERTEX_NORMAL,
				    GR_VERTEX_NUMBER,
				    GR_VERTEX_UV,
 				    GR_NO_DECORATION };
    myDecorRender->setupFromDisplayOptions(p.dopts, p.required_dec,
					   dp, pdecs, GR_POINT_ATTRIB);

    
    // Fetch P (point position). If its cache version matches, no upload is
    // required.
    pos = myGeo->findCachedAttrib(r, posname, RE_GPU_FLOAT32, 3,
				  RE_ARRAY_POINT, true);
    if(pos->getCacheVersion() != dp.geo_version)
    {
	// map() returns a pointer to the GL buffer
	UT_Vector3F *pdata = static_cast<UT_Vector3F *>(pos->map(r));
	if(pdata)
	{
	    for(int t=0; t<num_tets; t++)
	    {
		pdata[0] = pt(t*4);
		pdata[1] = pt(t*4+1);
		pdata[2] = pt(t*4+2);
	    
		pdata[3] = pt(t*4);
		pdata[4] = pt(t*4+2);
		pdata[5] = pt(t*4+3);

		pdata[6] = pt(t*4+1);
		pdata[7] = pt(t*4+2);
		pdata[8] = pt(t*4+3);

		pdata[9] = pt(t*4);
		pdata[10] = pt(t*4+3);
		pdata[11] = pt(t*4+1);

		pdata += 12;
	    }

	    // unmap the buffer so it can be used by GL
	    pos->unmap(r);
	    
	    // Always set the cache version after assigning data.
	    pos->setCacheVersion(dp.geo_version);
	}
    }

    
    // NOTE: you can add more attributes from a detail, such as Cd, uv, Alpha
    //       by repeating the process above.

    // Fetch N (point normal). This just generates normals for the tetras.
    nml = myGeo->findCachedAttrib(r, nmlname, RE_GPU_FLOAT32, 3,
				  RE_ARRAY_POINT, true);
    if(nml->getCacheVersion() != dp.geo_version)
    {
	UT_Vector3F *ndata = static_cast<UT_Vector3F *>(nml->map(r));
	if(ndata)
	{
	    UT_Vector3F n0, n1, n2, n3;

	    // This just creates primitive normals for the tet. It's currently
	    // not 100% correct (FIXME).
	    for(int t=0; t<num_tets; t++)
	    {
		n0 = -cross(pt(t*4+2)-pt(t*4), pt(t*4+1)-pt(t*4)).normalize();
		n1 =  cross(pt(t*4+3)-pt(t*4), pt(t*4+2)-pt(t*4)).normalize();
		n2 = -cross(pt(t*4+3)-pt(t*4+1),pt(t*4+2)-pt(t*4+1)).normalize();
		n3 = -cross(pt(t*4+1)-pt(t*4), pt(t*4+3)-pt(t*4)).normalize();

		ndata[0] = ndata[1] = ndata[2] = n0;
		ndata[3] = ndata[4] = ndata[5] = n1;
		ndata[6] = ndata[7] = ndata[8] = n2;
		ndata[9] = ndata[10] = ndata[11] = n3;

		ndata += 12;
	    }
	    
	    nml->unmap(r);

	    // Always set the cache version after assigning data.
	    nml->setCacheVersion(dp.geo_version);
	}
    }

    
    // Extra constant inputs for the GL3 default shaders we are using.
    // This isn't required unless 
    fpreal32 col[3] = { 1.0, 1.0, 1.0 };
    fpreal32 uv[2]  = { 0.0, 0.0 };
    fpreal32 alpha  = 1.0;
    fpreal32 pnt    = 0.0;
    UT_Matrix4F instance;
    instance.identity();
	
    myGeo->createConstAttribute(r, "Cd",    RE_GPU_FLOAT32, 3, col);
    myGeo->createConstAttribute(r, "uv",    RE_GPU_FLOAT32, 2, uv);
    myGeo->createConstAttribute(r, "Alpha", RE_GPU_FLOAT32, 1, &alpha);
    myGeo->createConstAttribute(r, "pointSelection", RE_GPU_FLOAT32, 1,&pnt);
    myGeo->createConstAttribute(r, "instmat", RE_GPU_MATRIX4, 1,
				instance.data());

    
    // Connectivity, for both shaded and wireframe
    myGeo->connectAllPrims(r, RE_GEO_WIRE_IDX, RE_PRIM_LINES, NULL, true);
    myGeo->connectAllPrims(r, RE_GEO_SHADED_IDX, RE_PRIM_TRIANGLES, NULL, true);
}


// GL3 shaders. These use some of the builtin Houdini shaders, which are
// described by the .prog file format - a simple container format for various
// shader stages and other information.

static RE_ShaderHandle theNQShader("material/GL32/beauty_lit.prog");
static RE_ShaderHandle theNQFlatShader("material/GL32/beauty_flat_lit.prog");
static RE_ShaderHandle theNQUnlitShader("material/GL32/beauty_unlit.prog");
static RE_ShaderHandle theHQShader("material/GL32/beauty_material.prog");
static RE_ShaderHandle theLineShader("basic/GL32/wire_color.prog");
static RE_ShaderHandle theConstShader("material/GL32/constant.prog");
static RE_ShaderHandle theZCubeShader("basic/GL32/depth_cube.prog");
static RE_ShaderHandle theZLinearShader("basic/GL32/depth_linear.prog");
static RE_ShaderHandle theMatteShader("basic/GL32/matte.prog");


void
GR_PrimTetra::render(RE_Render		    *r,
		     GR_RenderMode	     render_mode,
		     GR_RenderFlags	     flags,
		     const GR_DisplayOption *opt,
		     const RE_MaterialList  *materials)
{
    if(!myGeo)
	return;

    bool need_wire = (render_mode == GR_RENDER_WIREFRAME ||
		      render_mode == GR_RENDER_HIDDEN_LINE ||
		      render_mode == GR_RENDER_GHOST_LINE ||
		      render_mode == GR_RENDER_MATERIAL_WIREFRAME ||
		      (flags & GR_RENDER_FLAG_WIRE_OVER));
    
    // Shaded mode rendering
    if(render_mode == GR_RENDER_BEAUTY ||
       render_mode == GR_RENDER_MATERIAL ||
       render_mode == GR_RENDER_CONSTANT ||
       render_mode == GR_RENDER_HIDDEN_LINE ||
       render_mode == GR_RENDER_GHOST_LINE ||
       render_mode == GR_RENDER_DEPTH ||
       render_mode == GR_RENDER_DEPTH_LINEAR ||
       render_mode == GR_RENDER_DEPTH_CUBE ||
       render_mode == GR_RENDER_MATTE)
    {
	// enable polygon offset if doing a wireframe on top of shaded
	bool polyoff = r->isPolygonOffset();
	if(need_wire)
	    r->polygonOffset(true);

	r->pushShader();

	// GL3 requires the use of shaders. The fixed function pipeline
	// GL builtins (which are deprecated, like gl_ModelViewMatrix)
	// are not initialized in the GL3 renderer.
	    
	if(render_mode == GR_RENDER_BEAUTY)
	{
	    if(flags & GR_RENDER_FLAG_UNLIT)
		r->bindShader(theNQUnlitShader);
	    else if(flags & GR_RENDER_FLAG_FLAT_SHADED)
		r->bindShader(theNQFlatShader);
	    else
		r->bindShader(theNQShader);
	}
	else if(render_mode == GR_RENDER_MATERIAL)
	{
	    r->bindShader(theHQShader);
	}
	else if(render_mode == GR_RENDER_CONSTANT ||
		render_mode == GR_RENDER_DEPTH ||
		render_mode == GR_RENDER_HIDDEN_LINE ||
		render_mode == GR_RENDER_GHOST_LINE)
	{
	    // Reuse constant for depth-only since it's so lightweight.
	    r->bindShader(theConstShader);
	}
	else if(render_mode == GR_RENDER_DEPTH_LINEAR)
	{
	    // Depth written to world-space Z instead of non-linear depth
	    // buffer Z ([0..1] near-far depth range)
	    r->bindShader(theZLinearShader);
	}
	else if(render_mode == GR_RENDER_DEPTH_CUBE)
	{
	    // Linear depth written to 
	    r->bindShader(theZCubeShader);
	}
	else if(render_mode == GR_RENDER_MATTE)
	{
	    r->bindShader(theMatteShader);
	}

	
	// setup materials and lighting
	if(materials && materials->entries() == 1)
	{
	    RE_Shader *shader = r->getShader();
	    
	    // Set up lighting for any GL3 lighting blocks
	    if(shader)
		opt->getLightList()->bindForShader(r, shader);

	    // set up the main material block for GL3 
	    (*materials)(0)->updateShaderForMaterial(r, 0, true, true,
						     RE_SHADER_TARGET_TRIANGLE,
						     shader);
	}

	// Draw call for the geometry
	myGeo->draw(r, RE_GEO_SHADED_IDX);

	if(r->getShader())
	    r->getShader()->removeOverrideBlocks();
	r->popShader();

	if(need_wire && !polyoff)
	    r->polygonOffset(polyoff);
    }


    // Wireframe rendering
    if(need_wire)
    {
	// GL3 requires a shader even for simple wireframe rendering.
	r->pushShader(theLineShader);

	myGeo->draw(r, RE_GEO_WIRE_IDX);

	r->popShader();
    }
}

void
GR_PrimTetra::renderInstances(RE_Render		    *r,
			      GR_RenderMode	     render_mode,
			      GR_RenderFlags	     flags,
			      const GR_DisplayOption *opt,
			      const RE_MaterialList  *materials,
			      int instance_grp)
{
    // Similar to render, except that all the instances in group 'instance_grp'
    // should be rendered. Each instance group will call update() with
    // GR_UpdateParms::instance_group to identify the group and
    // GR_UpdateParms::instances to specify the transforms.
}
void
GR_PrimTetra::renderDecoration(RE_Render *r,
			       GR_Decoration decor,
			       const GR_DecorationParms &p)
{
    drawDecorationForGeo(r, myGeo, decor, p.opts, p.render_flags,
			 p.overlay, p.override_vis, p.instance_group,
			 GR_SELECT_NONE);

}

int
GR_PrimTetra::renderPick(RE_Render *r,
			 const GR_DisplayOption *opt,
			 unsigned int pick_type,
			 GR_PickStyle pick_style,
			 bool has_pick_map)
{
    // This example is not pickable.
    return 0;
}
