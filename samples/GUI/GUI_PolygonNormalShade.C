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
 * NAME:	GUI_PolygonNormalShade.h (GR Library, C++)
 *
 * COMMENTS:
 *	Example of a filter hook.
 *	Replaces Cd in a GT polygon mesh with false N color, mapped from
 *	normalized [-1,1] to [0,1].
 */
#include "GUI_PolygonNormalShade.h"

#include <DM/DM_RenderTable.h>
#include <GR/GR_DisplayOption.h>
#include <GT/GT_PrimPolygonMesh.h>
#include <GT/GT_AttributeList.h>
#include <GT/GT_DANumeric.h>

#include <UT/UT_DSOVersion.h>

using namespace HDK_Sample;

const char *NCD_OPTION = "visNasCd";

// install the hook.
void newRenderHook(DM_RenderTable *dm_table)
{
    // the priority only matters if multiple hooks are assigned to the same
    // primitive type. If this is the case, the hook with the highest priority
    // (largest priority value) is processed first.
    const int priority = 0;

    // register the actual hook
    dm_table->registerGTHook(new GUI_PolygonNormalShade(),
			     GT_PRIM_POLYGON_MESH,
			      priority, GUI_HOOK_FLAG_PRIM_FILTER);

    // because we hook into the refinement loop, when the option is
    // changed a refine must be performed. By default a custom option
    // does not trigger a refine.

    dm_table->installGeometryOption(
	NCD_OPTION, "Normal Color Override",
	GUI_GeometryOptionFlags(GUI_GEO_OPT_REFINE_ON_ACTIVATION |
	                        GUI_GEO_OPT_REFINE_ON_DEACTIVATION |
	                        GUI_GEO_OPT_GLOBAL_TOGGLE_VALUE));
}

GUI_PolygonNormalShade::GUI_PolygonNormalShade()
    : GUI_PrimitiveHook("Normal color override", GUI_RENDER_MASK_ALL)
{
}

GUI_PolygonNormalShade::~GUI_PolygonNormalShade()
{
}

GT_PrimitiveHandle
GUI_PolygonNormalShade::filterPrimitive(const GT_PrimitiveHandle &gt_prm,
					const GEO_Primitive  *geo_prm,
					const GR_RenderInfo  *info,
					GR_PrimAcceptResult  &processed)
{
    GT_PrimitiveHandle ph;
    
    if(!info->getDisplayOption()->getUserOptionState(NCD_OPTION))
    {
	// we're interested in this prim, but are not doing anything with it
	// at this time. Mark it as processed but don't return a new prim.
	processed = GR_PROCESSED;
	return ph;
    }

    // As this was registered for a GT prim type, a valid GT prim should be
    // passed to filterPrimitive(), not a GEO primitive.
    UT_ASSERT(geo_prm == NULL);
    UT_ASSERT(gt_prm);

    if(!gt_prm)
    {
	processed = GR_NOT_PROCESSED;
	return ph;
    }

    // Fetch the point normals (N) from the polygon mesh.
    GT_DataArrayHandle nh;
    bool point_normals = false;

    // Check to see if vertex normals are present.
    if(gt_prm->getVertexAttributes())
	nh = gt_prm->getVertexAttributes()->get("N");

    // If no vertex normals, check point normals.
    if(!nh)
    {
	if(gt_prm->getPointAttributes())
	    nh = gt_prm->getPointAttributes()->get("N");

	// If no point normals, generate smooth point normals from P and the
	// connectivity.
	if(!nh)
	{
	    auto mesh=UTverify_cast<const GT_PrimPolygonMesh *>(gt_prm.get());
	    if(mesh)
		nh = mesh->createPointNormals();
	}

	point_normals = true;
    }

    // Normals found: generate Cd from them.
    if(nh)
    {
	// Create a new data array for Cd (vec3 in FP16 format) and map the
	// normalized normal values from [-1,1] to [0,1].
	auto *cd =
	  new GT_DANumeric<fpreal16,GT_STORE_FPREAL16>(nh->entries(),
						       3, GT_TYPE_COLOR);
	GT_DataArrayHandle cdh = cd;
	
	for(int i=0; i<nh->entries(); i++)
	{
	    UT_Vector3F col;

	    nh->import(i, col.data(), 3);
	    col.normalize();

	    cd->getData(i)[0]= col.x() *0.5 + 0.5;
	    cd->getData(i)[1]= col.y() *0.5 + 0.5;
	    cd->getData(i)[2]= col.z() *0.5 + 0.5;
	}

	// Copy the input mesh, replacing either its vertex or point attribute
	// list.
	auto mesh=UTverify_cast<const GT_PrimPolygonMesh *>(gt_prm.get());

	if(point_normals)
	{
	    // Add Cd to the point attribute list, replacing it if Cd already
	    // exists.
	    GT_AttributeListHandle ah =
		gt_prm->getPointAttributes()->addAttribute("Cd", cdh, true);
	    ph = new GT_PrimPolygonMesh(*mesh,
					ah,
					mesh->getVertexAttributes(),
					mesh->getUniformAttributes(),
					mesh->getDetailAttributes());
	}
	else
	{
	    // Add Cd to the vertex attribute list, replacing it if Cd already
	    // exists.
	    GT_AttributeListHandle ah =
		gt_prm->getVertexAttributes()->addAttribute("Cd", cdh, true);
	    ph = new GT_PrimPolygonMesh(*mesh,
					mesh->getPointAttributes(),
					ah,
					mesh->getUniformAttributes(),
					mesh->getDetailAttributes());
	}
    }

    // return a new polygon mesh with modified Cd attribute.
    return ph;
}
