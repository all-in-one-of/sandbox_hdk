== What this is ==

A sample of adding a new primitive to Houdini using the simplified
packed primitive interface.

== Notes on how it works ==

The GU_PackedSphere implements the interface for a packed primitive.
If you contrast this with the tetprim example, you'll see that this is
a much simpler interface.

The rendering of the primitive is done using the GT_GEOPackeSphere class which
behaves in exactly the same way as the default rendering for packed
primitives. This class can be removed and there should be no change to the
rendering of this primitive in the viewport.

The packedsphere.C stand-alone application saves a single packed
primitive to a disk file.

== How to build ==

hcustom GU_PackedSphere.C
hcustom -s packedsphere.C
