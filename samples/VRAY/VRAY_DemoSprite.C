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

#include "VRAY_DemoSprite.h"

#include <VRAY/VRAY_IO.h>

#include <GU/GU_Detail.h>
#include <GU/GU_PrimPoly.h>
#include <GA/GA_AIFCopyData.h>
#include <GA/GA_Handle.h>
#include <GA/GA_Types.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Defines.h>
#include <SYS/SYS_Floor.h>
#include <SYS/SYS_Math.h>


#define MIN_CHUNK		8
#define SPRITE_LIMIT		1000
#define META_CORRECT		0.5
#define DEFAULT_ATTRIB_PATTERN	""
#define DEFAULT_SIZE		0.05F


namespace HDK_Sample {

class vray_SpriteAttribMap
{
public:
    int myDIndex;                       // Index of destination attribute
    const GA_Attribute *mySourceAttrib; // Source attribute

    vray_SpriteAttribMap *myNext;
};

}	// End HDK_Sample namespace

using namespace HDK_Sample;


// Arguments for this procedural:
//
//	-v velocity attribute name
//	-o object name
//	-t time step for motion blur
//	-A attribute pattern
//	-C chunk size
//	-L LOD
//	-M maximum number of sprites to generate from one procedural
static VRAY_ProceduralArg theArgs[] = {
    VRAY_ProceduralArg("velocity",	"string",	"v"),
    VRAY_ProceduralArg("object",	"string",	""),
    VRAY_ProceduralArg("attribute",	"string",	""),
    VRAY_ProceduralArg("chunksize",	"int",		"16"),
    VRAY_ProceduralArg("maxsprites",	"int",		"1000"),
    VRAY_ProceduralArg(),
};

// External entry point used to create this procedural
VRAY_Procedural *
allocProcedural(const char *)
{
    return new VRAY_DemoSprite();
}

// External entry point used to create this procedural's arguments
const VRAY_ProceduralArg *
getProceduralArgs(const char *)
{
    return theArgs;
}

static void
destroyMap(vray_SpriteAttribMap *map)
{
    while (map)
    {
	vray_SpriteAttribMap *next = map->myNext;
	delete map;
	map = next;
    }
}

static void
setAttribMap(vray_SpriteAttribMap *&maphead, const GU_Detail *gdp,
             const char *pattern)
{
    if (!gdp)
        return;

    int index = 0;
    const GA_AttributeDict &dict = gdp->pointAttribs();
    for (GA_AttributeDict::iterator it = dict.begin(GA_SCOPE_PUBLIC);
            !it.atEnd(); ++it)
    {
        const GA_Attribute *atr = it.attrib();
        UT_String name(atr->getName());
        if (name.multiMatch(pattern))
	{
	    if (atr->getAIFCopyData())
	    {
		vray_SpriteAttribMap *map = new vray_SpriteAttribMap();
		map->mySourceAttrib = atr;
		map->myDIndex = index++;
		map->myNext = maphead;
		maphead = map;
	    }
	}
    }
}

VRAY_DemoSprite::VRAY_DemoSprite()
{
    myBox.initBounds(0, 0, 0);
    myVelBox = myBox;
    myParms = 0;
}

// We get a rough bounding box for the sprite by expanding the box around
// the position by the largest component of the scale (assuming it were
// rotated 45 degrees).
static void
getRoughSpriteBox(UT_BoundingBox &box, UT_BoundingBox &vbox, const GA_Detail &gdp,
		  GA_Offset ptoff, const UT_Vector2 &sprite_scale,
		  const GA_ROHandleV3 &velh,
		  fpreal tscale)
{
    fpreal maxradius = SYSmax(sprite_scale.x(), sprite_scale.y()) * M_SQRT1_2;

    box.initBounds(gdp.getPos3(ptoff));
    box.expandBounds(0, maxradius);

    vbox = box;

    if (velh.isValid())
    {
	UT_Vector3 vel = velh.get(ptoff);
	vel *= tscale;
	vbox.translate(vel);
    }
}

