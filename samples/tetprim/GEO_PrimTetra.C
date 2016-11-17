/*
 * PROPRIETARY INFORMATION.  This software is proprietary to
 * Side Effects Software Inc., and is not to be reproduced,
 * transmitted, or disclosed in any way without written permission.
 *
 * Produced by:
 *	Jeff Lait
 *	Side Effects Software Inc
 *	477 Richmond Street West
 *	Toronto, Ontario
 *	Canada   M5V 3E7
 *	416-504-9876
 *
 * NAME:	GEO_PrimTetra.C ( GEO Library, C++)
 *
 * COMMENTS:	Base class for tetrahedrons.
 */

#include "GEO_PrimTetra.h"
#include <UT/UT_Defines.h>
#include <UT/UT_SparseArray.h>
#include <UT/UT_SysClone.h>
#include <UT/UT_IStream.h>
#include <UT/UT_JSONParser.h>
#include <UT/UT_JSONWriter.h>
#include <UT/UT_MemoryCounter.h>
#include <UT/UT_ParallelUtil.h>
#include <UT/UT_Vector3.h>
#include <GA/GA_AttributeRefMap.h>
#include <GA/GA_AttributeRefMapDestHandle.h>
#include <GA/GA_Defragment.h>
#include <GA/GA_WorkVertexBuffer.h>
#include <GA/GA_WeightedSum.h>
#include <GA/GA_MergeMap.h>
#include <GA/GA_IntrinsicMacros.h>
#include <GA/GA_ElementWrangler.h>
#include <GA/GA_PrimitiveJSON.h>
#include <GA/GA_RangeMemberQuery.h>
#include <GA/GA_SaveMap.h>
#include <GEO/GEO_ConvertParms.h>
#include <GEO/GEO_Detail.h>
#include <GEO/GEO_ParallelWiringUtil.h>
#include <GEO/GEO_PrimPoly.h>
#include <GEO/GEO_PrimType.h>
#include <DM/DM_RenderTable.h>

#include "GR_PrimTetra.h"
#include "GT_PrimTetra.h"

//#define TIMING_BUILDBLOCK
//#define SLOW_BUILDBLOCK

#ifdef TIMING_BUILDBLOCK
#include <UT/UT_StopWatch.h>
#define TIMING_DEF \
    UT_StopWatch timer; \
    timer.start();
#define TIMING_LOG(msg) \
    printf(msg ": %f milliseconds\n", 1000*timer.stop()); \
    fflush(stdout); \
    timer.start();
#else
#define TIMING_DEF
#define TIMING_LOG(msg)
#endif

using namespace HDK_Sample;

GA_PrimitiveDefinition *GEO_PrimTetra::theDef = 0;

// So it doesn't look like a magic number
#define PT_PER_TET 4

GEO_PrimTetra::GEO_PrimTetra(GA_Detail &d, GA_Offset offset)
    : GEO_Primitive(&d, offset)
{
    GA_Offset vtxoff = d.appendVertexBlock(4);
    for (GA_Size i = 0; i < PT_PER_TET; i++)
    {
        myVertexOffsets[i] = GA_Size(vtxoff);
        d.getTopology().wireVertexPrimitive(vtxoff, offset);
        ++vtxoff;
    }
}

GEO_PrimTetra::GEO_PrimTetra(const GA_MergeMap &map,
                GA_Detail &detail,
                GA_Offset offset,
                const GEO_PrimTetra &src)
    : GEO_Primitive(static_cast<GEO_Detail *>(&detail), offset)
{
    for (GA_Size i = 0; i < PT_PER_TET; i++)
    {
        myVertexOffsets[i] = GA_Size(map.mapDestFromSource(GA_ATTRIB_VERTEX, GA_Offset(src.myVertexOffsets[i])));
    }
}

GEO_PrimTetra::~GEO_PrimTetra()
{
    for (GA_Size i = 0; i < PT_PER_TET; i++)
    {
	if (fastVertexOffset(i) != GA_INVALID_OFFSET)
	    destroyVertex(fastVertexOffset(i));
    }
}

void
GEO_PrimTetra::clearForDeletion()
{
    for (int i = 0; i < PT_PER_TET; i++)
	myVertexOffsets[i] = GA_INVALID_OFFSET;
    GEO_Primitive::clearForDeletion();
}

