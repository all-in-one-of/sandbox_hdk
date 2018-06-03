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
 * NAME:	GEO_PrimTetra.C (HDK Sample, C++)
 *
 * COMMENTS:	This is an HDK example for making a custom tetrahedron primitive type.
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

GA_PrimitiveDefinition *GEO_PrimTetra::theDefinition = nullptr;

// So it doesn't look like a magic number
#define PT_PER_TET 4

GEO_PrimTetra::GEO_PrimTetra(GA_Detail &d, GA_Offset offset)
    : GEO_Primitive(&d, offset)
{
    // NOTE: There used to be an option to use a separate "merge constructor"
    //       to allow the regular constructor to add vertices to the detail,
    //       but this is no longer allowed.  This constructor *must* be safe
    //       to call from multiple threads on the same detail at the same time.
}

GEO_PrimTetra::~GEO_PrimTetra()
{
    // This class doesn't have anything that needs explicit destruction,
    // apart from what's done in the automatically-called base class
    // destructor.  If you add anything that needs to be explicitly
    // cleaned up, do it here, and make sure that it's safe to be
    // done to multiple primitives of the same detail at the same time.
}

void
GEO_PrimTetra::stashed(bool beingstashed, GA_Offset offset)
{
    GEO_Primitive::stashed(beingstashed, offset);

    // This function is used as a way to "stash" and "unstash" a primitive
    // as an alternative to freeing and reallocating a primitive.
    // This class doesn't have any data apart from what's in the base class,
    // but if you add any data, make sure that when beingstashed is true,
    // any allocated memory gets freed up, and when beingstashed is false,
    // (which will only happen after having first been called with true),
    // make sure that the primitive is initialized to a state that is as
    // if it were freshly constructed.
    // It should be threadsafe to call this on multiple primitives
    // in the same detail at the same time.
}

bool
GEO_PrimTetra::evaluatePointRefMap(GA_Offset result_vtx,
				GA_AttributeRefMap &map,
				fpreal u, fpreal v, unsigned du,
				unsigned dv) const
{
    // The parameter space of a tetrahedron requires 3 coordinates,
    // (u,v,w), not just (u,v), so this just falls back to picking
    // the first vertex's values.
    if (du==0 && dv==0)
    {
        map.copyValue(GA_ATTRIB_VERTEX, result_vtx,
                      GA_ATTRIB_VERTEX, getVertexOffset(0));
    }
    else
    {
        // Any derivative of a constant value is zero.
        map.zeroElement(GA_ATTRIB_VERTEX, result_vtx);
    }
    return true;
}


void
GEO_PrimTetra::reverse()
{
    // NOTE: We cannot reverse all of the vertices, else the sense
    //       of the tet is left unchnaged!
    GA_Size r = 0;
    GA_Size nr = 3 - r;
    GA_Offset other = myVertexList.get(nr);
    myVertexList.set(nr, myVertexList.get(r));
    myVertexList.set(r, other);
}

UT_Vector3
GEO_PrimTetra::computeNormal() const
{
    return UT_Vector3(0, 0, 0);
}

fpreal
GEO_PrimTetra::calcVolume(const UT_Vector3 &) const
{
    UT_Vector3 v0 = getPos3(0);
    UT_Vector3 v1 = getPos3(1);
    UT_Vector3 v2 = getPos3(2);
    UT_Vector3 v3 = getPos3(3);

    // Measure signed volume of pyramid (v0,v1,v2,v3) 
    float signedvol = -(v3-v0).dot(cross(v1-v0, v2-v0));

    signedvol /= 6;

    return signedvol;
}

fpreal
GEO_PrimTetra::calcArea() const
{
    fpreal area = 0; // Signed area

    UT_Vector3 v0 = getPos3(0);
    UT_Vector3 v1 = getPos3(1);
    UT_Vector3 v2 = getPos3(2);
    UT_Vector3 v3 = getPos3(3);

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

    UT_Vector3 v0 = getPos3(0);
    UT_Vector3 v1 = getPos3(1);
    UT_Vector3 v2 = getPos3(2);
    UT_Vector3 v3 = getPos3(3);

    length += (v1-v0).length();
    length += (v2-v0).length();
    length += (v3-v0).length();
    length += (v2-v1).length();
    length += (v3-v1).length();
    length += (v3-v2).length();

    return length;
}

