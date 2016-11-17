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
 * NAME:	GUI_PolygonNormalShade.h (GR Library, C++)
 *
 * COMMENTS:
 *	Example of a filter hook.
 *	Replaces Cd in a GT polygon mesh with false N color, mapped from
 *	normalized [-1,1] to [0,1].
 *
 */
#ifndef GUI_PolygonNormalShade_h
#define GUI_PolygonNormalShade_h

#include <GUI/GUI_PrimitiveHook.h>

namespace HDK_Sample
{

/// Render hook which visualizes normals by painting a normal map on 
/// polygon geometry with Cd.
class GUI_PolygonNormalShade : public GUI_PrimitiveHook
{
public:
	     GUI_PolygonNormalShade();
    virtual ~GUI_PolygonNormalShade();

    /// The main virtual for filtering GT or GEO primitives. If the hook is
    /// interested in the primitive, it should set 'processed' to  GR_PROCESSED,
    /// even if it is not actively modifying the primitive (in which case
    /// return a NULL primitive handle). This ensures that the primitive will
    /// be updated when its display option is toggled. If not interested in the
    /// primitive at all, set 'processed' to GR_NOT_PROCESSED.
    virtual GT_PrimitiveHandle filterPrimitive(const GT_PrimitiveHandle &gt_prm,
					       const GEO_Primitive  *geo_prm,
					       const GR_RenderInfo  *info,
					       GR_PrimAcceptResult  &processed);
};

}
#endif