void
GEO_PrimTetra::stashed(bool beingstashed, GA_Offset offset)
{
    // NB: Base class must be unstashed before we can call allocateVertex().
    GEO_Primitive::stashed(beingstashed, offset);
    for (int i = 0; i < PT_PER_TET; i++)
	myVertexOffsets[i] = beingstashed ? GA_INVALID_OFFSET : allocateVertex();
}

bool
GEO_PrimTetra::evaluatePointRefMap(GA_Offset result_vtx,
				GA_AttributeRefMap &map,
				fpreal u, fpreal v, unsigned du,
				unsigned dv) const
{
    map.copyValue(GA_ATTRIB_VERTEX, result_vtx,
		  GA_ATTRIB_VERTEX, getVertexOffset(0));
    return true;
}


void
GEO_PrimTetra::reverse()
{
    for (GA_Size r = 0; r < 2; r++)
    {
        GA_Size nr = 3 - r;
        GA_Offset other = fastVertexOffset(nr);

        myVertexOffsets[nr] = fastVertexOffset(r);
        myVertexOffsets[r] = other;
    }
}

UT_Vector3
GEO_PrimTetra::computeNormal() const
{
    return UT_Vector3(0, 0, 0);
}

fpreal
GEO_PrimTetra::calcVolume(const UT_Vector3 &) const
{
    UT_Vector3 v0 = getParent()->getPos3(vertexPoint(0));
    UT_Vector3 v1 = getParent()->getPos3(vertexPoint(1));
    UT_Vector3 v2 = getParent()->getPos3(vertexPoint(2));
    UT_Vector3 v3 = getParent()->getPos3(vertexPoint(3));

    // Measure signed volume of pyramid (v0,v1,v2,v3) 
    float signedvol = -(v3-v0).dot(cross(v1-v0, v2-v0));

    signedvol /= 6;

    return signedvol;
}

fpreal
GEO_PrimTetra::calcArea() const
{
    fpreal area = 0; // Signed area

    UT_Vector3 v0 = getParent()->getPos3(vertexPoint(0));
    UT_Vector3 v1 = getParent()->getPos3(vertexPoint(1));
    UT_Vector3 v2 = getParent()->getPos3(vertexPoint(2));
    UT_Vector3 v3 = getParent()->getPos3(vertexPoint(3));

    area += cross((v1 - v0), (v2 - v0)).length();
    area += cross((v3 - v1), (v2 - v1)).length();
    area += cross((v3 - v0), (v1 - v0)).length();
    area += cross((v2 - v0), (v3 - v0)).length();

    area = area / 2;

    return area;
}

fpreal
GEO_PrimTetra::calcPerimeter() const
{
    fpreal length = 0;

    UT_Vector3 v0 = getParent()->getPos3(vertexPoint(0));
    UT_Vector3 v1 = getParent()->getPos3(vertexPoint(1));
    UT_Vector3 v2 = getParent()->getPos3(vertexPoint(2));
    UT_Vector3 v3 = getParent()->getPos3(vertexPoint(3));

    length += (v1-v0).length();
    length += (v2-v0).length();
    length += (v3-v0).length();
    length += (v2-v1).length();
    length += (v3-v1).length();
    length += (v3-v2).length();

    return length;
}

GA_Size
GEO_PrimTetra::getVertexCount(void) const
{
    return PT_PER_TET;
}

int
GEO_PrimTetra::detachPoints(GA_PointGroup &grp)
{
    GA_Size count = 0;
    for (GA_Size i = 0; i < PT_PER_TET; ++i)
	if (grp.containsOffset(vertexPoint(i)))
	    count++;

    if (count == 0)
	return 0;

    if (count == PT_PER_TET)
	return -2;

    return -1;
}

GA_Primitive::GA_DereferenceStatus
GEO_PrimTetra::dereferencePoint(GA_Offset point, bool dry_run)
{
    for (GA_Size i = 0; i < PT_PER_TET; ++i)
    {
	if (vertexPoint(i) == point)
	{
	    return isDegenerate() ? GA_DEREFERENCE_DEGENERATE : GA_DEREFERENCE_FAIL;
	}
    }
    return GA_DEREFERENCE_OK;
}

