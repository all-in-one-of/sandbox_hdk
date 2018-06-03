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
 * NAME:	GUI_PrimFramework.h (GR Library, C++)
 *
 * COMMENTS:
 *
 *	Framework for a render hook, with documentation. Does not actually do
 *	anything other than act as a commented template.
 *
 */

#ifndef __GUI_PrimFramework__
#define __GUI_PrimFramework__

#include <GUI/GUI_PrimitiveHook.h>
#include <GR/GR_Primitive.h>

namespace HDK_Sample
{
    
/// The primitive render hook which creates GUI_PrimFramework objects.
class GUI_PrimFrameworkHook : public GUI_PrimitiveHook
{
public:
	     GUI_PrimFrameworkHook();
    virtual ~GUI_PrimFrameworkHook();

    /// This is called when a new GR_Primitive is required for a polysoup.
    /// In this case, the hook is augmenting the rendering of the polysoup by
    /// drawing a bounding box and a centroid around it.
    virtual GR_Primitive  *createPrimitive(const GT_PrimitiveHandle &gt_prim,
					   const GEO_Primitive	*geo_prim,
					   const GR_RenderInfo	*info,
					   const char		*cache_name,
					   GR_PrimAcceptResult  &processed);
};

/// Primitive object that is created by GUI_PrimFrameworkHook whenever a
/// a matching primitive is found.
class GUI_PrimFramework : public GR_Primitive
{
public:
			 GUI_PrimFramework(const GR_RenderInfo *info,
					 const char *cache_name,
					 const GEO_Primitive *prim);
    virtual		~GUI_PrimFramework();

    virtual const char *className() const { return "GUI_PrimFramework"; }

    /// See if the primitive can be consumed by this GR_Primitive. Only
    /// primitives from the same detail will ever be passed in. If the
    /// primitive hook specifies GUI_HOOK_COLLECT_PRIMITIVES then it is
    /// possible to have this called more than once for different GR_Primitives.
    /// A GR_Primitive that collects multiple GT or GEO primitives is
    /// responsible for keeping track of them (in a list, table, tree, etc).
    virtual GR_PrimAcceptResult	acceptPrimitive(GT_PrimitiveType t,
						int geo_type,
						const GT_PrimitiveHandle &ph,
						const GEO_Primitive *prim);

    /// Called whenever certain geometry-affecting display options are changed.
    /// This gives you the opportunity to see if you need to update the
    /// geometry based on what changed (you will need to cache the display
    /// options that you are interested in to determine this). Return:
    ///    DISPLAY_UNCHANGED  - no update required.
    ///    DISPLAY_CHANGED    - additional geometry attributes may be required
    ///    DISPLAY_VERSION_CHANGED - existing geometry must be redone
    /// If 'first_init' is true, this is being called to initialize any cached
    /// display options. The return value will be ignored. This will happen
    /// just after the primitive is created.
    virtual GR_DispOptChange displayOptionChange(const GR_DisplayOption &opts,
						 bool first_init);
    
    /// If this primitive requires an update when the view changes, return true.
    virtual bool	updateOnViewChange(const GR_DisplayOption &) const
			{ return false; }
    
    /// If updateOnViewChange() returns true, this is called when the view
    /// changes, but no other update reasons are present. Note that you do not
    /// have access to the primitive in this method, so you will need to cache
    /// anything relating to the primitive required by this method.
    virtual void	viewUpdate(RE_Render *r,
				   const GR_ViewUpdateParms &parms)
			{}
    /// For primitives that may need updating if the GL state changes, this
    /// hook allows you to perform a check if no update is otherwise required.
    /// Return true to have checkGLState() called. Returning true from that
    /// will trigger an update with the GR_GL_STATE_CHANGED reason set.
    /// @{
    virtual bool	needsGLStateCheck(const GR_DisplayOption &opts) const
			{ return false; }
    virtual bool	checkGLState(RE_Render *r,const GR_DisplayOption &opts)
			{ return false; }
    /// @}
    
    /// Called whenever the parent detail is changed, draw modes are changed,
    /// selection is changed, or certain volatile display options are changed
    /// (such as level of detail).
    virtual void	update(RE_Render		 *r,
			       const GT_PrimitiveHandle  &primh,
			       const GR_UpdateParms	 &p);

    /// return true if the primitive is in or overlaps the view frustum.
    /// always returning true will effectively disable frustum culling.
    virtual bool	inViewFrustum(const UT_Matrix4D &objviewproj)
			    { return true; }

    /// Called to do a variety of render tasks (beauty, wire, shadow, object
    /// pick)
    virtual void	render(RE_Render	      *r,
			       GR_RenderMode	       render_mode,
			       GR_RenderFlags	       flags,
			       GR_DrawParms	       draw_parms);
    
    /// Called when decoration 'decor' is required to be rendered.
    virtual void	 renderDecoration(RE_Render *r,
					  GR_Decoration decor,
					  const GR_DecorationParms &p);
    
    /// Render this primitive for picking, where pick_type is defined as one of
    /// the pickable bits in GU_SelectType.h (like GU_PICK_GEOPOINT)
    /// return the number of picks
    virtual int		renderPick(RE_Render *r,
				   const GR_DisplayOption *opt,
				   unsigned int pick_type,
				   GR_PickStyle pick_style,
				   bool has_pick_map);

private:
};


} // End HDK_Sample namespace.

#endif