int
GEO_PrimTetra::detachPoints(GA_PointGroup &grp)
{
    GA_Size count = 0;
    for (GA_Size i = 0; i < PT_PER_TET; ++i)
	if (grp.containsOffset(getPointOffset(i)))
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
	if (getPointOffset(i) == point)
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
	if (point_query.contains(getPointOffset(i)))
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
    static GA_PrimitiveJSON *theJSON = NULL;

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
    return myVertexList.jsonVertexArray(w, map);
}

bool
GEO_PrimTetra::loadVertexArray(UT_JSONParser &p, const GA_LoadMap &map)
{
    GA_Offset startvtxoff = map.getVertexOffset();

    int64 vtxoffs[PT_PER_TET];
    int nvertex = p.parseUniformArray(vtxoffs, PT_PER_TET);
    if (startvtxoff != GA_Offset(0))
    {
        for (int i = 0; i < nvertex; i++)
        {
            if (vtxoffs[i] >= 0)
                vtxoffs[i] += GA_Size(startvtxoff);
        }
    }
    for (int i = nvertex; i < PT_PER_TET; ++i)
        vtxoffs[i] = GA_INVALID_OFFSET;
    myVertexList.set(vtxoffs, PT_PER_TET, GA_Offset(0));
    if (nvertex < PT_PER_TET)
	return false;
    return true;
}


int
GEO_PrimTetra::getBBox(UT_BoundingBox *bbox) const
{
    bbox->initBounds(getPos3(0));
    bbox->enlargeBounds(getPos3(1));
    bbox->enlargeBounds(getPos3(2));
    bbox->enlargeBounds(getPos3(3));
    return 1;
}

UT_Vector3
GEO_PrimTetra::baryCenter() const
{
    UT_Vector3 sum(0,0,0);

    for (int i = 0; i < PT_PER_TET; ++i)
        sum += getPos3(i);

    sum /= PT_PER_TET;

    return sum;
}

bool
GEO_PrimTetra::isDegenerate() const
{
    // Duplicate points means degenerate.
    for (int i = 0; i < PT_PER_TET; i++)
    {
	for (int j = i+1; j < PT_PER_TET; j++)
	{
	    if (getPointOffset(i) == getPointOffset(j))
		return true;
	}
    }
    return false;
}

void
GEO_PrimTetra::copyPrimitive(const GEO_Primitive *psrc)
{
    if (psrc == this)
        return;

    // This sets the number of vertices to be the same as psrc, and wires
    // the corresponding vertices to the corresponding points.
    // This class doesn't have any more data, so we didn't need to
    // override copyPrimitive, but if you add any more data, copy it
    // below.
    GEO_Primitive::copyPrimitive(psrc);

    // Uncomment this to access other data
    //const GEO_PrimTetra *src = (const GEO_PrimTetra *)psrc;
}

GEO_Primitive *
GEO_PrimTetra::copy(int preserve_shared_pts) const
{
    GEO_Primitive *clone = GEO_Primitive::copy(preserve_shared_pts);

    if (!clone)
        return nullptr;

    // This class doesn't have any more data to copy, so we didn't need
    // to override this function, but if you add any, copy them here.

    // Uncomment this to access other data
    //GEO_PrimTetra *tet = (GEO_PrimTetra*)clone;

    return clone;
}

void
GEO_PrimTetra::copyUnwiredForMerge(
    const GA_Primitive *prim_src,
    const GA_MergeMap &map)
{
    UT_ASSERT( prim_src != this );

    // This copies the vertex list, with offsets mapped using map.
    GEO_Primitive::copyUnwiredForMerge(prim_src, map);

    // If you add any data that must be copyied from src in order for this
    // to be a separate but identical copy of src, copy it here,
    // but make sure it's safe to call copyUnwiredForMerge on multiple
    // primitives at the same time.

    // Uncomment this to access other data
    //const GEO_PrimTetra *src = static_cast<const GEO_PrimTetra *>(prim_src);
}