GA_Primitive::GA_DereferenceStatus
GEO_PrimTetra::dereferencePoints(const GA_RangeMemberQuery &point_query, bool dry_run)
{
    int		count = 0;
    for (GA_Size i = 0; i < PT_PER_TET; ++i)
    {
	if (point_query.contains(vertexPoint(i)))
	{
	    count++;
	}
    }

    if (count == PT_PER_TET)
	return GA_DEREFERENCE_DESTROY;
    if (count == 0)
	return GA_DEREFERENCE_OK;

    if (isDegenerate())
	return GA_DEREFERENCE_DEGENERATE;
    return GA_DEREFERENCE_FAIL;
}

///
/// JSON methods
///

namespace HDK_Sample {

class geo_PrimTetraJSON : public GA_PrimitiveJSON
{
public:
    geo_PrimTetraJSON()
    {
    }
    virtual ~geo_PrimTetraJSON() {}

    enum
    {
	geo_TBJ_VERTEX,
	geo_TBJ_ENTRIES
    };

    const GEO_PrimTetra	*tet(const GA_Primitive *p) const
			{ return static_cast<const GEO_PrimTetra *>(p); }
    GEO_PrimTetra	*tet(GA_Primitive *p) const
			{ return static_cast<GEO_PrimTetra *>(p); }

    virtual int		getEntries() const	{ return geo_TBJ_ENTRIES; }
    virtual const char	*getKeyword(int i) const
			{
			    switch (i)
			    {
				case geo_TBJ_VERTEX:	return "vertex";
				case geo_TBJ_ENTRIES:	break;
			    }
			    UT_ASSERT(0);
			    return NULL;
			}
    virtual bool saveField(const GA_Primitive *pr, int i,
			UT_JSONWriter &w, const GA_SaveMap &map) const
		{
		    switch (i)
		    {
			case geo_TBJ_VERTEX:
			    return tet(pr)->saveVertexArray(w, map);
			case geo_TBJ_ENTRIES:
			    break;
		    }
		    return false;
		}
    virtual bool saveField(const GA_Primitive *pr, int i,
			UT_JSONValue &v, const GA_SaveMap &map) const
		{
		    switch (i)
		    {
			case geo_TBJ_VERTEX:
			    return false;
			case geo_TBJ_ENTRIES:
			    break;
		    }
		    UT_ASSERT(0);
		    return false;
		}
    virtual bool loadField(GA_Primitive *pr, int i, UT_JSONParser &p,
			const GA_LoadMap &map) const
		{
		    switch (i)
		    {
			case geo_TBJ_VERTEX:
			    return tet(pr)->loadVertexArray(p, map);
			case geo_TBJ_ENTRIES:
			    break;
		    }
		    UT_ASSERT(0);
		    return false;
		}
    virtual bool loadField(GA_Primitive *pr, int i, UT_JSONParser &p,
			const UT_JSONValue &v, const GA_LoadMap &map) const
		{
		    switch (i)
		    {
			case geo_TBJ_VERTEX:
			    return false;
			case geo_TBJ_ENTRIES:
			    break;
		    }
		    UT_ASSERT(0);
		    return false;
		}
    virtual bool isEqual(int i, const GA_Primitive *p0,
			const GA_Primitive *p1) const
		{
		    switch (i)
		    {
			case geo_TBJ_VERTEX:
			    return false;
			case geo_TBJ_ENTRIES:
			    break;
		    }
		    UT_ASSERT(0);
		    return false;
		}
private:
};
}

static const GA_PrimitiveJSON *
tetrahedronJSON()
{
    static GA_PrimitiveJSON	*theJSON = NULL;

    if (!theJSON)
	theJSON = new geo_PrimTetraJSON();
    return theJSON;
}

const GA_PrimitiveJSON *
GEO_PrimTetra::getJSON() const
{
    return tetrahedronJSON();
}



bool
GEO_PrimTetra::saveVertexArray(UT_JSONWriter &w,
		const GA_SaveMap &map) const
{
    if (!w.beginUniformArray(PT_PER_TET, UT_JID_INT32))
	return false;
    for (int i = 0; i < PT_PER_TET; i++)
    {
	int32	v = map.getIndex(fastVertexOffset(i), GA_ATTRIB_VERTEX);
	if (!w.uniformWrite(v))
	    return false;
    }
    return w.endUniformArray();
}

