/*
 * Copyright (c) 2017
 *	Side Effects Software Inc.  All rights reserved.
 *
 * Redistribution and use of Houdini Development Kit samples in source and
 * binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. The name of Side Effects Software may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SIDE EFFECTS SOFTWARE `AS IS' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL SIDE EFFECTS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *----------------------------------------------------------------------------
 */

#include "GT_GEOPackedSphere.h"
#include <GT/GT_GEOPrimCollectBoxes.h>
#include <GT/GT_GEOPrimPacked.h>
#include <GT/GT_RefineParms.h>
#include <GU/GU_PrimPacked.h>

using namespace HDK_Sample;

namespace
{
    class PackedCollect : public GT_GEOPrimCollectData
    {
    public:
	PackedCollect(
	    const GT_GEODetailListHandle &geometry,
	    const GT_RefineParms *parms)
	    : GT_GEOPrimCollectData()
	    , myBoxes(geometry, true)
	    , myUseLOD(GT_RefineParms::getPackedViewportLOD(parms))
	{
	}
	GEO_ViewportLOD		lod(const GU_PrimPacked *pack) const
	{
	    if (myUseLOD)
	    {
		return pack->viewportLOD();
	    }
	    return GEO_VIEWPORT_FULL;
	}

	GT_GEOPrimCollectBoxes	myBoxes;
	bool			myUseLOD;
    };
}

void
GT_GEOPackedSphere::registerPrimitive(const GA_PrimitiveTypeId &id)
{
    new GT_GEOPackedSphere(id);
}

GT_GEOPackedSphere::GT_GEOPackedSphere(const GA_PrimitiveTypeId &id)
{
    bind(id);
}

GT_GEOPackedSphere::~GT_GEOPackedSphere()
{
}

GT_GEOPrimCollectData *
GT_GEOPackedSphere::beginCollecting(const GT_GEODetailListHandle &g,
	const GT_RefineParms *parms) const
{
    return new PackedCollect(g, parms);
}

GT_PrimitiveHandle
GT_GEOPackedSphere::collect(const GT_GEODetailListHandle &g,
	const GEO_Primitive *const* prim, int nseg,
	GT_GEOPrimCollectData *data) const
{
    PackedCollect	*c = data->asPointer<PackedCollect>();
    const GU_PrimPacked *packed = UTverify_cast<const GU_PrimPacked *>(prim[0]);
    GEO_ViewportLOD	 lod = c->lod(packed);

    switch (lod)
    {
	case GEO_VIEWPORT_HIDDEN:
	    return GT_PrimitiveHandle();
	case GEO_VIEWPORT_CENTROID:
	case GEO_VIEWPORT_BOX:
	{
	    UT_BoundingBox	box;
	    UT_Matrix4D		m4d;

	    packed->getUntransformedBounds(box);
	    packed->getFullTransform4(m4d);
	    if (lod == GEO_VIEWPORT_CENTROID)
		c->myBoxes.appendCentroid(box, m4d,
			   packed->getMapOffset(),
			   packed->getVertexOffset(0),
			   packed->getPointOffset(0));
	    else
		c->myBoxes.appendBox(box, m4d,
			   packed->getMapOffset(),
			   packed->getVertexOffset(0),
			   packed->getPointOffset(0));
	    return GT_PrimitiveHandle();
	}
	default:
	    break;
    }
    // Full or point cloud
    return GT_PrimitiveHandle(new GT_GEOPrimPacked(g->getGeometry(0), packed));
}

GT_PrimitiveHandle
GT_GEOPackedSphere::endCollecting(const GT_GEODetailListHandle &g,
	GT_GEOPrimCollectData *data) const
{
    PackedCollect	*c = data->asPointer<PackedCollect>();
    return c->myBoxes.getPrimitive();
}

