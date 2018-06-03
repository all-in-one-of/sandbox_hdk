/*
 * Copyright (c) 2016
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

#include "GU_PackedSharedData.h"
#include <GA/GA_LoadMap.h>
#include <GA/GA_SaveMap.h>
#include <GU/GU_PackedFactory.h>
#include <GU/GU_PrimPacked.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_MemoryCounter.h>
#include <UT/UT_Date.h>
#include <UT/UT_SysClone.h>
#include <UT/UT_JSONValue.h>
#include <FS/UT_DSO.h>

using namespace HDK_Sample;

namespace
{
    static const UT_StringRef	theDateStr = "creation_date";
    static const UT_StringRef	theHostStr = "creation_host";

    // It's possible to store multiple types of shared data.  Each type of
    // shared data should have it's own identifier.
    static const UT_StringRef	theSharedIdentifier = "hdk:shareddata";

    using SharedInfo = GU_PackedSharedData::SharedInfo;

    /// Class to store/resolve shared data for the primitive
    /// This data is created during loading when resolving the shared data, and
    /// is cleared when all primitives have resolved the shared information.
    ///
    /// @note You can also use GA_SharedLoadDataStat to load data when the
    /// geometry is loading for "stat" (i.e. not a full load).  You would need
    /// to implement GU_PackedFactory::statSharedData();  This can be used for
    /// things like bounding boxes, etc.
    class loadedSharedData : public GA_SharedLoadData
    {
    public:
	loadedSharedData(const UT_SharedPtr<SharedInfo> &data,
			    const UT_StringHolder &key)
	    : myData(data)
	    , myKey(key)
	{
	}
	virtual ~loadedSharedData()
	{
	}

	/// Return the key associated with this shared data
	virtual UT_StringHolder	getSharedDataKey() const
	{
	    return myKey;
	}
	UT_StringHolder			myKey;
	UT_SharedPtr<SharedInfo>	myData;
    };

    // Make a unique key for the shared data item.  Each instance of shared
    // data must have a unique key.
    static UT_StringHolder
    makeSharedDataKey(const UT_SharedPtr<SharedInfo> &date)
    {
	UT_WorkBuffer	key;
	// This could be a GUID or other entities, but in our case, it's pretty
	// simple to just store the address of the pointer.  The keys only need
	// to be unique within a single piece of geometry and we know no other
	// shared items can possibly have the same address.
	if (!date)
	    key.strcpy("<null>");
	else
	{
	    key.sprintf("shdata:%p", date.get());
	    key.lower();
	}
	return UT_StringHolder(key);
    }

    /// Factory for creating the packed primitive
    class PackedPrimFactory : public GU_PackedFactory
    {
    public:
	PackedPrimFactory()
	    : GU_PackedFactory("PackedSharedData", "Packed SharedData")
	{
	    registerIntrinsic(theDateStr,
		    StringHolderGetterCast(&GU_PackedSharedData::creationDate));
	    registerIntrinsic(theHostStr,
		    StringHolderGetterCast(&GU_PackedSharedData::creationHost));
	}
	virtual ~PackedPrimFactory() {}

	virtual GU_PackedImpl	*create() const
	{
	    return new GU_PackedSharedData();
	}

	/// Load the shared data.
	virtual GA_SharedLoadData	*loadSharedData(UT_JSONParser &p,
						const char *type,
						const char *key,
						bool isDelayedLoad) const
	{
	    UT_JSONValue	value;

	    if (!value.parseValue(p))
		return nullptr;

	    auto d = new SharedInfo;
	    if (value.getType() == UT_JSONValue::JSON_MAP)
	    {
		auto	map = value.getMap();
		auto	date = map->get(theDateStr);
		auto	host = map->get(theHostStr);
		if (date && date->getType() == UT_JSONValue::JSON_STRING)
		    d->myTimeStamp = *date->getStringHolder();
		if (host && host->getType() == UT_JSONValue::JSON_STRING)
		    d->myHostname = *host->getStringHolder();
	    }
	    return new loadedSharedData(d, key);
	}
    };

    static PackedPrimFactory *theFactory = NULL;

    // Simple geometry used for the primitive
    static GU_Detail *
    boxGeometry()
    {
	static GU_Detail	*theBox = nullptr;
	static UT_Lock	 theLock;
	UT_DoubleLock<GU_Detail *>	lock(theLock, theBox);
	if (!lock.getValue())
	{
	    GU_Detail	*tmp = new GU_Detail();
	    tmp->cube();
	    lock.setValue(tmp);
	}
	return lock.getValue();
    }
}

GA_PrimitiveTypeId GU_PackedSharedData::theTypeId(-1);

GU_PackedSharedData::SharedInfo::SharedInfo()
{
    UT_WorkBuffer	buf;
    UT_Date::dprintf(buf, "%Y-%m-%d %H:%M:%S", time(nullptr));
    myTimeStamp = buf;

    UT_String		hostname;
    UTgethostname(hostname);
    myHostname = hostname;
}

GU_PackedSharedData::GU_PackedSharedData()
    : GU_PackedImpl()
    , mySharedHandle()
    , mySharedInfo(new SharedInfo)
{
}

GU_PackedSharedData::GU_PackedSharedData(const GU_PackedSharedData &src)
    : GU_PackedImpl(src)
    , mySharedHandle(src.mySharedHandle)
    , mySharedInfo(src.mySharedInfo)
{
    // The copy c-tor just references the shared data.
}

GU_PackedSharedData::~GU_PackedSharedData()
{
}

void
GU_PackedSharedData::install(GA_PrimitiveFactory *gafactory)
{
    UT_ASSERT(!theFactory);
    if (theFactory)
	return;

    theFactory = new PackedPrimFactory();
    GU_PrimPacked::registerPacked(gafactory, theFactory);
    if (theFactory->isRegistered())
    {
	theTypeId = theFactory->typeDef().getId();
    }
    else
    {
	fprintf(stderr, "Unable to register packed shared data from %s\n",
		UT_DSO::getRunningFile());
    }
}

GU_PackedFactory *
GU_PackedSharedData::getFactory() const
{
    return theFactory;
}

GU_PackedImpl *
GU_PackedSharedData::copy() const
{
    return new GU_PackedSharedData(*this);
}

void
GU_PackedSharedData::clearData()
{
}

bool
GU_PackedSharedData::isValid() const
{
    return true;
}

template <typename T>
void
GU_PackedSharedData::updateFrom(const T &options, const GA_LoadMap *loadmap)
{
    UT_StringHolder	key;

    if (loadmap && import(options, theSharedIdentifier, key))
    {
	// This retrieves a handle to the shared data which can be resolved on
	// request.  The load of the shared data will be deffered (i.e. the
	// data is not available immediately).  For example, if the geometry is
	// being loaded from a disk file, it's possible the shared data will
	// not be loaded at all.
	mySharedHandle = loadmap->needSharedData(key,
				    "PackedSharedData", nullptr);
    }
}

bool
GU_PackedSharedData::save(UT_Options &options, const GA_SaveMap &map) const
{
    // Save out shared data information.  We don't save the actual shared data
    // here, we just save a reference to the data that needs to be resolved at
    // load time.
    options.setOptionS(theSharedIdentifier, makeSharedDataKey(mySharedInfo));

    return true;
}

bool
GU_PackedSharedData::getBounds(UT_BoundingBox &box) const
{
    box.initBounds(-1, -1, -1);
    box.enlargeBounds(1, 1, 1);
    return true;
}

bool
GU_PackedSharedData::getRenderingBounds(UT_BoundingBox &box) const
{
    // When geometry contains points or curves, the width attributes need to be
    // taken into account when computing the rendering bounds.
    return getBounds(box);
}

void
GU_PackedSharedData::getVelocityRange(UT_Vector3 &min, UT_Vector3 &max) const
{
    min = 0;	// No velocity attribute on geometry
    max = 0;
}

void
GU_PackedSharedData::getWidthRange(fpreal &min, fpreal &max) const
{
    min = max = 0;	// Width is only important for curves/points.
}

bool
GU_PackedSharedData::unpack(GU_Detail &destgdp) const
{
    unpackToDetail(destgdp, boxGeometry());
    return true;
}

int64
GU_PackedSharedData::getMemoryUsage(bool inclusive) const
{
    int64 mem = inclusive ? sizeof(*this) : 0;
    mem += mySharedInfo->myTimeStamp.getMemoryUsage(true);
    mem += mySharedInfo->myHostname.getMemoryUsage(true);
    return mem;
}

void
GU_PackedSharedData::countMemory(UT_MemoryCounter &counter, bool inclusive) const
{
    if (counter.mustCountUnshared())
    {
        size_t mem = inclusive ? sizeof(*this) : 0;
        UT_MEMORY_DEBUG_LOG("GU_PackedSharedData", int64(mem));
        counter.countUnshared(mem);
    }

    // The UT_MemoryCounter interface needs to be enhanced to efficiently count
    // shared memory for details. Skip this for now.
#if 0
    mem += mySharedInfo->myTimeStamp.getMemoryUsage(true);
    mem += mySharedInfo->myHostname.getMemoryUsage(true);
#endif
}

bool
GU_PackedSharedData::resolveSharedData() const
{
    // The lock is kind of terrible and should likely not be used in production
    // code.  Other options include patterns like a double-lock or task
    // exclusives.
    static UT_Lock	theLock;
    UT_Lock::Scope	lock(theLock);

    if (mySharedHandle)
    {
	const GA_SharedLoadData *item;
	const loadedSharedData	*data;
	item = mySharedHandle->resolveSharedData(nullptr);
	data = dynamic_cast<const loadedSharedData *>(item);
	if (data)
	{
	    // Replace the current creation date with the shared data
	    mySharedInfo = data->myData;
	}
	else
	{
	    // This shouldn't really ever happen
	    return false;	// Unable to resolve shared data
	}

	// When all primitives referencing the shared data are resolved, the
	// shared contents will be deleted.
	mySharedHandle.clear();
    }
    return true;
}

bool
GU_PackedSharedData::saveSharedData(UT_JSONWriter &w,
	GA_SaveMap &map,
	GA_GeometryIndex *geometryIndex) const
{
    bool	ok = true;

    resolveSharedData();	// Ensure shared data is resolved before saving
    if (mySharedInfo)
    {
	// We only want to save the shared data once, so we need to make a key
	// that's unique to my shared data.
	auto	&&key = makeSharedDataKey(mySharedInfo);

	// Check to see whether the shared data has been saved by another
	// primitive
	if (!map.hasSavedSharedData(key))
	{
	    // We're the first primitive to try to save the shared data.
	    map.setSavedSharedData(key);

	    // We need to save out the primitive name
	    ok = ok && w.jsonStringToken(getFactory()->name());

	    // Now, we need to save the location of the shared data in the
	    // index.  This is used to handle seeking for deferred load of the
	    // shared data.
	    geometryIndex->addEntry(key, w.getNumBytesWritten());

	    // Save out the shared data
	    ok = ok && w.jsonBeginArray();
	    ok = ok && w.jsonString(theSharedIdentifier);
	    ok = ok && w.jsonString(key);

	    ok = ok && w.jsonBeginMap();
	    // If you're saving Houdini geometry, you need to wrap the geometry
	    // in: w.startNewFile() and w.endNewFile().  This fixes seek tables
	    // for shared keys, etc.  For example: @code
	    //	w.startNewFile();
	    //    gdp->save(w);
	    //  w.endNewFile();
	    // @endcode
	    // But for our purposes, we just save a string to the JSON stream
	    ok = ok && w.jsonKeyToken(theDateStr);
	    ok = ok && w.jsonString(mySharedInfo->myTimeStamp);
	    ok = ok && w.jsonKeyToken(theHostStr);
	    ok = ok && w.jsonString(mySharedInfo->myHostname);
	    ok = ok && w.jsonEndMap();

	    // Now we need to mark the end of the entry.  This allows the
	    // loader to skip past the shared data (seeking) so the shared data
	    // load can be deferred until it's required.
	    UT_WorkBuffer	endkey(key);
	    endkey.append(GA_SAVEMAP_END_TOKEN);
	    geometryIndex->addEntry(endkey.buffer(), w.getNumBytesWritten());

	    ok = ok && w.jsonEndArray();	// Close off the array
	}
    }
    return ok;
}

UT_StringHolder
GU_PackedSharedData::creationDate() const
{
    // Before returning the creation date, ensure any shared data is loaded.
    resolveSharedData();
    return mySharedInfo->myTimeStamp;
}

UT_StringHolder
GU_PackedSharedData::creationHost() const
{
    // Before returning the hostname, ensure any shared data is loaded.
    resolveSharedData();
    return mySharedInfo->myHostname;
}

/// DSO registration callback
void
newGeometryPrim(GA_PrimitiveFactory *f)
{
    GU_PackedSharedData::install(f);
}