bool
GEO_PrimTetra::loadVertexArray(UT_JSONParser &p, const GA_LoadMap &map)
{
    GA_Offset vtxoff = map.getVertexOffset();

    int nvertex = p.parseUniformArray(myVertexOffsets, PT_PER_TET);
    for (int i = 0; i < nvertex; i++)
    {
	if (myVertexOffsets[i] >= 0)
	    myVertexOffsets[i] += GA_Size(vtxoff);
    }

    if (nvertex < PT_PER_TET)
	return false;
    return true;
}


int
GEO_PrimTetra::getBBox(UT_BoundingBox *bbox) const
{
    int		 cnt;

    bbox->initBounds(getDetail().getPos3(vertexPoint(0)));

    for (cnt = PT_PER_TET-1; cnt > 0; cnt--)
    {
	bbox->enlargeBounds(getDetail().getPos3(vertexPoint(cnt)));
    }

    return 1;
}

UT_Vector3
GEO_PrimTetra::baryCenter() const
{
    float	x, y, z;
    int		i;

    x = y = z = 0.0;

    for (i=PT_PER_TET-1; i>=0; i--)
    {
	UT_Vector3 pos(getDetail().getPos3(vertexPoint(i)));
	x += pos.x();
	y += pos.y();
	z += pos.z();
    }

    x /= PT_PER_TET;
    y /= PT_PER_TET;
    z /= PT_PER_TET;

    return UT_Vector3(x, y, z);
}

bool
GEO_PrimTetra::isDegenerate() const
{
    // Duplicate points means degenerate.
    for (int i = 0; i < PT_PER_TET; i++)
    {
	for (int j = i+1; j < PT_PER_TET; j++)
	{
	    if (vertexPoint(i) == vertexPoint(j))
		return true;
	}
    }
    return false;
}

void
GEO_PrimTetra::copyPrimitive(const GEO_Primitive *psrc)
{
    if (psrc == this) return;

    const GEO_PrimTetra *src = (const GEO_PrimTetra *)psrc;
    const GA_IndexMap &points = getParent()->getPointMap();
    const GA_IndexMap &src_points = src->getParent()->getPointMap();

    // TODO: Well and good to reuse the attribute handle for all our
    //       vertices, but we should do so across primitives as well.
    GA_VertexWrangler vertex_wrangler(*getParent(), *src->getParent());

    for (GA_Size i = 0; i < PT_PER_TET; ++i)
    {
        GA_Offset v = fastVertexOffset(i);
        GA_Offset ptoff = src->vertexPoint(i);
        if (&points != &src_points)
        {
            GA_Index ptidx = src_points.indexFromOffset(ptoff);
            ptoff = points.offsetFromIndex(ptidx);
        }
        wireVertex(v, ptoff);
        vertex_wrangler.copyAttributeValues(v, src->fastVertexOffset(i));
    }
}

GEO_Primitive *
GEO_PrimTetra::copy(int preserve_shared_pts) const
{
    GEO_Primitive *clone = GEO_Primitive::copy(preserve_shared_pts);

    if (clone)
    {
	GEO_PrimTetra	*face = (GEO_PrimTetra*)clone;
	GA_Offset	 ppt;

	// TODO: Well and good to reuse the attribute handle for all our
	//       points/vertices, but we should do so across primitives
	//       as well.
	GA_ElementWranglerCache	 wranglers(*getParent(),
					   GA_PointWrangler::INCLUDE_P);

	int nvtx = getVertexCount();

	int i;

	if (preserve_shared_pts)
	{
	    UT_SparseArray<GA_Offset *>	addedpoints;
	    GA_Offset			*ppt_ptr;

	    for (i = 0; i < nvtx; i++)
	    {
		GA_Offset		 src_ppt = vertexPoint(i);
		GA_Offset		 v  = face->fastVertexOffset(i);
		GA_Offset		 sv = fastVertexOffset(i);

		if (!(ppt_ptr = addedpoints(src_ppt)))
		{
		    ppt = getParent()->appendPointOffset();
		    wranglers.getPoint().copyAttributeValues(ppt, src_ppt);
		    addedpoints.append(src_ppt, new GA_Offset(ppt));
		}
		else
		    ppt = *ppt_ptr;
		face->wireVertex(v, ppt);
		wranglers.getVertex().copyAttributeValues(v, sv);
	    }

	    int dummy_index;
	    for (i = 0; i < addedpoints.entries(); i++)
		delete (GA_Offset *)addedpoints.getRawEntry(i, dummy_index);
	}
	else
	{
	    for (i = 0; i < nvtx; i++)
	    {
		GA_Offset	v = face->fastVertexOffset(i);
		ppt = getParent()->appendPointOffset();
		face->wireVertex(v, ppt);
		wranglers.getPoint().copyAttributeValues(ppt, vertexPoint(i));
		wranglers.getVertex().copyAttributeValues(v, fastVertexOffset(i));
	    }
	}
    }
    return clone;
}

