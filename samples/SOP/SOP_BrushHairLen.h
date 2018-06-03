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

#ifndef __SOP_BrushHairLen__
#define __SOP_BrushHairLen__

#include <SOP/SOP_Node.h>
#include <SOP/SOP_BrushBase.h>

namespace HDK_Sample {
class SOP_BrushHairLen : public SOP_BrushBase
{
public:
	     SOP_BrushHairLen(OP_Network *net, const char *, OP_Operator *entry);
    virtual ~SOP_BrushHairLen();

    static OP_Node	*myConstructor(OP_Network  *net, const char *name,
				       OP_Operator *entry);
    static PRM_Template	 myTemplateList[];

    /// This is the callback triggered when a BRUSHOP_CALLBACK is used:
    void		 brushOpCallback(
			    GA_Offset pt,
			    const UT_Array<GA_Offset> *ptneighbour,
			    GA_Offset vtx,
			    const UT_Array<GA_Offset> *vtxneighbour,
			    float alpha,
			    GEO_Delta *delta);

protected:
    virtual OP_ERROR	 cookMySop   (OP_Context &context);

    /// These are the many & various methods used by BrushBase to
    /// determine the current brush status.  We can either hook them
    /// up to parameters (via evalFloat) or hard code them.  In this
    /// case we have hard coded most.

    /// Public methods needed by MSS:
public:
    /// Finds the geometry to do the intersection with.
    const GU_Detail		*getIsectGdp(fpreal t);
    
    /// If the action of the brush will change point positions,
    /// we should set this to 1.  We create geometry, but leave point
    /// positions untouched.
    virtual int		altersGeometry() const
	{ return 0; }
    /// We do attribute changes.
    virtual int		altersColor() const
	{ return 1; }
    /// This gets the current radius of the brush, unmodulated by the
    /// current pressure amount.
    virtual fpreal	RAWRADIUS(fpreal t)
	{ return evalFloat("radius", 0, t); }
    /// This gets the raw radius for the UV space.
    virtual fpreal	RAWUVRADIUS(fpreal t)
	{ return evalFloat("uvradius", 0, t); }
    /// This is how much the HITPRESSURE should affect the RADIUS.
    virtual fpreal 	RADIUSPRESSURE(fpreal /*t*/)
	{ return 1.0f; }
    /// The opacity, hardcoded to 1.0f, for a full replace
    virtual fpreal	RAWOPACITY(fpreal /*t*/)
	{ return 1.0f; }
    virtual fpreal	OPACITYPRESSURE(fpreal /*t*/)
	{ return 1.0f; }

    /// This is how far along the brush axis it will be able to paint.
    /// We tie it directly to the raidus here.
    virtual fpreal	DEPTH(fpreal t)
	{ return evalFloat("radius", 0, t); }
    virtual fpreal	HEIGHT(fpreal t)
	{ return evalFloat("radius", 0, t); }
    /// Whether to use depth clipping at all.
    virtual int		USEDEPTH()
	{ return 1; }
    /// Whether to not paint across unconnected seams.  We are using depth
    /// clipping in this tool, so have it off.
    virtual int		USECONNECTIVITY()
	{ return 0; }

    /// This needs to map our OP_Menu into SOP_BrushOp.  We leave this
    /// to the C code.
    virtual SOP_BrushOp	OP();

    /// Whether accumulate stencil is currently on.  We keep it off.
    virtual int		ACCUMSTENCIL()
	{ return 0; }
    /// Projection Type 1 is orient to surface, 0 is to use it flat on.
    /// We hard code it to flat on.
    virtual int		PROJECTIONTYPE()
	{ return 0; }
    /// In realtime mode, all changes are applied immediately to the
    /// gdp, rather than being stored in the CurrentDelta during a brush
    /// stroke.  Some types of brushing require this behaviour implicitly.
    virtual int		REALTIME()
	{ return 1; }
    /// This returns a SOP_BrushShape.  We hardcode to circle.
    virtual int		SHAPE(fpreal /*t*/)
	{ return SOP_BRUSHSHAPE_CIRCLE; }

    /// Protected methods needed by SOP_BrushBase:
protected:
    /// This returns a GU_BrushMergeMode, defined in GU_Brush.h
    /// We just use the standard replace.
    virtual int		MERGEMODE()
	{ return GU_BRUSHMERGEMODE_REPLACE; }