int
VRAY_DemoSprite::initChild(VRAY_DemoSprite *sprite, const UT_BoundingBox &box)
{
    myParms = sprite->myParms;
    myParms->myRefCount++;

    const GU_Detail *gdp = getPointGdp();

    UT_Vector2 sprite_scale(0.1, 0.1);

    // Initially, we divide the points based solely on the actual
    // point positions.  We then compute the actual bounding box of
    // this division.  This ensures that each point is only in
    // one procedural.
    myPointList.setCapacity(sprite->myPointList.entries());
    for (exint i = 0; i < sprite->myPointList.entries(); ++i)
    {
	GA_Index idx = sprite->myPointList(i);
	GA_Offset ptoff = gdp->pointOffset(idx);
	if (box.isInside(gdp->getPos3(ptoff)))
	{
	    myPointList.append(idx);
	}
    }
    myPointList.setCapacity(myPointList.entries());

    UT_BoundingBox tbox;
    UT_BoundingBox tvbox;
    for (exint i = 0; i < myPointList.entries(); ++i)
    {
	GA_Index idx = myPointList(i);
	GA_Offset ptoff = gdp->pointOffset(idx);

	if (myParms->mySpriteScaleH.isValid())
	    sprite_scale = myParms->mySpriteScaleH.get(ptoff);

	getRoughSpriteBox(tbox, tvbox, *gdp, ptoff, sprite_scale,
		          myParms->myVelH, myParms->myTimeScale);

	if (i == 0)
	{
	    myBox = tbox;
	    myVelBox = tvbox;
	}
	else
	{
	    myBox.enlargeBounds(tbox);
	    myVelBox.enlargeBounds(tvbox);
	}
    }

//    printf("init child with %d entries\n", myPointList.entries());
    if (!myPointList.entries())
	return 0;
    myBox.clipBounds(box);
    return 1;
}

VRAY_DemoSprite::~VRAY_DemoSprite()
{
    myParms->myRefCount--;
    if (!myParms->myRefCount)
    {
	destroyMap(myParms->myAttribMap);
	delete myParms;
    }
}

const char *
VRAY_DemoSprite::className() const
{
    return "sprite";
}

int
VRAY_DemoSprite::initialize(const UT_BoundingBox *box)
{
    void		*handle;
    const char		*name;
    UT_BoundingBox	 tbox, tvbox;
    const GU_Detail	*gdp;
    UT_Matrix4		 xform;
    UT_String		 str;
    int			 vblur;

    myParms = new VRAY_DemoSpriteParms;
    myParms->myGdp = 0;
    myParms->myVelH.clear();
    myParms->mySpriteScaleH.clear();
    myParms->mySpriteRotH.clear();
    myParms->mySpriteShopH.clear();
    myParms->mySpriteTexH.clear();
    myParms->myChunkSize = MIN_CHUNK * 2;
    myParms->mySpriteLimit = SPRITE_LIMIT;
    myParms->myRefCount = 1;
    myParms->myAttribMap = 0;

    //
    // First, find the geometry object we're supposed to render
    name = 0;
    if (import("object", str))
	name = str.isstring() ? (const char *)str : 0;
    handle = queryObject(name);
    if (!handle)
    {
	VRAYerror("%s couldn't find object '%s'", className(), name);
	return 0;
    }
    name = queryObjectName(handle);
    VRAY_ROProceduralGeo	parent_geo = queryGeometry(handle);
    gdp = myParms->myGdp = parent_geo.get();
    if (!gdp)
    {
	VRAYerror("%s object '%s' has no geometry", className(), name);
	return 0;
    }

    // Retrieve the velocity scale for use in velocity motion blur
    // calculations
    if (!import("object:velocityscale", &myParms->myTimeScale, 1))
	myParms->myTimeScale = 0.0f;

    vblur = 0;
    import("object:velocityblur", &vblur, 0);

    // Form the rotation matrix to orient the sprites toward the viewer
    // without scaling
    UT_Vector3	scale;
    myParms->myViewRotation = queryTransform(handle, 0);
    myParms->myViewRotation.invert();
    myParms->myViewRotation.extractScales(scale);

    // Since extractScales may return negative scales (for negative
    // determinant matrices), these then need to be factored back into the
    // rotation matrix.  Only the positive scale factors should be
    // extracted here.
    if (scale[0] < 0)
	myParms->myViewRotation.scale(-1, -1, -1);

    myParms->mySpriteScaleH = GA_ROHandleV2(gdp->findFloatTuple(GA_ATTRIB_POINT,
					"spritescale", 2));
    myParms->mySpriteRotH = GA_ROHandleF(gdp->findFloatTuple(GA_ATTRIB_POINT,
					"spriterot", 1));
    myParms->mySpriteShopH = GA_ROHandleS(gdp->findStringTuple(GA_ATTRIB_POINT,
					"spriteshop", 1));
    myParms->mySpriteTexH = GA_ROHandleV3(gdp->findFloatTuple(GA_ATTRIB_POINT,
					"spriteuv", 3));

    //
    // Now, find the velocity attribute (if it's there)
    if (vblur)
    {
	str = 0;
	import("velocity", str);
	if (str.isstring())
	{
	    myParms->myVelH = GA_ROHandleV3(gdp->findFloatTuple(GA_ATTRIB_POINT, str, 3));
	    if (myParms->myVelH.isInvalid())
		VRAYwarning("%s object (%s) couldn't find the '%s' attribute",
			className(), name, (const char *)str);
	}
    }

    UT_Vector2 sprite_scale(0.1, 0.1);

    GA_Index i = 0;
    GA_Offset ptoff;
    GA_FOR_ALL_PTOFF(gdp, ptoff)
    {
	if (myParms->mySpriteScaleH.isValid())
	    sprite_scale = myParms->mySpriteScaleH.get(ptoff);

	getRoughSpriteBox(tbox, tvbox, *gdp, ptoff, sprite_scale,
		          myParms->myVelH, myParms->myTimeScale);
	myPointList.append(i);
	if (i == 0)
	{
	    myBox = tbox;
	    myVelBox = tvbox;
	}
	else
	{
	    myBox.enlargeBounds(tbox);
	    myVelBox.enlargeBounds(tvbox);
	}
        ++i;
    }

    if (!myPointList.entries())
    {
	VRAYwarning("%s found no points in %s", className(), name);
	return 1;
    }

    str = 0;
    import("attribute", str);
    if (str.isstring())
	setAttribMap(myParms->myAttribMap, gdp, str);
    import("chunksize", &myParms->myChunkSize, 1);
    if (myParms->myChunkSize < MIN_CHUNK)
	myParms->myChunkSize = MIN_CHUNK;
    import("maxsprites", &myParms->mySpriteLimit, 1);
    if (myParms->mySpriteLimit < SPRITE_LIMIT)
	myParms->mySpriteLimit = SPRITE_LIMIT;

    if (box)
    {
	myBox.clipBounds(*box);
	myVelBox.clipBounds(*box);
	if (!myVelBox.isValid())
	{
	    VRAYwarning("%s empty box for %s", className(), name);
	    return 0;
	}
    }

    return 1;
}

