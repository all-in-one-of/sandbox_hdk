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
 * NAME:	GT_PrimTetra.h ( GT Library, C++)
 *
 * COMMENTS:
 */

#include "GT_PrimTetra.h"
#include "GEO_PrimTetra.h"
#include <GT/GT_GEOPrimitive.h>
#include <GT/GT_GEODetailList.h>
#include <GT/GT_GEOAttributeFilter.h>
#include <GT/GT_Refine.h>
#include <GT/GT_DAConstantValue.h>
#include <GT/GT_PrimPolygonMesh.h>

using namespace HDK_Sample;

void
GT_PrimTetraCollect::registerPrimitive(const GA_PrimitiveTypeId &id)
{
    // Just construct.  The constructor registers itself.
    new GT_PrimTetraCollect(id);
}

GT_PrimTetraCollect::GT_PrimTetraCollect(const GA_PrimitiveTypeId &id)
    : myId(id)
{
    // Bind this collector to the given primitive id.  When GT refines
    // primitives and hits the given primitive id, this collector will be
    // invoked.
    bind(myId);
}

GT_PrimTetraCollect::~GT_PrimTetraCollect()
{
}

GT_GEOPrimCollectData *
GT_PrimTetraCollect::beginCollecting(const GT_GEODetailListHandle &geometry,
	const GT_RefineParms *parms) const
{
    // Collect the tet primitive offsets in a container
    return new GT_GEOPrimCollectOffsets();
}

GT_PrimitiveHandle
GT_PrimTetraCollect::collect(const GT_GEODetailListHandle &geometry,
	const GEO_Primitive *const* prim_list,
	int nsegments,
	GT_GEOPrimCollectData *data) const
{
    GT_GEOPrimCollectOffsets	*list;
    list = data->asPointer<GT_GEOPrimCollectOffsets>();
    list->append(prim_list[0]);
    return GT_PrimitiveHandle();
}

GT_PrimitiveHandle
GT_PrimTetraCollect::endCollecting(const GT_GEODetailListHandle &geometry,
				GT_GEOPrimCollectData *data) const
{
    GU_ConstDetailHandle	 gdh = geometry->getGeometry(0);
    GU_DetailHandleAutoReadLock	 rlock(gdh);
    const GU_Detail		&gdp = *rlock;
    GT_GEOPrimCollectOffsets	*list;
    list = data->asPointer<GT_GEOPrimCollectOffsets>();

    const GT_GEOOffsetList	&offsets = list->getPrimitives();
    if (!offsets.entries())
	return GT_PrimitiveHandle();

    // There have been tet's collected, so now we have to build up the
    // appropriate structures to build the GT primitive.
    GT_GEOOffsetList	ga_faces;
    GT_GEOOffsetList	ga_vertices;
    static const int	vertex_order[] = {
			    0, 1, 2,
			    1, 3, 2,
			    1, 0, 3,
			    0, 2, 3
			};

    // Extract two lists, the list of the primitive corresponding to each face
    // in the polygon mesh, along with the list of all the vertices for each
    // face in the mesh.
    for (exint i = 0; i < offsets.entries(); ++i)
    {
	// Each tet has 4 faces
	const GEO_Primitive	*prim = gdp.getGEOPrimitive(offsets(i));
	for (int face = 0; face < 4; ++face)
	{
	    // Duplicate the primitive offset for each face
	    ga_faces.append(offsets(i));

	    // And now, build a list of the vertex offsets
	    for (int voff = 0; voff < 3; ++voff)
	    {
		int	vidx = vertex_order[face*3 + voff];
		ga_vertices.append(prim->getVertexOffset(vidx));
	    }
	}
    }

    // Build the data structures needed for GT_PrimPolygonMesh.
    GT_PrimPolygonMesh		*pmesh;
    GT_DataArrayHandle		 counts;
    GT_DataArrayHandle		 ptnums;
    GT_AttributeListHandle	 shared;
    GT_AttributeListHandle	 vertex;
    GT_AttributeListHandle	 uniform;
    GT_AttributeListHandle	 detail;

    // The attribute filter isn't used in this primitive, but it allows
    // attributes to be excluded (i.e. spheres, might not want the "N" or the
    // "P" attributes).
    GT_GEOAttributeFilter	 filter;

    // The "shared" attributes will contain the attributes for all the points
    // in the geometry.
    //
    // The attribute list consists of GT data arrays which are filled with
    // attribute data from the GA detail(s).  Each data array stored in the
    // attribute list will have as many entries as the gdp's point offsets.
    // The elements should be indexed by the corresponding point offset
    shared = geometry->getPointAttributes(filter);

    // The vertex attributes are indexed by the GA_Offset of the vertices
    //
    // Each array in the attribute list will be filled by the attribute data
    // associated with the GA_Offset of the vertex in the ga_vertices array.
    // These are indexed by @c 0 to @c ga_vertices.entries().
    vertex = geometry->getVertexAttributes(filter, &ga_vertices);

    // Create primitive attributes.
    //
    // Each array item is filled with the attribute data for the corresponding
    // face in the ga_faces array.
    uniform = geometry->getPrimitiveAttributes(filter, &ga_faces);

    // Create detail attributes.  These are common for all faces
    detail = geometry->getDetailAttributes(filter);

    // Every face has 3 vertices, so create an array of the right size that's
    // filled with '3'.
    counts = GT_DataArrayHandle(new GT_IntConstant(ga_faces.entries(), 3));

    // Since the @c shared array is indexed by the point offset, there needs to
    // be a mapping from the vertex to the point offset.
    ptnums = ga_vertices.createVertexPointArray(gdp);

    pmesh = new GT_PrimPolygonMesh(counts, ptnums,
			shared, vertex, uniform, detail);

    return GT_PrimitiveHandle(pmesh);
}
