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
 * NAME:	GEO_PrimTetra.h (HDK Sample, C++)
 *
 * COMMENTS:	This is an HDK example for making a custom tetrahedron primitive type.
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

namespace HDK_Sample {

class GEO_PrimTetra : public GEO_Primitive
{
protected:
    /// NOTE: The destructor should only be called from GA_PrimitiveList,
    ///       via a GA_Primitive pointer.
    virtual ~GEO_PrimTetra();
public:
    /// NOTE: To create a new primitive owned by the detail, call
    ///       GA_Detail::appendPrimitive or appendPrimitiveBlock on
    ///       the detail, not this constructor.
    GEO_PrimTetra(GA_Detail &d, GA_Offset offset);

    /// @{
    /// Required interface methods
    virtual bool  	isDegenerate() const;
    virtual int		getBBox(UT_BoundingBox *bbox) const;
    virtual void	reverse();
    virtual UT_Vector3  computeNormal() const;
    virtual void	copyPrimitive(const GEO_Primitive *src);
    virtual void	copyUnwiredForMerge(const GA_Primitive *src,
					    const GA_MergeMap &map);

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

    /// Finds where the specified vertex offset is in this primitive's
    /// vertex list.
    GA_Size findVertex(GA_Offset vtxoff) const
    { 
	for (GA_Size i = 0; i < 4; i++)
	{
	    if (getVertexOffset(i) == vtxoff)
		return i;
	}
	return -1;
    }

    /// Finds where in this primitive's vertex list, some vertex
    /// is wired to the specified point offset.
    GA_Size findPoint(GA_Offset ptoff) const
    {
	for (GA_Size i = 0; i < 4; i++)
	{
	    if (getPointOffset(i) == ptoff)
		return i;
	}
	return -1;
    }

    /// Report approximate memory usage.
    virtual int64 getMemoryUsage() const;

    /// Count memory usage using a UT_MemoryCounter in order to count
    /// shared memory correctly.
    /// NOTE: This should always include sizeof(*this).
    virtual void countMemory(UT_MemoryCounter &counter) const;

    /// Allows you to find out what this primitive type was named.
    static const GA_PrimitiveTypeId &theTypeId() { return theDefinition->getId(); }

    /// Must be invoked during the factory callback to add us to the
    /// list of primitives
    static void registerMyself(GA_PrimitiveFactory *factory);

    virtual const GA_PrimitiveDefinition &getTypeDef() const
    { return *theDefinition; }

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
    //       so that it can access myVertexList.
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
                tet->myVertexList.setTrivial(vtxoff, 4);
                vtxoff += 4;
            }
        }

    private:
        GA_PrimitiveList &myPrimitiveList;
        const GA_Offset myStartPrim;
        const GA_Offset myStartVtx;
    };

protected:
    /// Declare methods for implementing intrinsic attributes.
    GA_DECLARE_INTRINSICS(GA_NO_OVERRIDE)

    void createVertices() const;
private:
    static GA_PrimitiveDefinition *theDefinition;
};

}

#endif
