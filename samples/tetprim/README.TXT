== What this is ==

This provides an example of adding a new geometry primitive to Houdini.
It is a simple tetrahedron of 4 points.

== Notes on how it works ==

You need to subclass off both GU_Primitive and GEO_Primitive for new
primitives.  The GEO subclass defines most of the structure of the primitive,
the number of vertices, etc.  The GU layer defines utility functions such
as conversion.

The GT class is used by the new viewport rendering code. This will create a 
polygon mesh for the tetmesh. 

The GR class can also be used to define a render hook for the tetmesh. It will
render it in the viewport, and also has greater ability to display decorations
and guides in addition to the basic mesh. If you want to use the render hook
instead of the GT tesselation class, edit tetra.C and uncomment the #define for
TETRA_GR_PRIMITIVE. Render hooks can also collect multiple GEO primitives into
a single GR_Primitive - uncomment #define TETRA_GR_PRIM_COLLECTION to see how
this works. 

== How to build ==

hcustom tetra.C

If the SOP does not show up, 
setenv HOUDINI_DSO_ERROR 1
and look for any dso errors that are reported.
