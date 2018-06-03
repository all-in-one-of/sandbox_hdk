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

#ifndef __GU_PackedSharedData__
#define __GU_PackedSharedData__

#include <GU/GU_PackedImpl.h>
#include <UT/UT_SharedPtr.h>

class GU_PrimPacked;

namespace HDK_Sample
{

/// Example of a simple packed primitive which has shared data
///
/// The primitive created has data which is shared between multiple primitives.
/// The shared data is saved out just one time to the .bgeo file and remains
/// shared after loading.
///
class GU_PackedSharedData : public GU_PackedImpl
{
public:
    GU_PackedSharedData();
    GU_PackedSharedData(const GU_PackedSharedData &src);
    virtual ~GU_PackedSharedData();

    static void install(GA_PrimitiveFactory *factory);

    /// Get the type ID for the GU_PackedSharedData primitive type.
    static GA_PrimitiveTypeId typeId()
    {
        return theTypeId;
    }

    /// @{
    /// Virtual interface from GU_PackedImpl interface
    virtual GU_PackedFactory	*getFactory() const;
    virtual GU_PackedImpl	*copy() const;
    virtual void		 clearData();

    virtual bool	isValid() const;
    virtual bool	load(const UT_Options &options,
				const GA_LoadMap &loadmap)
			{
			    updateFrom(options, &loadmap);
			    return true;
			}
    virtual bool	supportsJSONLoad() const	{ return true; }
    virtual bool	loadFromJSON(const UT_JSONValueMap &options,
				const GA_LoadMap &loadmap)
			{
			    updateFrom(options, &loadmap);
			    return true;
			}
    virtual void	update(const UT_Options &options)
			{
			    updateFrom(options, nullptr);
			}
    virtual bool	save(UT_Options &options, const GA_SaveMap &map) const;
    virtual bool	getBounds(UT_BoundingBox &box) const;
    virtual bool	getRenderingBounds(UT_BoundingBox &box) const;
    virtual void	getVelocityRange(UT_Vector3 &min, UT_Vector3 &max) const;
    virtual void	getWidthRange(fpreal &min, fpreal &max) const;
    virtual bool	unpack(GU_Detail &destgdp) const;

    /// Report memory usage (includes all shared memory)
    virtual int64 getMemoryUsage(bool inclusive) const;

    /// Count memory usage using a UT_MemoryCounter in order to count
    /// shared memory correctly.
    virtual void countMemory(UT_MemoryCounter &counter, bool inclusive) const;
    /// @}

    /// @{
    /// Member data accessors for intrinsics
    UT_StringHolder	creationDate() const;
    UT_StringHolder	creationHost() const;
    /// @}

    /// Save shared primitive data
    virtual bool	saveSharedData(UT_JSONWriter &w,
				GA_SaveMap &map,
				GA_GeometryIndex *geometryIndex) const;

    /// Information which is shared between multiple primitives
    struct SharedInfo
    {
	SharedInfo();
	UT_StringHolder	myTimeStamp;	// Creation time
	UT_StringHolder	myHostname;	// Creation host name
    };

protected:
    /// updateFrom() will update from either UT_Options or UT_JSONValueMap
    template <typename T>
    void	updateFrom(const T &options, const GA_LoadMap *loadmap);

private:
    /// Method to resolve the shared data
    bool			resolveSharedData() const;

    /// The shared handle is used to keep a "handle" to the shared data.  This
    /// handle will be resolved on demand.
    mutable GA_SharedDataHandlePtr	mySharedHandle;
    /// Shared pointer to actual shared data for the primitive
    mutable UT_SharedPtr<SharedInfo>	mySharedInfo;

    static GA_PrimitiveTypeId theTypeId;
};

}	// End namespace

#endif
