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
 * NAME:	GEO_PrimTetra.h ( GEO Library, C++)
 *
 * COMMENTS:	This is the base class for all triangle mesh types.
 */

#ifndef __HDK_GEO_PrimTetra__
#define __HDK_GEO_PrimTetra__

#include <GEO/GEO_Primitive.h>
#include <GA/GA_Detail.h>
#include <GA/GA_PrimitiveDefinition.h>
#include <GA/GA_Types.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_ParallelUtil.h>
#include <UT/UT_Vector3.h>

class GA_Detail;

namespace HDK_Sample {

class GEO_PrimTetra : public GEO_Primitive
{
protected:
    /// NOTE: The destructor should only be called from subclass
    ///       destructors.
    virtual ~GEO_PrimTetra();

public:
    /// NOTE: The constructor should only be called from subclass
    ///       constructors.
    GEO_PrimTetra(GA_Detail &d, GA_Offset offset = GA_INVALID_OFFSET);

    /// NOTE: The constructor should only be called from subclass
    ///       constructors.
    /// This one is for merging, and *must* be threadsafe!
    /// It only needs to be provided if the main constructor modifies
    /// the detail or is thread-unsafe in other ways.
    GEO_PrimTetra(const GA_MergeMap &map,
                  GA_Detail &detail,
                  GA_Offset offset,
                  const GEO_PrimTetra &src);

    /// @{
    /// Required interface methods
    virtual bool  	isDegenerate() const;
    virtual int		getBBox(UT_BoundingBox *bbox) const;
    virtual void	reverse();
    virtual UT_Vector3  computeNormal() const;
    virtual void	copyPrimitive(const GEO_Primitive *src);
    virtual void	copyUnwiredForMerge(const GA_Primitive *src,
					    const GA_MergeMap &map);

    // Query the number of vertices in the array. This number may be smaller
    // than the actual size of the array.
    virtual GA_Size	getVertexCount() const;
    virtual GA_Offset	getVertexOffset(GA_Size index) const
			    { return fastVertexOffset(index); }

    // Take the whole set of points into consideration when applying the
    // point removal operation to this primitive. The method returns 0 if
    // successful, -1 if it failed because it would have become degenerate,
    // and -2 if it failed because it would have had to remove the primitive
    // altogether.
    virtual int		 detachPoints(GA_PointGroup &grp);
    /// Before a point is deleted, all primitives using the point will be
    /// notified.  The method should return "false" if it's impossible to
    /// delete the point.  Otherwise, the vertices should be removed.
    virtual GA_DereferenceStatus        dereferencePoint(GA_Offset point,
						bool dry_run=false);
    virtual GA_DereferenceStatus        dereferencePoints(
						const GA_RangeMemberQuery &pt_q,
						bool dry_run=false);
    virtual const GA_PrimitiveJSON	*getJSON() const;

    /// Defragmentation
    virtual void	swapVertexOffsets(const GA_Defragment &defrag);

    /// Evalaute a point given a u,v coordinate (with derivatives)
    virtual bool	evaluatePointRefMap(GA_Offset result_vtx,
				GA_AttributeRefMap &hlist,
				fpreal u, fpreal v, uint du, uint dv) const;
    /// Evalaute position given a u,v coordinate (with derivatives)
    virtual int		evaluatePointV4( UT_Vector4 &pos, float u, float v = 0,
					unsigned du=0, unsigned dv=0) const
			 {
			    return GEO_Primitive::evaluatePoint(pos, u, v,
					du, dv);
			 }
    /// @}

    /// @{
    /// Though not strictly required (i.e. not pure virtual), these methods
    /// should be implemented for proper behaviour.
    virtual GEO_Primitive	*copy(int preserve_shared_pts = 0) const;

    // Have we been deactivated and stashed?
    virtual void	stashed(bool beingstashed, GA_Offset offset=GA_INVALID_OFFSET);

    // We need to invalidate the vertex offsets
    virtual void	clearForDeletion();
    /// @}

    /// @{
    /// Optional interface methods.  Though not required, implementing these
    /// will give better behaviour for the new primitive.
    virtual UT_Vector3	baryCenter() const;
    virtual fpreal	calcVolume(const UT_Vector3 &refpt) const;
    virtual fpreal	calcArea() const;
    virtual fpreal	calcPerimeter() const;
    /// @}

    /// Load the order from a JSON value
    bool		loadOrder(const UT_JSONValue &p);

    /// @{
    /// Save/Load vertex list to a JSON stream
    bool		saveVertexArray(UT_JSONWriter &w,
				const GA_SaveMap &map) const;
    bool		loadVertexArray(UT_JSONParser &p,
				const GA_LoadMap &map);
    /// @}

    /// Method to perform quick lookup of vertex without the virtual call
    GA_Offset	fastVertexOffset(GA_Size index) const
    {
	UT_ASSERT_P(index < 4);
	return (GA_Offset) myVertexOffsets[index];
    }