void
VRAY_DemoSprite::getBoundingBox(UT_BoundingBox &box)
{
    // We need to return the maximum area that the primitive is defined over.
    box = myBox;
    box.enlargeBounds(myVelBox);
}

static void
applyMapToPrimitive(vray_SpriteAttribMap *map,
	const UT_Array<GA_Attribute*> &dest_attribs,
	GEO_Primitive *dest, const GA_Detail &srcgdp, GA_Offset srcptoff)
{
    GA_Offset destoff = dest->getMapOffset();

    while (map)
    {
        UT_ASSERT(map->mySourceAttrib);

        GA_Attribute *dest_attrib = dest_attribs(map->myDIndex);
        const GA_AIFCopyData *copy = dest_attrib->getAIFCopyData();
        copy->copy(*dest_attrib, destoff, *map->mySourceAttrib, srcptoff);

        map = map->myNext;
    }
}

static void
transformPoint(GA_Detail &gdp, GA_Offset ptoff, fpreal dx, fpreal dy, const UT_Matrix4 &xform)
{
    UT_Vector3 P = gdp.getPos3(ptoff);
    P.x() += dx;
    P.y() += dy;
    P *= xform;
    gdp.setPos3(ptoff, P);
}

static void
convertPath(const char *src_path, const char *path, UT_String &full_path)
{
    if (path && strlen(path) > 0 && path[0] != '/')
    {
	full_path = src_path;
	full_path += "/";
	full_path += path;
	full_path.collapseAbsolutePath();
    }
    else
	full_path = path;
}