    /// Not used:
    virtual void	SCRIPT(UT_String & /*s*/, fpreal /*t*/)
	{ }
    
    /// These are used by deformation brush ops:
    virtual int		AXIS()
	{ return 0; }
    virtual fpreal	USERX(fpreal /*t*/)
	{ return 0.0f; }
    virtual fpreal	USERY(fpreal /*t*/)
	{ return 0.0f; }
    virtual fpreal	USERZ(fpreal /*t*/)
	{ return 0.0f; }

    /// These query the current raystate.  We read off the cached values...
    virtual fpreal	RAYORIENTX(fpreal /*t*/)
	{ return myRayOrient.x(); }
    virtual fpreal	RAYORIENTY(fpreal /*t*/)
	{ return myRayOrient.y(); }
    virtual fpreal	RAYORIENTZ(fpreal /*t*/)
	{ return myRayOrient.z(); }
    virtual fpreal	RAYHITX(fpreal /*t*/)
	{ return myRayHit.x(); }
    virtual fpreal	RAYHITY(fpreal /*t*/)
	{ return myRayHit.y(); }
    virtual fpreal	RAYHITZ(fpreal /*t*/)
	{ return myRayHit.z(); }
    virtual fpreal	RAYHITU(fpreal /*t*/)
	{ return myRayHitU; }
    virtual fpreal	RAYHITV(fpreal /*t*/)
	{ return myRayHitV; }
    virtual fpreal	RAYHITW(fpreal /*t*/)
	{ return myRayHitW; }
    virtual fpreal	RAYHITPRESSURE(fpreal /*t*/)
	{ return myRayHitPressure; }
    virtual int		PRIMHIT(fpreal /*t*/)
	{ return myPrimHit; }
    virtual int		PTHIT(fpreal /*t*/)
	{ return myPtHit; }
    virtual int		EVENT()
	{ return myEvent; }

    /// These query the current "colours" of the brush...
    /// F is for foreground, B, for background.  Up to a 3 vector is
    /// possible in the order R, G, B.
    /// If USE_FOREGROUND is true, the F will be used, otherwise B.
    virtual bool	USE_FOREGROUND()
	{ return myUseFore; }
    virtual fpreal	FGR(fpreal t)
	{ return evalFloat("flen", 0, t); }
    virtual fpreal	FGG(fpreal /*t*/)
	{ return 0.0f; }
    virtual fpreal	FGB(fpreal /*t*/)
	{ return 0.0f; }
    virtual fpreal	BGR(fpreal t)
	{ return evalFloat("blen", 0, t); }
    virtual fpreal	BGG(fpreal /*t*/)
	{ return 0.0f; }
    virtual fpreal	BGB(fpreal /*t*/)
	{ return 0.0f; }

    /// These control the shape of the nib.  We have hard coded
    /// most of them...
    /// What percentage of the radius to devote to the fall off curve.
    virtual fpreal 	SOFTEDGE(fpreal /*t*/)
	{ return 1.0f; }
    /// Which metaball kernel to use as the fall off curve.
    virtual void	KERNEL(UT_String &str, fpreal /*t*/)
	{ str = "Elendt"; }
    /// How to determine the upvector, unnecessary with circular brushes.
    virtual int		UPTYPE(fpreal /*t*/)
	{ return 0; }
    virtual fpreal	UPX(fpreal /*t*/)
	{ return 0.0f; }
    virtual fpreal	UPY(fpreal /*t*/)
	{ return 0.0f; }
    virtual fpreal	UPZ(fpreal /*t*/)
	{ return 0.0f; }