void
GEO_PrimTetra::copyUnwiredForMerge(const GA_Primitive *prim_src,
				       const GA_MergeMap &map)
{
    UT_ASSERT( prim_src != this );

    const GEO_PrimTetra *src = static_cast<const GEO_PrimTetra *>(prim_src);

    // TODO: It's unfortunate that our constructor always allocates the
    //       vertices.  An alternative that avoids this creation and
    //       immediate destruction of these short lived vertices is to
    //       implement a custom constructor for merging and register it
    //       with our GA_PrimitiveDefinition.
    //
    //       See GA_PrimitiveDefinition::setMergeConstructor() for more.
    for (int i = 0; i < PT_PER_TET; i++)
    {
	if (fastVertexOffset(i) != GA_INVALID_OFFSET)
	    destroyVertex(fastVertexOffset(i));
    }

    if (map.isIdentityMap(GA_ATTRIB_VERTEX))
    {
	for (int i = 0; i < PT_PER_TET; i++)
	    myVertexOffsets[i] = src->myVertexOffsets[i];
    }
    else
    {
	int n = PT_PER_TET;
	for (int i = 0; i < n; i++)
	{
	    // Get source index
	    GA_Offset sidx = src->fastVertexOffset(i);
	    // Map to dest
	    myVertexOffsets[i] = map.mapDestFromSource(GA_ATTRIB_VERTEX, sidx);
	}
    }
}

void
GEO_PrimTetra::swapVertexOffsets(const GA_Defragment &defrag)
{
    GA_Size		nchanged = 0;
    for (int i = 0; i < PT_PER_TET; i++)
    {
	GA_Offset	v = fastVertexOffset(i);
	if (defrag.hasOffsetChanged(v))
	{
	    myVertexOffsets[i] = defrag.mapOffset(v);
	    nchanged++;
	}
    }
}

GEO_PrimTetra *
GEO_PrimTetra::build(GA_Detail *gdp, bool appendPoints)
{
    GEO_PrimTetra *tet = static_cast<GEO_PrimTetra *>(gdp->appendPrimitive(GEO_PrimTetra::theTypeId()));

    // NOTE: By design, npts will always be 4 for a tetrahedron.
    const GA_Size npts = tet->getVertexCount();
    if (appendPoints)
    {
        GEO_Primitive *prim = tet;
        const GA_Offset startptoff = gdp->appendPointBlock(npts);
        for (GA_Size i = 0; i < npts; i++)
        {
            prim->setPointOffset(i, startptoff+i);
        }
    }
    return tet;
}

namespace {

class geo_SetTopoPrimsParallel
{
public:
    geo_SetTopoPrimsParallel(GA_ATITopology *topology, const GA_Offset startprim,
            const GA_Offset startvtx, const GA_Size ntets)
        : myTopology(topology)
        , myStartPrim(startprim)
        , myStartVtx(startvtx)
        , myNumTets(ntets)
    {}

