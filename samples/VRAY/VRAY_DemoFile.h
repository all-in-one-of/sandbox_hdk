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
 * This is a sample procedural DSO to generate a box
 */

#ifndef __VRAY_DemoFile__
#define __VRAY_DemoFile__

#include <UT/UT_BoundingBox.h>
#include <UT/UT_String.h>
#include <VRAY/VRAY_Procedural.h>

namespace HDK_Sample {

/// @brief A procedural which does a deferred load of geometry from disk
class VRAY_DemoFile : public VRAY_Procedural {
public:
	     VRAY_DemoFile();
    virtual ~VRAY_DemoFile();

    virtual const char	*className() const;
    virtual int		 initialize(const UT_BoundingBox *);
    virtual void	 getBoundingBox(UT_BoundingBox &box);
    virtual void	 render();

private:
    UT_BoundingBox	 myBox;
    UT_StringHolder	 myFile, myBlurFile;
    fpreal		 myShutter;
    bool		 myVelocityBlur;
    fpreal		 myPreBlur, myPostBlur;
};

}	// End HDK_Sample namespace
#endif
