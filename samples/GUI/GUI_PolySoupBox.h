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
 * NAME:	GUI_PolySoupBox.h (GR Library, C++)
 *
 * COMMENTS:
 *
 *	Adds a display option for the bounding box and another for the
 *	barycenter of the entire polysoup. This hook only renders additional
 *	decorations. It does not draw the surface itself.
 *
 */

#ifndef __GUI_PolySoupBox__
#define __GUI_PolySoupBox__

#include <GUI/GUI_PrimitiveHook.h>
#include <GR/GR_Primitive.h>

class RE_Geometry;

namespace HDK_Sample
{
    
/// The primitive render hook which creates GUI_PolySoupBox objects.
class GUI_PolySoupBoxHook : public GUI_PrimitiveHook
{
public:
	     GUI_PolySoupBoxHook();
    virtual ~GUI_PolySoupBoxHook();

    /// This is called when a new GR_Primitive is required for a polysoup.
    /// In this case, the hook is augmenting the rendering of the polysoup by
    /// drawing a bounding box and a centroid around it.
    virtual GR_Primitive  *createPrimitive(const GT_PrimitiveHandle &gt_prim,
					   const GEO_Primitive	*geo_prim,
					   const GR_RenderInfo	*info,
					   const char		*cache_name,
					   GR_PrimAcceptResult  &processed);
};

/// Primitive object that is created by GUI_PolySoupBoxHook whenever a
/// polysoup primitive is found. This object can be persistent between
/// renders, though display flag changes or navigating though SOPs can cause
/// it to be deleted and recreated later.
class GUI_PolySoupBox : public GR_Primitive
{
public:
			 GUI_PolySoupBox(const GR_RenderInfo *info,
					 const char *cache_name,
					 const GEO_Primitive *prim);
    virtual		~GUI_PolySoupBox();

    virtual const char *className() const { return "GUI_PolySoupBox"; }

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

    /// Called whenever the parent detail is changed, draw modes are changed,
    /// selection is changed, or certain volatile display options are changed
    /// (such as level of detail).
    virtual void	update(RE_Render		 *r,
			       const GT_PrimitiveHandle  &primh,
			       const GR_UpdateParms	 &p);

    /// For this primitive hook, which only renders decorations, the render
    /// methods do nothing, allowing the native Houdini code to do the work.
    /// @{
    virtual void	render(RE_Render	      *r,
			       GR_RenderMode	       render_mode,
			       GR_RenderFlags	       flags,
			       const GR_DisplayOption *opt,
			       const RE_MaterialList  *materials);

    virtual void	renderInstances(RE_Render	       *r,
					GR_RenderMode		render_mode,
					GR_RenderFlags		flags,
					const GR_DisplayOption *opt,
					const RE_MaterialList  *materials,
					int instance_group);

    virtual int		renderPick(RE_Render *r,
				   const GR_DisplayOption *opt,
				   unsigned int pick_type,
				   GR_PickStyle pick_style,
				   bool has_pick_map);
    /// @}
    
    /// Called when decoration 'decor' is required to be rendered.
    virtual void	 renderDecoration(RE_Render *r,
					  GR_Decoration decor,
					  const GR_DecorationParms &p);
    
private:
    void	renderBBox(RE_Render *r,
			   const GR_DisplayOption *opts);

    void	renderBary(RE_Render *r,
			   const GR_DisplayOption *opts);
    
    RE_Geometry		*myGeometry;
};


} // End HDK_Sample namespace.

#endif