static int
makeSpritePoly(GU_Detail *gdp, const GU_Detail *src, UT_IntArray &points,
	       const VRAY_DemoSpriteParms &parms, const char *srcpath)
{
    UT_Array<GA_Attribute*> dest_attribs;
    GA_RWHandleV3 txth;
    GA_RWHandleS shoph;
    if (points.entries())
    {
	if (parms.mySpriteShopH.isValid())
	{
	    shoph = GA_RWHandleS(gdp->addStringTuple(
		GA_ATTRIB_PRIMITIVE, "shop_materialpath", 1));

	    if (parms.mySpriteTexH.isValid())
	    {
		txth = GA_RWHandleV3(gdp->addTextureAttribute(GA_ATTRIB_POINT));
	    }
	}

	// Create the attributes that we wish to copy from the points.
	if (parms.myAttribMap)
	{
            // NOTE: This only works because the first myDIndex is the
            //       largest.
	    dest_attribs.entries(parms.myAttribMap->myDIndex+1);

	    for (vray_SpriteAttribMap *map = parms.myAttribMap; map; map = map->myNext)
	    {
		const GA_Attribute *atr = map->mySourceAttrib;
                UT_ASSERT(map->myDIndex < dest_attribs.entries());
		dest_attribs(map->myDIndex) = gdp->addPrimAttrib(atr);
		UT_ASSERT(dest_attribs(map->myDIndex) != NULL);
	    }
	}
    }

    fpreal uMin = 0.0F;
    fpreal vMin = 0.0F;
    fpreal uMax = 1.0F;
    fpreal vMax = 1.0F;
    UT_Vector2R size(DEFAULT_SIZE, DEFAULT_SIZE);
    UT_Matrix4 view_inverse;
    view_inverse = parms.myViewRotation;

    GA_Offset ptoff = gdp->appendPointBlock(4 * points.entries());
    GA_Offset primoff = gdp->appendPrimitiveBlock(GA_PRIMPOLY, points.entries());

    // It is important to note that the order of the vertices is reversed,
    // making this a backfacing polygon.  We do this to make sure that the
    // s, t coordinates run correctly for when we're not bothering with a
    // texture attribute.
    for (exint i = 0; i < points.entries(); i++)
    {
	GA_Offset srcptoff = src->pointOffset(points(i));
	if (parms.mySpriteScaleH.isValid())
	{
            size = parms.mySpriteScaleH.get(srcptoff) * 0.5F;
	}
	if (parms.mySpriteTexH.isValid())
	{
            UT_Vector3 txt = parms.mySpriteTexH.get(srcptoff);
            uMin = txt.x();
            uMin = txt.y();
            uMax = txt.x()+txt.z();
            vMax = txt.y()+txt.z();
        }

        UT_Vector3 srcpos = src->getPos3(srcptoff);
        UT_Matrix4 xform;
	xform.identity();
	xform.translate(srcpos.x(), srcpos.y(), srcpos.z());
	xform.leftMult(view_inverse);
	if (parms.mySpriteRotH.isValid())
            xform.prerotate(UT_Axis3::ZAXIS, SYSdegToRad(
                            parms.mySpriteRotH.get(srcptoff)));

        GU_PrimPoly *poly = (GU_PrimPoly *)gdp->getPrimitiveList().get(primoff);
        poly->close();
        poly->appendVertex(ptoff);
        poly->appendVertex(ptoff+1);
        poly->appendVertex(ptoff+2);
        poly->appendVertex(ptoff+3);

	::transformPoint(*gdp, ptoff, -size.x(), -size.y(), xform);
	if (txth.isValid())
	{
            txth.set(ptoff, UT_Vector3(uMin, vMin, 0));
	}
        ++ptoff;

	::transformPoint(*gdp, ptoff, size.x(), -size.y(), xform);
	if (txth.isValid())
	{
            txth.set(ptoff, UT_Vector3(uMax, vMin, 0));
	}
        ++ptoff;

	::transformPoint(*gdp, ptoff, size.x(), size.y(), xform);
	if (txth.isValid())
	{
	    txth.set(ptoff, UT_Vector3(uMax, vMax, 0));
	}
        ++ptoff;

	::transformPoint(*gdp, ptoff, -size.x(), size.y(), xform);
	if (txth.isValid())
	{
            txth.set(ptoff, UT_Vector3(uMin, vMax, 0));
	}
        ++ptoff;

	if (shoph.isValid())
	{
	    const char *path = parms.mySpriteShopH.get(srcptoff);
	    UT_String full_path;
	    convertPath(srcpath, path, full_path);
	    shoph.set(poly->getMapOffset(), full_path.buffer());
	}

	// Apply the attribute map if we actually have one.
	if (parms.myAttribMap)
	    applyMapToPrimitive(parms.myAttribMap, dest_attribs,
				poly,
                                *src, srcptoff);

        ++primoff;
    }

    return gdp->getNumPrimitives();
}

