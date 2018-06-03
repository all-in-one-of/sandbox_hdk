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

#include "GU_PackedSphere.h"
#include <GU/GU_PackedFactory.h>
#include <GU/GU_PrimSphere.h>
#include <GU/GU_PrimPacked.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_MemoryCounter.h>
#include <FS/UT_DSO.h>
#include "GT_GEOPackedSphere.C"

using namespace HDK_Sample;

namespace
{
    static const UT_StringRef	theLodStr = "lod";

    class SphereFactory : public GU_PackedFactory
    {
    public:
	SphereFactory()
	    : GU_PackedFactory("PackedSphere", "Packed Sphere")
	{
	    registerIntrinsic(theLodStr,
		    IntGetterCast(&GU_PackedSphere::lod),
		    IntSetterCast(&GU_PackedSphere::setLOD));
	}
	virtual ~SphereFactory() {}

	virtual GU_PackedImpl	*create() const
	{
	    return new GU_PackedSphere();
	}
    };

    static SphereFactory *theFactory = NULL;

    /// Store spheres in a shared cache
    class CacheEntry
    {
    public:
	CacheEntry(int lod)
	    : myGdp()
	{
	    GU_Detail		*gdp = new GU_Detail();
	    GU_PrimSphereParms	 parms(gdp);
	    parms.freq = lod;
	    GU_PrimSphere::build(parms, GEO_PRIMPOLYSOUP);
	    myGdp.allocateAndSet(gdp);
	};
	~CacheEntry()
	{
	}
	GU_ConstDetailHandle	detail() const
	{
	    return GU_ConstDetailHandle(myGdp);
	}

    private:
	GU_DetailHandle	myGdp;
    };
    // The cache is quite simple, we just support lod's between 1 and 32.
    #define MIN_LOD	1
    #define MAX_LOD	32
    static CacheEntry	*theCache[MAX_LOD+1];
    static UT_Lock	 theLock;

    static GU_ConstDetailHandle
    getSphere(int lod)
    {
	UT_AutoLock	lock(theLock);
	lod = SYSclamp(lod, MIN_LOD, MAX_LOD);
	if (!theCache[lod])
	    theCache[lod] = new CacheEntry(lod);
	return theCache[lod]->detail();
    }
}

GA_PrimitiveTypeId GU_PackedSphere::theTypeId(-1);

GU_PackedSphere::GU_PackedSphere()
    : GU_PackedImpl()
    , myDetail()
    , myLOD(2)
{
}

GU_PackedSphere::GU_PackedSphere(const GU_PackedSphere &src)
    : GU_PackedImpl(src)
    , myDetail()
    , myLOD(0)
{
    setLOD(src.myLOD);
}

GU_PackedSphere::~GU_PackedSphere()
{
    clearSphere();
}

void
GU_PackedSphere::install(GA_PrimitiveFactory *gafactory)
{
    UT_ASSERT(!theFactory);
    if (theFactory)
	return;

    theFactory = new SphereFactory();
    GU_PrimPacked::registerPacked(gafactory, theFactory);
    if (theFactory->isRegistered())
    {
	theTypeId = theFactory->typeDef().getId();
	GT_GEOPackedSphere::registerPrimitive(theTypeId);
    }
    else
    {
	fprintf(stderr, "Unable to register packed sphere from %s\n",
		UT_DSO::getRunningFile());
    }
}

void
GU_PackedSphere::clearSphere()
{
    myDetail = GU_ConstDetailHandle();
}

GU_PackedFactory *
GU_PackedSphere::getFactory() const
{
    return theFactory;
}

GU_PackedImpl *
GU_PackedSphere::copy() const
{
    return new GU_PackedSphere(*this);
}

void
GU_PackedSphere::clearData()
{
    // This method is called when primitives are "stashed" during the cooking
    // process.  However, primitives are typically immediately "unstashed" or
    // they are deleted if the primitives aren't recreated after the fact.
    // We can just leave our data.
}

bool
GU_PackedSphere::isValid() const
{
    return detail().isValid();
}

template <typename T>
void
GU_PackedSphere::updateFrom(const T &options)
{
    int		ival;
    if (import(options, theLodStr, ival))
	setLOD(ival);
}

bool
GU_PackedSphere::save(UT_Options &options, const GA_SaveMap &map) const
{
    options.setOptionI(theLodStr, lod());
    return true;
}

bool
GU_PackedSphere::getBounds(UT_BoundingBox &box) const
{
    // All spheres are unit spheres with transforms applied
    box.initBounds(-1, -1, -1);
    box.enlargeBounds(1, 1, 1);
    // If computing the bounding box is expensive, you may want to cache the
    // box by calling setBoxCache(box)
    // SYSconst_cast(this)->setBoxCache(box);
    return true;
}

bool
GU_PackedSphere::getRenderingBounds(UT_BoundingBox &box) const
{
    // When geometry contains points or curves, the width attributes need to be
    // taken into account when computing the rendering bounds.
    return getBounds(box);
}

void
GU_PackedSphere::getVelocityRange(UT_Vector3 &min, UT_Vector3 &max) const
{
    min = 0;	// No velocity attribute on geometry
    max = 0;
}

void
GU_PackedSphere::getWidthRange(fpreal &min, fpreal &max) const
{
    min = max = 0;	// Width is only important for curves/points.
}

bool
GU_PackedSphere::unpack(GU_Detail &destgdp) const
{
    // This may allocate geometry for the primitive
    GU_DetailHandleAutoReadLock	rlock(getPackedDetail());
    if (!rlock.getGdp())
	return false;
    return unpackToDetail(destgdp, rlock.getGdp());
}

GU_ConstDetailHandle
GU_PackedSphere::getPackedDetail(GU_PackedContext *context) const
{
    if (!detail().isValid() && lod() > 0)
    {
	/// Create the sphere on demand.  If the user only requests the
	/// bounding box (i.e. the viewport LOD is set to "box"), then we never
	/// have to actually create the sphere's geometry.
	GU_PackedSphere		*me = const_cast<GU_PackedSphere *>(this);
	GU_ConstDetailHandle	 dtl = getSphere(lod());

	if (dtl != me->detail())
	{
	    me->setDetail(dtl);
	    getPrim()->getParent()->getPrimitiveList().bumpDataId();
	}
    }
    return detail();
}

int64
GU_PackedSphere::getMemoryUsage(bool inclusive) const
{
    int64 mem = inclusive ? sizeof(*this) : 0;

    // Don't count the (shared) GU_Detail, since that will greatly
    // over-estimate the overall memory usage.
    mem += detail().getMemoryUsage(false);

    return mem;
}

void
GU_PackedSphere::countMemory(UT_MemoryCounter &counter, bool inclusive) const
{
    if (counter.mustCountUnshared())
    {
        size_t mem = inclusive ? sizeof(*this) : 0;
        mem += detail().getMemoryUsage(false);
        UT_MEMORY_DEBUG_LOG("GU_PackedSphere", int64(mem));
        counter.countUnshared(mem);
    }

    // The UT_MemoryCounter interface needs to be enhanced to efficiently count
    // shared memory for details. Skip this for now.
#if 0
    if (detail().isValid())
    {
        GU_DetailHandleAutoReadLock gdh(detail());
        gdh.getGdp()->countMemory(counter, true);
    }
#endif
}

void
GU_PackedSphere::setLOD(exint l)
{
    clearSphere();
    myLOD = l;
    topologyDirty();	// Notify base primitive that topology has changed
}

/// DSO registration callback
void
newGeometryPrim(GA_PrimitiveFactory *f)
{
    GU_PackedSphere::install(f);
}
