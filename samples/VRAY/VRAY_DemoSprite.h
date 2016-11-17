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
 * This is a sample procedural DSO which creates sprites on point geometry.
 */

#ifndef __VRAY_DemoSprite__
#define __VRAY_DemoSprite__

#include <VRAY/VRAY_Procedural.h>
#include <GU/GU_Detail.h>
#include <UT/UT_BoundingBox.h>
#include <UT/UT_IntArray.h>

namespace HDK_Sample {

class vray_SpriteAttribMap;

class VRAY_DemoSpriteParms {
public:
    // Data which is uniform for all splits
    const GU_Detail		*myGdp;
    GA_ROHandleV3		 myVelH;
    GA_ROHandleV2		 mySpriteScaleH;
    GA_ROHandleF		 mySpriteRotH;
    GA_ROHandleS		 mySpriteShopH;
    GA_ROHandleV3		 mySpriteTexH;
    int				 myChunkSize;
    int				 mySpriteLimit;
    float			 myTimeScale;
    int				 myRefCount;
    UT_Matrix3			 myViewRotation;
    vray_SpriteAttribMap	*myAttribMap;
};

class VRAY_DemoSprite : public VRAY_Procedural {
public:
	     VRAY_DemoSprite();
    virtual ~VRAY_DemoSprite();

    virtual const char	*className() const;
    virtual int		 initialize(const UT_BoundingBox *box);
    int			 initChild(VRAY_DemoSprite *sprite,
				    const UT_BoundingBox &box);
    virtual void	 getBoundingBox(UT_BoundingBox &box);
    virtual void	 render();

private:
    const GU_Detail	*getPointGdp() const	{ return myParms->myGdp; }
    fpreal		 getTime() const	{ return myParms->myTimeScale; }
    int			 getChunk() const	{ return myParms->myChunkSize; }

    VRAY_DemoSpriteParms	*myParms;

    // Data which is unique per procedural
    UT_BoundingBox	 myBox;
    UT_BoundingBox	 myVelBox;
    UT_IntArray		 myPointList;
};

}	// End HDK_Sample namespace

#endif