GEO_PrimTetra *
GEO_PrimTetra::build(GA_Detail *gdp, bool appendPoints)
{
    GEO_PrimTetra *tet = static_cast<GEO_PrimTetra *>(gdp->appendPrimitive(GEO_PrimTetra::theTypeId()));

    // Add 4 vertices.  The constructor did not add any.
    GA_Offset vtxoff = gdp->appendVertexBlock(PT_PER_TET);
    tet->myVertexList.setTrivial(vtxoff, PT_PER_TET);
    GA_ATITopology *vtx_to_prim = gdp->getTopology().getPrimitiveRef();
    if (vtx_to_prim)
    {
        GA_Offset primoff = tet->getMapOffset();
        for (GA_Size i = 0; i < PT_PER_TET; i++)
            vtx_to_prim->setLink(vtxoff+i, primoff);
    }

    // NOTE: By design, npts will always be 4 (i.e. PT_PER_TET) for a tetrahedron.
    //       The call to getVertexCount() is just as an example.
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

    // Set the vertex lists of the tets
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
static void
geoNewPrimTetraBlock(
    GA_Primitive **new_prims,
    GA_Size nprimitives,
    GA_Detail &gdp,
    GA_Offset start_offset,
    const GA_PrimitiveDefinition &def)
{
    if (nprimitives >= 4*GA_PAGE_SIZE)
    {
        // Allocate them in parallel if we're allocating many.
        // This is using the C++11 lambda syntax to make a functor.
        UTparallelForLightItems(UT_BlockedRange<GA_Offset>(start_offset, start_offset+nprimitives),
            [new_prims,&gdp,start_offset](const UT_BlockedRange<GA_Offset> &r){
                GA_Offset primoff(r.begin());
                GA_Primitive **pprims = new_prims+(primoff-start_offset);
                GA_Offset endprimoff(r.end());
                for ( ; primoff != endprimoff; ++primoff, ++pprims)
                    *pprims = new GEO_PrimTetra(gdp, primoff);
            });
    }
    else
    {
        // Allocate them serially if we're only allocating a few.
        GA_Offset endprimoff(start_offset + nprimitives);
        for (GA_Offset primoff(start_offset); primoff != endprimoff; ++primoff, ++new_prims)
            *new_prims = new GEO_PrimTetra(gdp, primoff);
    }
}

void
GEO_PrimTetra::registerMyself(GA_PrimitiveFactory *factory)
{
    // Ignore double registration
    if (theDefinition)
	return;

    theDefinition = factory->registerDefinition(
        "HDK_Tetrahedron",
        geoNewPrimTetraBlock,
        GA_FAMILY_NONE,
        "hdk_tetrahedron");

    // NOTE: Calling setHasLocalTransform(false) is redundant,
    //       but if your custom primitive has a local transform,
    //       it must call setHasLocalTransform(true).
    theDefinition->setHasLocalTransform(false);
    registerIntrinsics(*theDefinition);

#ifndef TETRA_GR_PRIMITIVE
    
    // Register the GT tesselation too (now we know what type id we have)
    GT_PrimTetraCollect::registerPrimitive(theDefinition->getId());
    
#else

#ifdef TETRA_GR_PRIM_COLLECTION
    GUI_PrimitiveHookFlags collect_type = GUI_HOOK_FLAG_COLLECT_PRIMS;
#else
    GUI_PrimitiveHookFlags collect_type = GUI_HOOK_FLAG_NONE;
#endif

    // Since we're only registering one hook, the priority does not matter.
    const int hook_priority = 0;
    
    DM_RenderTable::getTable()->registerGEOHook(
        new GR_PrimTetraHook,
        theDefinition->getId(),
        hook_priority,
        collect_type);
#endif
    
}

int64
GEO_PrimTetra::getMemoryUsage() const
{
    // NOTE: The only memory owned by this primitive is itself
    //       and its base class.
    int64 mem = sizeof(*this) + getBaseMemoryUsage();
    return mem;
}

void
GEO_PrimTetra::countMemory(UT_MemoryCounter &counter) const
{
    // NOTE: There's no shared memory in this primitive,
    //       apart from possibly in the case class.
    counter.countUnshared(sizeof(*this));
    countBaseMemory(counter);
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