    /// Alpha noise in the paper space
    virtual fpreal	PAPERNOISE(fpreal /*t*/)
	{ return 0.0f; }
    /// Alpha noise in the brush space
    virtual fpreal	SPLATTER(fpreal /*t*/)
	{ return 0.0f; }
    /// Name of the bitmap to use if bitmap brush is on, which it isn't.
    virtual void	BITMAP(UT_String & /*str*/, fpreal /*t*/)
	{ }
    /// Which channel of the bitmap brush should be used.
    virtual int		BITMAPCHAN(fpreal /*t*/)
	{ return 0; }
    /// More hard coded stuff directly from the Nib tab...
    virtual fpreal	ANGLE(fpreal /*t*/)
	{ return 0.0f; }
    virtual fpreal	SQUASH(fpreal /*t*/)
	{ return 1.0f; }
    virtual int		DOSTAMPING()
	{ return 0; }
    virtual int		WRITEALPHA()
	{ return 0; }
    /// We explicitly override these parameters as we want the brush
    /// to automatically edit our "hairlen" point attribute.
    virtual int		OVERRIDECD()
	{ return 1; }
    virtual void	CDNAME(UT_String &str, fpreal /*t*/)
	{ str = "hairlen"; }
    /// As WRITEALPHA is off, this is irrelevant:
    virtual int		OVERRIDEALPHA()
	{ return 0; }
    virtual void	ALPHANAME(UT_String & /*str*/, fpreal /*t*/)
	{ }
    /// For Comb style brushes, this preserves the normal length.
    virtual int		PRESERVENML()
	{ return 0; }
    /// For Comb style brushes this allows overriding the normal attribute.
    virtual int		OVERRIDENML()
	{ return 0; }
    virtual void	NMLNAME(UT_String & /*str*/, fpreal /*t*/)
	{ }

    /// These methods are used to get the current symmetry operations
    /// that are enabled with the brush.  We don't use any of them here.
    virtual int		DOREFLECTION()
	{ return 0; }
    virtual int		DOROTATION()
	{ return 0; }
    virtual fpreal	SYMMETRYDIRX(fpreal /*t*/)
	{ return 0.0f; }
    virtual fpreal	SYMMETRYDIRY(fpreal /*t*/)
	{ return 0.0f; }
    virtual fpreal	SYMMETRYDIRZ(fpreal /*t*/)
	{ return 0.0f; }
    virtual fpreal	SYMMETRYORIGX(fpreal /*t*/)
	{ return 0.0f; }
    virtual fpreal	SYMMETRYORIGY(fpreal /*t*/)
	{ return 0.0f; }
    virtual fpreal	SYMMETRYORIGZ(fpreal /*t*/)
	{ return 0.0f; }
    virtual int		SYMMETRYROT(fpreal /*t*/)
	{ return 0; }
    virtual fpreal	SYMMETRYDIST(fpreal /*t*/)
	{ return 0.0f; }

    /// This determines if the Cd or Normal should be auto-added.
    /// We have it off as we add it by hand in our cook.
    virtual int		ADDATTRIB()
	{ return 0; }
    /// This determines if visualization will occur.
    virtual int		VISUALIZE()
	{ return 0; }
    virtual int		VISTYPE()
	{ return 0; }
    virtual fpreal	VISLOW(fpreal /*t*/)
	{ return 0.0f; }
    virtual fpreal	VISHIGH(fpreal /*t*/)
	{ return 1.0f; }
    virtual int		VISMODE()
	{ return 0; }
    virtual fpreal	ZEROWEIGHTCOLOR_R() 
	{ return 1.0f; }
    virtual fpreal	ZEROWEIGHTCOLOR_G() 
	{ return 1.0f; }
    virtual fpreal	ZEROWEIGHTCOLOR_B() 
	{ return 1.0f; }

    /// This is used by capture brushes to determine if they should
    /// normalize the weights.
    virtual int		NORMALIZEWEIGHT()
	{ return 0; }

    /// Should return true if this brush will affect capture regions.
    virtual int		USECAPTURE()
	{ return 0; }
    /// Returns the capture region to brush for such brushes.
    virtual int		CAPTUREIDX(fpreal /*t*/)
	{ return 0; }

    /// These determine if the relevant parameters have changed.  They are
    /// often used to determine when to invalidate internal caches.
    