    /// Both methods find the index of the given vertex (vtx or the vertex
    /// that contains ppt). Return -1 if not found. 
    GA_Size	findVertex(GA_Offset vtxoff) const
    { 
	for (GA_Size i = 0; i < 4; i++)
	{
	    if (fastVertexOffset(i) == vtxoff)
		return i;
	}
	return -1;
    }
    GA_Size	findPoint(GA_Offset ptoff) const
    {
	for (GA_Size i = 0; i < 4; i++)
	{
	    if (vertexPoint(i) == ptoff)
		return i;
	}
	return -1;
    }
    void	setVertexPoint(GA_Size i, GA_Offset pt)
    {
	if (i < 4)
	    wireVertex(fastVertexOffset(i), pt);
    }

    /// @warning vertexPoint() doesn't check the bounds.  Use with caution.
    GA_Offset		vertexPoint(GA_Size i) const
    { return getDetail().vertexPoint(fastVertexOffset(i)); }

    /// Report approximate memory usage.
    virtual int64 getMemoryUsage() const;

    /// Count memory usage using a UT_MemoryCounter in order to count
    /// shared memory correctly.
    /// NOTE: This should always include sizeof(*this).
    virtual void countMemory(UT_MemoryCounter &counter) const;

    /// Allows you to find out what this primitive type was named.
    static GA_PrimitiveTypeId	 theTypeId() { return theDef->getId(); }

    /// Must be invoked during the factory callback to add us to the
    /// list of primitives
    static void		registerMyself(GA_PrimitiveFactory *factory);

    virtual const GA_PrimitiveDefinition &getTypeDef() const
    { return *theDef; }

    /// @{
    /// Conversion functions
    virtual GEO_Primitive *convert(GEO_ConvertParms &parms,
                                   GA_PointGroup *usedpts = 0);
    virtual GEO_Primitive *convertNew(GEO_ConvertParms &parms);
    /// @}

    /// Optional build function
    static GEO_PrimTetra *build(GA_Detail *gdp, bool appendpts = true);

    /// Builds tetrahedrons using the specified range of point offsets,
    /// as dictated by ntets and tetpointnumbers, in parallel.
    /// tetpointnumbers lists the *offsets* of the points used by
    /// each tetrahedron *MINUS* startpt, i.e. they are offsets relative to
    /// startpt, *not* indices relative to startpt. The offset of the first
    /// tetrahedron is returned, and the rest are at consecutive offsets. All
    /// tetpointnumbers must be between 0 (inclusive) and npoints (exclusive).
    ///
    /// NOTE: Existing primitives *are* allowed to be using the points in
    /// the specified range already, and the tetrahedrons being created do not
    /// do not need to use all of the points in the range.  However,
    /// these cases may impact performance.
    static GA_Offset    buildBlock(GA_Detail *detail,
                                   const GA_Offset startpt,
                                   const GA_Size npoints,
                                   const GA_Size ntets,
                                   const int *tetpointnumbers);

    virtual void	normal(NormalComp &output) const;

    virtual int		intersectRay(const UT_Vector3 &o, const UT_Vector3 &d,
				float tmax = 1E17F, float tol = 1E-12F,
				float *distance = 0, UT_Vector3 *pos = 0,
				UT_Vector3 *nml = 0, int accurate = 0,
				float *u = 0, float *v = 0,
				int ignoretrim = 1) const;

    // NOTE: This functor class must be in the scope of GEO_PrimTetra
    //       so that it can access myVertexOffsets.
    //       It has to be public so that UTparallelForLightItems
    //       can be instantiated for GCC with it.
    class geo_SetVertexListsParallel
    {
    public:
        geo_SetVertexListsParallel(GA_Detail *detail, const GA_Offset startprim,
                const GA_Offset startvtx)
            : myPrimitiveList(detail->getPrimitiveList())
            , myStartPrim(startprim)
            , myStartVtx(startvtx)
        {}

        void operator()(const UT_BlockedRange<GA_Size> &r) const
        {
            char          bcnt = 0;
            UT_Interrupt *boss = UTgetInterrupt();
            
            GA_Offset vtxoff = myStartVtx + 4*r.begin();
            for (GA_Size i = r.begin(); i != r.end(); ++i)
            {
                if (!bcnt++ && boss->opInterrupt())
                    break;
                GA_Offset offset = myStartPrim + i;
                GEO_PrimTetra *tet = (GEO_PrimTetra *)myPrimitiveList.get(offset);
                for (int j = 0; j < 4; ++j)
                    tet->myVertexOffsets[j] = GA_Size(vtxoff++);
            }
        }

    private:
        GA_PrimitiveList &myPrimitiveList;
        const GA_Offset myStartPrim;
        const GA_Offset myStartVtx;
    };

protected:
    /// Declare methods for implementing intrinsic attributes.
    GA_DECLARE_INTRINSICS()

    /// This is designed to match with GA_OffsetList which also
    /// uses only 32bit storage for now.
    int32			myVertexOffsets[4];

private:
    static GA_PrimitiveDefinition	 *theDef;
};

}

#endif