    void operator()(const GA_SplittableRange &r) const
    {
        GA_Offset start;
        GA_Offset end;
        for (GA_Iterator it = r.begin(); it.blockAdvance(start, end); )
        {
            GA_Size relativestart = start - myStartVtx;
            GA_Size relativeend = end - myStartVtx;
            GA_Offset tet  = (relativestart / PT_PER_TET) + myStartPrim;
            GA_Size vert = relativestart % PT_PER_TET;
            GA_Offset endtet = (relativeend / PT_PER_TET) + myStartPrim;

            // The range may start after the beginning of the first tet
            if (vert != 0)
            {
                for (; vert < PT_PER_TET; ++vert)
                    myTopology->setLink(start++, tet);
                ++tet;
            }

            // Full tets in the middle
            for (; tet != endtet; ++tet)
            {
                for (GA_Size i = 0; i < PT_PER_TET; ++i)
                    myTopology->setLink(start++, tet);
            }

            // The range may end before the end of the last tet
            if (start < end)
            {
                while (start < end)
                    myTopology->setLink(start++, tet);
            }
        }
    }

private:
    GA_ATITopology *myTopology;
    const GA_Offset myStartPrim;
    const GA_Offset myStartVtx;
    const GA_Size myNumTets;
};

}

// Instantiation here, because GCC can't figure out templates, even in header files
template void UTparallelForLightItems(const UT_BlockedRange<GA_Size> &, const GEO_PrimTetra::geo_SetVertexListsParallel &);

GA_Offset
GEO_PrimTetra::buildBlock(GA_Detail *detail,
                        const GA_Offset startpt,
                        const GA_Size npoints,
                        const GA_Size ntets,
                        const int *tetpointnumbers)
{
    if (ntets == 0)
        return GA_INVALID_OFFSET;

    TIMING_DEF;

    const GA_Offset endpt = startpt + npoints;
    const GA_Size nvertices = ntets * PT_PER_TET;

    // Create the empty primitives
    const GA_Offset startprim = detail->appendPrimitiveBlock(GEO_PrimTetra::theTypeId(), ntets);

    TIMING_LOG("Creating primitives");

    // Create the uninitialized vertices
    const GA_Offset startvtx = detail->appendVertexBlock(nvertices);
    const GA_Offset endvtx = startvtx + nvertices;

    TIMING_LOG("Appending vertices");

    // Set the vertex-to-point mapping
    GA_ATITopology *vertexToPoint = detail->getTopology().getPointRef();
    if (vertexToPoint)
    {
        UTparallelForLightItems(GA_SplittableRange(GA_Range(detail->getVertexMap(), startvtx, endvtx)),
                geo_SetTopoMappedParallel(vertexToPoint, startpt, startvtx, tetpointnumbers));

        TIMING_LOG("Setting vtx->pt");
    }

    // Set the vertex-to-primitive mapping
    GA_ATITopology *vertexToPrim = detail->getTopology().getPrimitiveRef();
    if (vertexToPrim)
    {
        UTparallelForLightItems(GA_SplittableRange(GA_Range(detail->getVertexMap(), startvtx, endvtx)),
                geo_SetTopoPrimsParallel(vertexToPrim, startprim, startvtx, ntets));

        TIMING_LOG("Setting vtx->prm");
    }

    // Set the vertex lists of the polygons
    UTparallelForLightItems(UT_BlockedRange<GA_Size>(0, ntets),
            geo_SetVertexListsParallel(detail, startprim, startvtx));

    TIMING_LOG("Setting vertex lists");

    // Check whether the linked list topologies need to be set
    GA_ATITopology *pointToVertex = detail->getTopology().getVertexRef();
    GA_ATITopology *vertexToNext = detail->getTopology().getVertexNextRef();
    GA_ATITopology *vertexToPrev = detail->getTopology().getVertexPrevRef();
    if (pointToVertex && vertexToNext && vertexToPrev)
    {
        // Create a trivial map from 0 to nvertices
        UT_IntArray map(nvertices, nvertices);

        TIMING_LOG("Allocating map");

        UTparallelForLightItems(UT_BlockedRange<GA_Size>(0, nvertices), geo_TrivialArrayParallel(map));

        TIMING_LOG("Creating trivial map");

        // Sort the map in parallel according to the point offsets of the vertices
        UTparallelSort(map.array(), map.array() + nvertices, geo_VerticesByPointCompare<true>(tetpointnumbers));

        TIMING_LOG("Sorting array map");

        // Create arrays for the next vertex and prev vertex in parallel
        // If we wanted to do this in geo_LinkToposParallel, we would first
        // have to create an inverse map anyway to find out where vertices
        // are in map, so this saves re-traversing things.
        UT_IntArray nextvtxarray(nvertices, nvertices);
        UT_IntArray prevvtxarray(nvertices, nvertices);
        UT_Lock lock;
        UTparallelForLightItems(UT_BlockedRange<GA_Size>(0, nvertices),
                geo_NextPrevParallel(map, tetpointnumbers, nextvtxarray, prevvtxarray, startvtx, startpt, pointToVertex, vertexToPrev, lock));

        TIMING_LOG("Finding next/prev");

        // Set the point-to-vertex topology in parallel
        // This needs to be done after constructing the next/prev array,
        // because it checks for existing vertices using the points.
        UTparallelForLightItems(GA_SplittableRange(GA_Range(detail->getPointMap(), startpt, endpt)),
                geo_Pt2VtxTopoParallel(pointToVertex, map, tetpointnumbers, startvtx, startpt));

        TIMING_LOG("Setting pt->vtx");

        // Clear up some memory before filling up the linked list topologies
        map.setCapacity(0);

        TIMING_LOG("Clearing map");

        // Fill in the linked list topologies
        UTparallelForLightItems(GA_SplittableRange(GA_Range(detail->getVertexMap(), startvtx, endvtx)),
                geo_LinkToposParallel(vertexToNext, vertexToPrev, nextvtxarray, prevvtxarray, startvtx));

        TIMING_LOG("Setting links");
    }

    return startprim;
}

