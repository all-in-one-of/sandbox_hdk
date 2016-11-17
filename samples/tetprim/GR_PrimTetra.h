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
 * NAME:	GR_PrimTetra.h (GR Library, C++)
 *
 * COMMENTS:
 */

#ifndef __GR_PrimTetra__
#define __GR_PrimTetra__

#include <GUI/GUI_PrimitiveHook.h>
#include <GR/GR_Primitive.h>

namespace HDK_Sample
{
    
/// The primitive render hook which creates GR_PrimTetra objects.
class GR_PrimTetraHook : public GUI_PrimitiveHook
{
public:
	     GR_PrimTetraHook();
    virtual ~GR_PrimTetraHook();

    /// This is called when a new GR_Primitive is required for a tetra.
    /// gt_prim or geo_prim contains the GT or GEO primitive this object is being
    /// created for, depending on whether this hook is registered to capture
    /// GT or GEO primitives.
    /// info and cache_name should be passed down to the GR_Primitive
    /// constructor.
    /// processed should return GR_PROCESSED or GR_PROCESSED_NON_EXCLUSIVE if
    /// a primitive is created. Non-exclusive allows other render hooks or the
    /// native Houdini primitives to be created for the same primitive, which is
    /// useful for support hooks (drawing decorations, bounding boxes, etc). 
    virtual GR_Primitive  *createPrimitive(const GT_PrimitiveHandle &gt_prim,
					   const GEO_Primitive	*geo_prim,
					   const GR_RenderInfo	*info,
					   const char		*cache_name,
					   GR_PrimAcceptResult  &processed);
};
    
/// Primitive object that is created by GR_PrimTetraHook whenever a
/// tetrahedron primitive is found. This object can be persistent between
/// renders, though display flag changes or navigating though SOPs can cause
/// it to be deleted and recreated later.
class GR_PrimTetra : public GR_Primitive
{
public:
			 GR_PrimTetra(const GR_RenderInfo *info,
				      const char *cache_name,
				      const GEO_Primitive *prim);
    virtual		~GR_PrimTetra();

    virtual const char *className() const { return "GR_PrimTetra"; }

    /// See if the tetra primitive can be consumed by this primitive. Only
    /// tetra from the same detail will ever be passed in. If the primitive hook
    /// specifies GUI_HOOK_COLLECT_PRIMITIVES then it is possible to have this
    /// called more than once for different GR_Primitives. A GR_Primitive that
    /// collects multiple GT or GEO primitives is responsible for keeping track
    /// of them (in a list, table, etc).
    virtual GR_PrimAcceptResult	acceptPrimitive(GT_PrimitiveType t,
						int geo_type,
						const GT_PrimitiveHandle &ph,
						const GEO_Primitive *prim);

    /// For collection primitives (GUI_HOOK_COLLECT_PRIMITIVES), this is called
    /// before traversing the primitives. It will not be called for
    /// GUI_HOOK_PER_PRIMITIVE hooks. This should reset any lists of primitives.
    virtual void	resetPrimitives();


    /// Called whenever the parent detail is changed, draw modes are changed,
    /// selection is changed, or certain volatile display options are changed
    /// (such as level of detail).
    virtual void	update(RE_Render		 *r,
			       const GT_PrimitiveHandle  &primh,
			       const GR_UpdateParms	 &p);

    /// Called whenever the primitive is required to render, which may be more
    /// than one time per viewport redraw (beauty, shadow passes, wireframe-over)
    /// It also may be called outside of a viewport redraw to do picking of the
    /// geometry.
    virtual void	render(RE_Render	      *r,
			       GR_RenderMode	       render_mode,
			       GR_RenderFlags	       flags,
			       const GR_DisplayOption *opt,
			       const RE_MaterialList  *materials);
    virtual void	renderInstances(RE_Render *r,
					GR_RenderMode render_mode,
					GR_RenderFlags flags,
					const GR_DisplayOption *opt,
					const RE_MaterialList *materials,
					int render_instance);
    virtual void	renderDecoration(RE_Render *r,
					 GR_Decoration decor,
					 const GR_DecorationParms &parms);
    virtual int		renderPick(RE_Render *r,
				   const GR_DisplayOption *opt,
				   unsigned int pick_type,
				   GR_PickStyle pick_style,
				   bool has_pick_map);
private:
    int	myID;
    RE_Geometry *myGeo;
    
#ifdef TETRA_GR_PRIM_COLLECTION
    GA_IndexArray myPrimIndices;
#endif
};



} // End HDK_Sample namespace.

#endif