    /// hasStrokeChanged should return true if any of the setHit* were called,
    /// as it will cause a new dab to be applied.
    virtual bool	hasStrokeChanged(fpreal /*t*/)
	{ return myStrokeChanged; }
    /// This should return true if the style of the brush changed, specifically
    /// the foreground or background colours.  We test this trhough isParmDirty
    /// in the C code.
    virtual bool	hasStyleChanged(fpreal t);
    /// This determines if the nib file changed.  We don't have a nib file.
    virtual bool	hasNibFileChanged(fpreal /*t*/)
	{ return false; }
    /// This returns true if any nib parameters (such as shape, etc) have
    /// changed since the last cook.
    virtual bool	hasNibLookChanged(fpreal /*t*/)
	{ return false; }
    /// This returns true if the type of accumulation mode changed.
    virtual bool	hasAccumStencilChanged(fpreal /*t*/)
	{ return false; }
    /// This returns true if the capture index has changed.
    virtual bool	hasCaptureIdxChanged(fpreal /*t*/)
	{ return false; }
    /// This return strue if the visualization range has changed.
    virtual bool	hasVisrangeChanged(fpreal /*t*/)
	{ return false; }

    /// If this returns true, the selection will be cooked as the
    /// group.  We don't want that, so return false.
    virtual bool	wantsCookSelection() const
	{ return false; }

    /// public methods used by MSS level to write to this level:
    /// The usual brush ops (such as Paint, etc) will set parameters
    /// with these values.  We just cache them locally here.
public:
    /// We are locking off accumulate stencil (ACCUMSTENCIL) so can
    /// ignore attempts to turn it on.
    virtual void setAccumulateStencil(bool /*yesno*/) {}
    virtual void setRayOrigin(const UT_Vector3 &orig, fpreal /*t*/)
	{ myRayHit = orig; myStrokeChanged = true; forceRecook(); }
    virtual void setRayOrientation(const UT_Vector3 &orient, fpreal /*t*/)
	{ myRayOrient = orient; myStrokeChanged = true; forceRecook(); }
    virtual void setHitPrimitive(int primidx, fpreal /*t*/)
	{ myPrimHit = primidx; myStrokeChanged = true; forceRecook(); }
    virtual void setHitPoint(int ptidx, fpreal /*t*/)
	{ myPtHit = ptidx; myStrokeChanged = true; forceRecook(); }
    virtual void setHitUVW(fpreal u, fpreal v, fpreal w, fpreal /*t*/)
    {
        myRayHitU = u;
        myRayHitV = v;
        myRayHitW = w;
        myStrokeChanged = true;
        forceRecook();
    }
    virtual void setHitPressure(fpreal pressure, fpreal /*t*/)
	{ myRayHitPressure = pressure; myStrokeChanged = true; forceRecook(); }
    virtual void setBrushEvent(SOP_BrushEvent event)
	{ myEvent = event; myStrokeChanged = true; forceRecook(); }
    /// This one must map from SOP_BrushOp into our own op menu.
    /// Thus it is left to the C file.
    virtual void setBrushOp(SOP_BrushOp op);
    /// As we are always returning CIRCLE for SHAPE(), we ignore
    /// requests to change the brush shape.
    virtual void setBrushShape(SOP_BrushShape /*shape*/) {}
    /// As we are locking projection type to not orient to surface,
    /// we can ignore these change requests (see PROJECTIONTYPE)
    virtual void setProjectionType(int /*projtype*/) {}
    virtual void useForegroundColor()
	{ myUseFore = true; }
    virtual void useBackgroundColor()
	{ myUseFore = false; }
    /// This is used by the eyedropper to write into the current colour
    /// field.
    virtual void setCurrentColor(const UT_Vector3 &cd)
	{
	    setFloat(myUseFore ? "flen" : "blen", 0, 0, cd.x());
	}

    /// This is used to update the radii from the state:
    virtual void setRadius(fpreal r, fpreal t)
    {
	setFloat("radius", 0, t, r);
    }
    virtual void setUVRadius(fpreal r, fpreal t)
    {
	setFloat("uvradius", 0, t, r);
    }

protected:
    /// This method handles the erase callback relevant to this type
    /// of brush.
    virtual void	doErase();

private:
    /// Here we cache the current ray hit values...
    UT_Vector3		myRayOrient, myRayHit;
    float		myRayHitU, myRayHitV, myRayHitW;
    float		myRayHitPressure;
    int			myPrimHit;
    int			myPtHit;
    int			myEvent;
    bool		myUseFore;
    bool		myStrokeChanged;


    /// These are cached for the brushop callback to know the attribute
    /// indices.
    bool		myHairlenFound;
    GA_RWHandleF	myHairlenHandle;
    fpreal		myTime;
};
} // End HDK_Sample namespace

#endif