// Static callback for our factory.
static GA_Primitive *
geo_newPrimTetra(GA_Detail &detail, GA_Offset offset,
	const GA_PrimitiveDefinition &)
{
    return new GEO_PrimTetra(detail, offset);
}

static GA_Primitive *
geo_MergeConstructor(
    const GA_MergeMap &map,
    GA_Detail &detail,
    GA_Offset offset,
    const GA_Primitive &src)
{
    return new GEO_PrimTetra(
        map,
        detail,
        offset,
        static_cast<const GEO_PrimTetra &>(src));
}

void
GEO_PrimTetra::registerMyself(GA_PrimitiveFactory *factory)
{
    // Ignore double registration
    if (theDef)
	return;

    theDef = factory->registerDefinition("HDK_Tetrahedron",
			geo_newPrimTetra,
			GA_FAMILY_NONE);

    theDef->setLabel("hdk_tetrahedron");
    theDef->setMergeConstructor(geo_MergeConstructor);
    // NOTE: Calling setHasLocalTransform(false) is redundant,
    //       but if your custom primitive has a local transform,
    //       it must call setHasLocalTransform(true).
    theDef->setHasLocalTransform(false);
    registerIntrinsics(*theDef);

#ifndef TETRA_GR_PRIMITIVE
    
    // Register the GT tesselation too (now we know what type id we have)
    GT_PrimTetraCollect::registerPrimitive(theDef->getId());
    
#else

#ifdef TETRA_GR_PRIM_COLLECTION
    GUI_PrimitiveHookFlags collect_type = GUI_HOOK_FLAG_COLLECT_PRIMS;
#else
    GUI_PrimitiveHookFlags collect_type = GUI_HOOK_FLAG_NONE;
#endif

    // Since we're only registering one hook, the priority does not matter.
    const int hook_priority = 0;
    
    DM_RenderTable::getTable()->registerGEOHook(new GR_PrimTetraHook,
						theDef->getId(),
						hook_priority, 
						collect_type);
#endif
    
}

int64
GEO_PrimTetra::getMemoryUsage() const
{
    // NOTE: The only memory owned by this primitive is itself.
    int64 mem = sizeof(*this);
    return mem;
}

void
GEO_PrimTetra::countMemory(UT_MemoryCounter &counter) const
{
    // NOTE: There's no shared memory in this primitive.
    counter.countUnshared(sizeof(*this));
}

