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

#ifndef __GT_PrimTetra__
#define __GT_PrimTetra__

#include <GT/GT_GEOPrimCollect.h>

namespace HDK_Sample {

/// Hook to handle tesselation of tetra primitives
///
/// When rendering tet primitives, collect all the tet primitives together,
/// then a polygonal mesh is generated for the external hull, removing any
/// shared faces on the inside.
class GT_PrimTetraCollect : public GT_GEOPrimCollect
{
public:
    /// Register the GT collector
    static void	registerPrimitive(const GA_PrimitiveTypeId &id);

    /// Constructor.  The @c id is used to bind the collector to the proper
    /// primitive type.
    GT_PrimTetraCollect(const GA_PrimitiveTypeId &id);

    /// Destructor
    virtual ~GT_PrimTetraCollect();

    /// Return a structure to capture all the tet primitives
    virtual GT_GEOPrimCollectData *
		beginCollecting(const GT_GEODetailListHandle &geometry,
				const GT_RefineParms *parms) const;

    /// When refining a single tet primitive, we add it to the container
    virtual GT_PrimitiveHandle
		collect(const GT_GEODetailListHandle &geometry,
				const GEO_Primitive *const* prim_list,
				int nsegments,
				GT_GEOPrimCollectData *data) const;
    /// At the end of collecting, the single outside skin is generated
    virtual GT_PrimitiveHandle
		endCollecting(const GT_GEODetailListHandle &geometry,
				GT_GEOPrimCollectData *data) const;
private:
    GA_PrimitiveTypeId	myId;
};

}	// End HDK_Sample namespace

#endif
