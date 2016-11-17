/*
 * Copyright (c) 2015
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
 * This is a sample procedural DSO which scatters points inside a box and then
 * generates a box at each scattered point.  It used a second procedural to
 * perform the stamping.
 */

#ifndef __VRAY_DemoStamp__
#define __VRAY_DemoStamp__

#include <UT/UT_BoundingBox.h>
#include <VRAY/VRAY_Procedural.h>

namespace HDK_Sample {

/// A procedural which splits into further procedurals during rendering
class VRAY_DemoStamp : public VRAY_Procedural {
public:
	     VRAY_DemoStamp();
    virtual ~VRAY_DemoStamp();

    virtual const char	*className() const;
    virtual int		 initialize(const UT_BoundingBox *);
    virtual void	 getBoundingBox(UT_BoundingBox &box);
    virtual void	 render();

private:
    UT_BoundingBox	 myBox;		// Bounding box
    fpreal		 mySize;	// Size of each child
    int			 myCount;	// Number of children
    int			 mySeed;	// Random seed
};

}	// End HDK_Sample namespace

#endif
