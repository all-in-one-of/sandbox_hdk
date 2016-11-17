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
 * This is a sample procedural DSO to generate a volumetric sphere
 */

#include "VRAY_DemoVolumeSphere.h"
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_FloatArray.h>
#include <UT/UT_IntArray.h>
#include <UT/UT_Array.h>
#include <UT/UT_StringArray.h>
#include <VRAY/VRAY_Volume.h>

//
// vray_VolumeSphere
//

namespace HDK_Sample {

/// @brief Volume primitive used by @ref VRAY/VRAY_DemoVolumeSphere.C
class vray_VolumeSphere : public VRAY_Volume {
public:
    virtual float	 getNativeStepSize() const { return 0.1F; }
    virtual void	 getBoxes(UT_Array<UT_BoundingBox> &boxes,
				  float radius,
				  float dbound,
				  float zerothreshold) const;
    virtual void	 getAttributeBinding(UT_StringArray &names,
					     UT_IntArray &sizes) const;
    virtual void	 evaluate(const UT_Vector3 &pos,
		    		  const UT_Filter &filter,
				  float radius, float time,
				  int idx, float *data) const;
    virtual UT_Vector3	 gradient(const UT_Vector3 &pos,
		    		  const UT_Filter &filter,
				  float radius, float time,
				  int idx) const;
};

}

using namespace HDK_Sample;

void
vray_VolumeSphere::getBoxes(UT_Array<UT_BoundingBox> &boxes,
			    float,
			    float dbound,
			    float) const
{
    // For this example, we simply create a single box which contains the unit
    // sphere which.  It's important to account for the displacement bound
    // (dbound)
    boxes.append(UT_BoundingBox(-1-dbound, -1-dbound, -1-dbound,
				 1+dbound,  1+dbound,  1+dbound));
}

void
vray_VolumeSphere::getAttributeBinding(UT_StringArray &names,
				    UT_IntArray &sizes) const
{
    // These are the "VEX" variables we provide to shaders.
    // "density[1]" represents the density of the sphere (1 inside, 0 outside)
    // "radius[1]" is the distance from the origin
    // Both are "float" types.
    //
    // The order in which they are added is used in the evaluate() method below
    names.append("density");
    sizes.append(1);
    names.append("radius");
    sizes.append(1);
}

void
vray_VolumeSphere::evaluate(const UT_Vector3 &pos,
			  const UT_Filter &,
			  float, float,
			  int idx, float *data) const
{
    switch (idx)
    {
	// 0 == "density"
	case 0: data[0] = (pos.length2() < 1.0F) ? 1.0F : 0.0F; break;
	// 1 == "radius"
	case 1: data[0] = pos.length(); break;

	default: UT_ASSERT(0 && "Invalid attribute evaluation");
    }
}

UT_Vector3
vray_VolumeSphere::gradient(const UT_Vector3 &,
			 const UT_Filter &,
			 float, float,
			 int) const
{
    return UT_Vector3(0, 0, 0);
}

//
// VRAY_DemoVolumeSphere
//

static VRAY_ProceduralArg	theArgs[] = {
    VRAY_ProceduralArg()
};

VRAY_Procedural *
allocProcedural(const char *)
{
    return new VRAY_DemoVolumeSphere();
}

const VRAY_ProceduralArg *
getProceduralArgs(const char *)
{
    return theArgs;
}

VRAY_DemoVolumeSphere::VRAY_DemoVolumeSphere()
{
}

VRAY_DemoVolumeSphere::~VRAY_DemoVolumeSphere()
{
}

int
VRAY_DemoVolumeSphere::initialize(const UT_BoundingBox *box)
{
    myBox = UT_BoundingBox(-1, -1, -1, 1, 1, 1);
    if (box)
	myBox.enlargeBounds(*box);

    return 1;
}

void
VRAY_DemoVolumeSphere::getBoundingBox(UT_BoundingBox &box)
{
    box = myBox;
}

void
VRAY_DemoVolumeSphere::render()
{
    VRAY_ProceduralChildPtr	obj = createChild();
    obj->addVolume(new vray_VolumeSphere, 0.0F);
}