static GEO_Primitive *
geo_buildPoly(GEO_PrimTetra *tet, GEO_Detail *gdp, int v1, int v2, int v3,
              GEO_ConvertParms &parms)
{
    GEO_PrimPoly *poly = GEO_PrimPoly::build(gdp, 3, false, false);

    parms.getWranglers().getPrimitive().copyAttributeValues(poly->getMapOffset(), tet->getMapOffset());
    if (parms.preserveGroups)
        parms.getGroupWranglers().getPrimitive().copyAttributeValues(poly->getMapOffset(), tet->getMapOffset());

    GA_VertexWrangler &wrangler = parms.getWranglers().getVertex();
    gdp->copyVertex(poly->getVertexOffset(0), tet->getVertexOffset(v1), wrangler, NULL);
    gdp->copyVertex(poly->getVertexOffset(1), tet->getVertexOffset(v2), wrangler, NULL);
    gdp->copyVertex(poly->getVertexOffset(2), tet->getVertexOffset(v3), wrangler, NULL);

    return poly;
}

GEO_Primitive *
GEO_PrimTetra::convertNew(GEO_ConvertParms &parms)
{
    GEO_Primitive *prim = NULL;
    GEO_Detail *gdp = getParent();

    if (parms.toType() == GEO_PrimTypeCompat::GEOPRIMPOLY)
    {
        geo_buildPoly(this, gdp, 0, 1, 2, parms);
        geo_buildPoly(this, gdp, 1, 3, 2, parms);
        geo_buildPoly(this, gdp, 1, 0, 3, parms);
        prim = geo_buildPoly(this, gdp, 0, 2, 3, parms);
    }

    return prim;
}

GEO_Primitive *
GEO_PrimTetra::convert(GEO_ConvertParms &parms, GA_PointGroup *usedpts)
{
    GEO_Primitive	*prim = convertNew(parms);
    GEO_Detail		*gdp  = getParent();
    GA_PrimitiveGroup	*group;

    if (prim)
    {
	if (usedpts) addPointRefToGroup(*usedpts);
	if (group = parms.getDeletePrimitives())
	     group->add(this);
	else gdp->deletePrimitive(*this, !usedpts);
    }
    return prim;
}

void
GEO_PrimTetra::normal(NormalComp &output) const
{
    // No need here.
}

int
GEO_PrimTetra::intersectRay(const UT_Vector3 &org, const UT_Vector3 &dir,
		float tmax, float , float *distance,
		UT_Vector3 *pos, UT_Vector3 *nml,
		int, float *, float *, int) const
{
    // TODO: Check each of the 4 triangles for intersection,
    //       instead of just checking the bounding box.

    UT_BoundingBox bbox;
    getBBox(&bbox);

    float dist;
    int result =  bbox.intersectRay(org, dir, tmax, &dist, nml);
    if (result)
    {
	if (distance) *distance = dist;
	if (pos) *pos = org + dist * dir;
    }
    return result;
}

// This is the usual DSO hook.
extern "C" {
void
newGeometryPrim(GA_PrimitiveFactory *factory)
{
    GEO_PrimTetra::registerMyself(factory);
}
}

// Implement intrinsic attributes
enum
{
    geo_INTRINSIC_ADDRESS,	// Return the address of the primitive
    geo_INTRINSIC_AUTHOR,	// Developer's name
    geo_NUM_INTRINSICS		// Number of intrinsics
};

namespace
{
    static int64
    intrinsicAddress(const GEO_PrimTetra *prim)
    {
	// An intrinsic attribute which returns the address of the primitive
	return (int64)prim;
    }
    static const char *
    intrinsicAuthor(const GEO_PrimTetra *)
    {
	// An intrinsic attribute which returns the HDK author's name
	return "My Name";
    }
};

// Start defining intrinsic attributes, we pass our class name and the number
// of intrinsic attributes.
GA_START_INTRINSIC_DEF(GEO_PrimTetra, geo_NUM_INTRINSICS)

    // See GA_IntrinsicMacros.h for further information on how to define
    // intrinsic attribute evaluators.
    GA_INTRINSIC_I(GEO_PrimTetra, geo_INTRINSIC_ADDRESS, "address",
		intrinsicAddress)
    GA_INTRINSIC_S(GEO_PrimTetra, geo_INTRINSIC_AUTHOR, "author",
		intrinsicAuthor)

// End intrinsic definitions (our class and our base class)
GA_END_INTRINSIC_DEF(GEO_PrimTetra, GEO_Primitive)