static void
velocityMove(GU_Detail *gdp, const GA_RWHandleV3 &mpos, const GU_Detail *src,
	     const UT_IntArray &points, const GA_ROHandleV3 &velh,
	     fpreal scale)
{
    for (exint i = 0; i < points.entries(); ++i)
    {
	GA_Offset ptoff = src->pointOffset(points(i));
	UT_Vector3 vel = velh.get(ptoff);
	vel *= scale;

	for (int j = 0; j < 4; j++)
	{
	    GA_Offset off = gdp->getPointMap().offsetFromIndex(
		    GA_Index(i*4 + j));
	    mpos.add(off, vel);
	}
    }
}

static inline int
computeDivs(fpreal inc, fpreal min)
{
    int	divs = (int)SYSceil(inc / min);
	 if (divs < 1) divs = 1;
    else if (divs > 4) divs = 4;
    return divs;
}

void
VRAY_DemoSprite::render()
{
    fpreal		 max;
    UT_BoundingBox	 kidbox;
    VRAY_DemoSprite	*kid;
    int			 dogeo;
    int			 nx, ny, nz;
    int			 ix, iy, iz;
    fpreal		 xinc, yinc, zinc, factor;
    fpreal		 xv, yv, zv;
    fpreal		 dfactor;
    int			 sprite_limit = myParms->mySpriteLimit;
    fpreal		 lod;

    if (!myPointList.entries())
	return;

    dogeo = 1;
    // Compute LOD without regards to motion blur
    lod = getLevelOfDetail(myBox);
    if (lod > myParms->myChunkSize && myPointList.entries() > sprite_limit)
    {
	// Split into further procedurals
	dogeo = 0;
	xinc = myBox.sizeX();
	yinc = myBox.sizeY();
	zinc = myBox.sizeZ();

	max = myBox.sizeMax();
	dfactor = (xinc+yinc+zinc)/max;
	factor = SYSpow((fpreal)myPointList.entries() / sprite_limit,
		      1.0F/dfactor);
	if (factor > 4)
	    factor = 4;
	max /= factor;
	//printf("Preparing to split %d points with %g lod [%g %g %g]\n",
	//	myPointList.entries(), lod,
	//	myBox.sizeX(), myBox.sizeY(), myBox.sizeZ());

	nx = ::computeDivs(xinc, max);
	ny = ::computeDivs(yinc, max);
	nz = ::computeDivs(zinc, max);

	if (nx == 1 && ny == 1 && nz == 1)
	{
	    if (xinc > yinc)
	    {
		if (xinc > zinc) nx = 2;
		else		 nz = 2;
	    }
	    else
	    {
		if (yinc > zinc) ny = 2;
		else		 nz = 2;
	    }
	}
	xinc /= (fpreal)nx;
	yinc /= (fpreal)ny;
	zinc /= (fpreal)nz;
	//printf("breaking up into: %dx%dx%d\n", nx, ny, nz);

	for (iz = 0, zv = myBox.vals[2][0]; iz < nz; iz++, zv += zinc)
	{
	    for (iy = 0, yv = myBox.vals[1][0]; iy < ny; iy++, yv += yinc)
	    {
		for (ix = 0, xv = myBox.vals[0][0]; ix < nx; ix++, xv += xinc)
		{
		    kidbox.initBounds(xv, yv, zv);
		    kidbox.enlargeBounds(xv+xinc, yv+yinc, zv+zinc);
		    kid = new VRAY_DemoSprite();
		    if (!kid->initChild(this, kidbox))
			delete kid;
		    else
		    {
			VRAY_ProceduralChildPtr	child = createChild();
			child->addProcedural(kid);
		    }
		}
	    }
	}
    }

    if (dogeo)
    {
	lod = SYSmax(lod, (fpreal)3);
	VRAY_ProceduralGeo	geo = createGeometry();

	// Don't turn backface culling on.  We attempt to reduce data
	// size by using the st parameterization of the quad instead
	// of texture coordinates when we can, and when doing this, the
	// quad actually turns out to be backfacing.

	if (makeSpritePoly(geo.get(), getPointGdp(), myPointList, *myParms,
			    queryRootName()))
	{
	    if (myParms->myVelH.isValid())
	    {
		GA_RWHandleV3 vpos(geo.appendSegmentAttribute(1.0, "P"));
		velocityMove(geo.get(), vpos, getPointGdp(), myPointList,
			     myParms->myVelH, getTime());
	    }
	    VRAY_ProceduralChildPtr	obj = createChild();
	    obj->addGeometry(geo);
	}
    }
}
