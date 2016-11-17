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
 * NAME:	GUI_PrimVolumeVelField.h (GR Library, C++)
 *
 * COMMENTS:
 *
 *	Framework for a render hook, with documentation. Does not actually do
 *	anything other than act as a commented template.
 *
 */

#ifndef __GUI_PrimVolumeVelField__
#define __GUI_PrimVolumeVelField__

#include <GUI/GUI_PrimitiveHook.h>
#include <GR/GR_Primitive.h>

class RE_Geometry;

namespace HDK_Sample
{
    
/// The primitive render hook which creates GUI_PrimVolumeVelField objects.
class GUI_PrimVolumeVelFieldHook : public GUI_PrimitiveHook
{
public:
	     GUI_PrimVolumeVelFieldHook();
    virtual ~GUI_PrimVolumeVelFieldHook();

    /// This is called when a new GR_Primitive is required for a polysoup.
    /// In this case, the hook is augmenting the rendering of the polysoup by
    /// drawing a bounding box and a centroid around it.
    virtual GR_Primitive  *createPrimitive(const GT_PrimitiveHandle &gt_prim,
					   const GEO_Primitive	*geo_prim,
					   const GR_RenderInfo	*info,
					   const char		*cache_name,
					   GR_PrimAcceptResult  &processed);
};

/// Primitive object that is created by GUI_PrimVolumeVelFieldHook whenever a
/// a matching primitive is found.
class GUI_PrimVolumeVelField : public GR_Primitive
{
public:
			GUI_PrimVolumeVelField(const GR_RenderInfo *info,
					       const char *cache_name);
    virtual	       ~GUI_PrimVolumeVelField();

    virtual const char *className() const { return "GUI_PrimVolumeVelField"; }

    virtual void	resetPrimitives();
    
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

    /// Called to do a variety of render tasks (beauty, wire, shadow, object
    /// pick)
    virtual void	render(RE_Render	      *r,
			       GR_RenderMode	       render_mode,
			       GR_RenderFlags	       flags,
			       const GR_DisplayOption *opt,
			       const RE_MaterialList  *materials);

    /// Similar to render(), but with object instances.
    virtual void	renderInstances(RE_Render	       *r,
					GR_RenderMode		render_mode,
					GR_RenderFlags		flags,
					const GR_DisplayOption *opt,
					const RE_MaterialList  *materials,
					int			instance_grp);
    
    virtual int		renderPick(RE_Render *r,
				   const GR_DisplayOption *opt,
				   unsigned int pick_type,
				   GR_PickStyle pick_style,
				   bool has_pick_map);

    // If the vector scale changes, this primitive needs to update.
    virtual GR_DispOptChange displayOptionChange(const GR_DisplayOption &opts,
						 bool first_init);
    
private:
    void		drawField(RE_Render	        *r,
				  GR_RenderMode		 render_mode,
				  GR_RenderFlags	 flags,
				  const GR_DisplayOption *opt,
				  const RE_MaterialList  *materials,
				  int			 instance_grp);

    void		draw(RE_Render *r,
			     int instance_group,
			     fpreal point_size = 2.0);
    
    RE_Geometry		*myField;
    GA_Offset		 myVX, myVY, myVZ;
    GA_Index		 myVXIdx, myVYIdx, myVZIdx;
    fpreal		 myVectorScale;
    bool		 myPrimSelected;
    bool		 myObjectSelected;
};


} // End HDK_Sample namespace.

#endif
