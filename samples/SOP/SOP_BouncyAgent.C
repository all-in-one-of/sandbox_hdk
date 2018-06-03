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

/*! @file SOP_BouncyAgent.C

  @brief Demonstrates example for creating a procedural agent primitive.

  The node creates an agent primitive for every point from its input geometry
  that uses the same shared agent definition. The agent definition itself is a
  unit polygonal sphere that has skin weights bound to two joints (a parent and
  a child) with animation that bounces it in TY.

*/

#include "SOP_BouncyAgent.h"

#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_Type.h>
#include <CH/CH_Manager.h>
#include <GU/GU_Agent.h>
#include <GU/GU_AgentRig.h>
#include <GU/GU_AgentShapeLib.h>
#include <GU/GU_AgentLayer.h>
#include <GU/GU_PrimPacked.h>
#include <GU/GU_PrimSphere.h>
#include <GEO/GEO_AttributeCaptureRegion.h>
#include <GEO/GEO_AttributeIndexPairs.h>
#include <GA/GA_AIFIndexPair.h>
#include <CL/CL_Clip.h>
#include <CL/CL_Track.h>
#include <UT/UT_Array.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_String.h>
#include <UT/UT_StringArray.h>
#include <UT/UT_WorkBuffer.h>
#include <stdlib.h>

using namespace HDK_Sample;

// Provide entry point for installing this SOP.
void
newSopOperator(OP_OperatorTable *table)
{
    OP_Operator *op = new OP_Operator(
        "hdk_bouncyagent",
        "BouncyAgent",
        SOP_BouncyAgent::myConstructor,
        SOP_BouncyAgent::myTemplateList,
        1,      // min inputs
        1       // max inputs
        );
    table->addOperator(op);
}

OP_Node *
SOP_BouncyAgent::myConstructor(
	OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_BouncyAgent(net, name, op);
}

// SOP Parameters.
static PRM_Name	    sopAgentName("agentname", "Agent Name");
static PRM_Default  sopAgentNameDef(0, "agent1");
static PRM_Name	    sopHeight("height", "Height");
static PRM_Default  sopHeightDef(5.0);
static PRM_Name	    sopClipLength("cliplength", "Clip Length"); // seconds
static PRM_Name	    sopClipOffset("clipoffset", "Clip Offset"); // seconds
static PRM_Default  sopClipOffsetDef(0, "$T");
static PRM_Range    sopClipOffsetRange(PRM_RANGE_UI, 0, PRM_RANGE_UI, 10);
static PRM_Name	    sopReload("reload", "Reload");

PRM_Template
SOP_BouncyAgent::myTemplateList[] =
{
    PRM_Template(PRM_ALPHASTRING,   1, &sopAgentName, &sopAgentNameDef),
    PRM_Template(PRM_FLT_J,	    1, &sopHeight, &sopHeightDef),
    PRM_Template(PRM_FLT_J,	    1, &sopClipLength, PRMoneDefaults,
				    0, &PRMrulerRange),
    PRM_Template(PRM_FLT_J,	    1, &sopClipOffset, &sopClipOffsetDef,
				    0, &sopClipOffsetRange),
    PRM_Template(PRM_CALLBACK,	    1, &sopReload, 0, 0, 0,
				    &SOP_BouncyAgent::onReload),
    PRM_Template() // sentinel
};

// Constructor
SOP_BouncyAgent::SOP_BouncyAgent(
	OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op)
{
    // This indicates that this SOP manually manages its data IDs,
    // so that Houdini can identify what attributes may have changed,
    // e.g. to reduce work for the viewport, or other SOPs that
    // check whether data IDs have changed.
    // By default, (i.e. if this line weren't here), all data IDs
    // would be bumped after the SOP cook, to indicate that
    // everything might have changed.
    // If some data IDs don't get bumped properly, the viewport
    // may not update, or SOPs that check data IDs
    // may not cook correctly, so be *very* careful!
    mySopFlags.setManagesDataIDs(true);
}

// Destructor
SOP_BouncyAgent::~SOP_BouncyAgent()
{
}

enum
{
    SOP_SKIN_RIG_INDEX = 0,
    SOP_PARENT_RIG_INDEX,
    SOP_CHILD_RIG_INDEX,
    // Set up exclusive rig index range for joints
    SOP_JOINT_BEGIN = SOP_PARENT_RIG_INDEX,
    SOP_JOINT_END   = SOP_CHILD_RIG_INDEX + 1
};

// Create the rig.
// Rigs define the names of the transforms in a tree hierarchy.
static GU_AgentRigPtr
sopCreateRig(const char *path)
{
    UT_String rig_name = path;
    rig_name += "?rig";
    GU_AgentRigPtr rig = GU_AgentRig::addRig(rig_name);

    UT_StringArray transforms;
    transforms.append("skin");	    // SOP_SKIN_RIG_INDEX
    transforms.append("parent");    // SOP_PARENT_RIG_INDEX
    transforms.append("child");	    // SOP_CHILD_RIG_INDEX

    UT_IntArray child_counts;
    child_counts.append(0); // 'skin' has 0 children
    child_counts.append(1); // 'parent' has 1 child
    child_counts.append(0); // 'child' has 0 children

    UT_IntArray children;
    // 'skin' has no children so we don't need to append anything for it
    children.append(SOP_CHILD_RIG_INDEX); // 'parent' has 'child' as the child
    // 'child' has no children so we don't need to append anything for it

    // now construct the rig
    if (!rig->construct(transforms, child_counts, children))
	return nullptr;
    return rig;
}

static GU_Detail *
sopCreateSphere(bool for_default)
{
    GU_Detail *shape = new GU_Detail;
    GU_PrimSphereParms sphere(shape);
    if (for_default)
    {
	sphere.freq = 2;
	sphere.type = GEO_PATCH_TRIANGLE;
	GU_PrimSphere::build(sphere, GEO_PRIMPOLY);
    }
    else
    {
	// Position and scale the collision sphere to account for the
	// deformation animation.
	sphere.xform.scale(1.5, 1, 1.5);
	sphere.xform.translate(0, 1, 0.5);
	GU_PrimSphere::build(sphere, GEO_PRIMSPHERE);
    }
    return shape;
}

// For simplicity, we bind all points to all the transforms except for
// SOP_SKIN_RIG_INDEX.
static void
sopAddSkinWeights(const GU_AgentRig &rig, const GU_DetailHandle &shape)
{
    GU_DetailHandleAutoWriteLock gdl(shape);
    GU_Detail *gdp = gdl.getGdp();

    // Create skinning attribute with 2 regions and each point is bound to both
    // rig transforms.
    int num_regions = (SOP_JOINT_END - SOP_JOINT_BEGIN);
    GEO_Detail::geo_NPairs num_pairs_per_pt(num_regions);
    GA_RWAttributeRef capt = gdp->addPointCaptureAttribute(num_pairs_per_pt);
    int regions_i = -1;
    GA_AIFIndexPairObjects *regions
	= GEO_AttributeCaptureRegion::getBoneCaptureRegionObjects(
							    capt, regions_i);
    regions->setObjectCount(num_regions);

    // Tell the skinning attribute the names of the rig transforms. This needs
    // to be done after calling regions->setObjectCount().
    GEO_RWAttributeCapturePath paths(gdp);
    for (int i = 0; i < num_regions; ++i)
	paths.setPath(i, rig.transformName(SOP_JOINT_BEGIN + i));

    // Set up the rest transforms for the skin weight bindings. For efficiency,
    // these are actually stored in the skin weight bindings as the INVERSE of
    // the world space rest transform of the joint.
    for (int i = 0; i < num_regions; ++i)
    {
	GEO_CaptureBoneStorage r;
	// Recall that the unit sphere is created at the origin. So to position
	// the parent joint at the base of the sphere, it should be a position
	// (0, -1, 0). However, since these are actually inverses, we'll
	// translate by the opposite sign instead.
	if (i == 0)
	    r.myXform.translate(0, +1.0, 0);
	else
	    r.myXform.translate(0, -1.0, 0);
	regions->setObjectValues(i, regions_i, r.floatPtr(),
				 GEO_CaptureBoneStorage::tuple_size);
    }

    // Set up the weights
    const GA_AIFIndexPair *weights = capt->getAIFIndexPair();
    weights->setEntries(capt, num_regions);
    for (GA_Offset ptoff : gdp->getPointRange())
    {
	for (int i = 0; i < num_regions; ++i)
	{
	    // Set the region index that the point is captured by.
	    // Note that these index into 'paths' above.
	    weights->setIndex(capt, ptoff, /*entry*/i, /*region*/i);
	    // Set the weight that the point is captured by transform i.
	    // Notice that all weights for the point should sum to 1.
	    fpreal weight = 1.0/num_regions;
	    weights->setData(capt, ptoff, /*entry*/i, weight);
	}
    }
}

// Default convention is to prefix the shape name by the layer name
#define SOP_DEFAULT_SKIN_NAME	GU_AGENT_LAYER_DEFAULT".skin"
#define SOP_COLLISION_SKIN_NAME	GU_AGENT_LAYER_COLLISION".skin"

// Create the shape library which contains a list of all the shape geometries
// that can be attached to the rig transforms.
static GU_AgentShapeLibPtr
sopCreateShapeLib(const char *path, const GU_AgentRig &rig)
{
    UT_String shapelib_name = path;
    shapelib_name += "?shapelib";
    GU_AgentShapeLibPtr shapelib = GU_AgentShapeLib::addLibrary(shapelib_name);

    GU_DetailHandle skin_geo;
    skin_geo.allocateAndSet(sopCreateSphere(true), /*own*/true);
    sopAddSkinWeights(rig, skin_geo);
    shapelib->addShape(SOP_DEFAULT_SKIN_NAME, skin_geo);

    // The collision geometry is intended to be simplified versions of the
    // default layer shapes. The bounding box for the default layer shape is
    // computed from the corresponding collision layer shape.
    GU_DetailHandle coll_geo;
    coll_geo.allocateAndSet(sopCreateSphere(false), /*own*/true);
    shapelib->addShape(SOP_COLLISION_SKIN_NAME, coll_geo);

    return shapelib;
}

// Create a default layer with the sphere as the geometry bound to the rig.
// Layers assign geometry from the shapelib to be used for the rig
// transforms.
// Agents must have at least 2 layers:
//	    GU_AGENT_LAYER_DEFAULT ("default"
//		- Used for display/render
//	    GU_AGENT_LAYER_COLLISION ("collision")
//		- Simple geometry to be used for the bounding box that
//		  encompasses the corresponding shape in default layer for all
//		  possible local deformations.
// In general, it can have more layers and we can set which of those we use
// for the default and collision.
static GU_AgentLayerPtr
sopCreateDefaultLayer(
	const char *path,
	const GU_AgentRigPtr &rig,
	const GU_AgentShapeLibPtr &shapelib)
{
    UT_StringArray shape_names;
    UT_IntArray transform_indices;
    UT_Array<bool> deforming;

    // Simply bind the skin geometry to the 'skin' transform which we know is
    // transform index 0.
    shape_names.append(SOP_DEFAULT_SKIN_NAME);
    transform_indices.append(SOP_SKIN_RIG_INDEX);
    deforming.append(true); // has skin weights

    UT_String unique_name = path;;
    unique_name += "?default_layer";
    GU_AgentLayerPtr layer
	= GU_AgentLayer::addLayer(unique_name, rig, shapelib);
    if (!layer->construct(shape_names, transform_indices, deforming))
	return nullptr;

    UT_StringHolder layer_name(UTmakeUnsafeRef(GU_AGENT_LAYER_DEFAULT));
    layer->setName(layer_name);

    return layer;
}

// See also comments for sopCreateDefaultLayer().
static GU_AgentLayerPtr
sopCreateCollisionLayer(
	const char *path,
	const GU_AgentRigPtr &rig,
	const GU_AgentShapeLibPtr &shapelib)
{
    UT_StringArray shape_names;
    UT_IntArray transform_indices;
    UT_Array<bool> deforming;

    // For character rigs, the collision shapes are typically attached to the
    // joint transforms so that they can proxy for skin deformation.
    shape_names.append(SOP_COLLISION_SKIN_NAME);
    transform_indices.append(SOP_PARENT_RIG_INDEX);
    deforming.append(false); // has NO skin weights

    UT_String unique_name = path;;
    unique_name += "?collision_layer";
    GU_AgentLayerPtr
	layer = GU_AgentLayer::addLayer(unique_name, rig, shapelib);
    if (!layer->construct(shape_names, transform_indices, deforming))
	return nullptr;

    UT_StringHolder layer_name(UTmakeUnsafeRef(GU_AGENT_LAYER_COLLISION));
    layer->setName(layer_name);

    return layer;
}

static fpreal*
sopAddTrack(CL_Clip &chans, const GU_AgentRig &rig, int i, const char *trs_name)
{
    UT_WorkBuffer str;
    str.sprintf("%s:%s", rig.transformName(i).buffer(), trs_name);
    return chans.addTrack(str.buffer())->getData();
}

// Create some bouncy animation for the agent
static GU_AgentClipPtr
sopCreateBounceClip(CL_Clip& chans, const GU_AgentRigPtr &rig, fpreal height)
{
    int num_samples = chans.getTrackLength();

    // Set the ty of the 'parent' transform that bounces. Note that these
    // transforms here are in local space. The valid channel names are:
    //	    Translate:	tx ty tz
    //	    Rotate:	rx ry rz (euler angles in degrees, XYZ rotation order)
    //	    Scale:	sx sy sz
    fpreal *ty = sopAddTrack(chans, *rig, SOP_PARENT_RIG_INDEX, "ty");
    fpreal *sy = sopAddTrack(chans, *rig, SOP_PARENT_RIG_INDEX, "sy");
    for (int i = 0; i < num_samples; ++i)
    {
	ty[i] = height * sin(i * M_PI / (num_samples-1));
	// add some squash and stretch
	sy[i] = 1.0 - SYSabs(0.5 * sin(i * M_PI / (num_samples-1)));
    }

    // Set the ty of the 'child' transform. Note that these transforms here are
    // in local space.
    fpreal *tz = sopAddTrack(chans, *rig, SOP_CHILD_RIG_INDEX, "tz");
    ty = sopAddTrack(chans, *rig, SOP_CHILD_RIG_INDEX, "ty");
    for (int i = 0; i < num_samples; ++i)
    {
	ty[i] = 2.0;					// sphere diameter
	tz[i] = 1.5 * sin(i * M_PI / (num_samples-1));	// sway a bit forwards
    }

    // Finally load the agent clip from the CL_Clip animation we created
    GU_AgentClipPtr clip = GU_AgentClip::addClip("bounce", rig);
    if (!clip)
	return nullptr;
    clip->load(chans);

    return clip;
}

// Enable this to debug our definition
#define SOP_SAVE_AGENT_DEFINITION 0

// Create the agent definition
GU_AgentDefinitionPtr
SOP_BouncyAgent::createDefinition(fpreal t) const
{
    // Typically, the definition is loaded from disk which has a filename for
    // each of the different parts. Since we're doing this procedurally, we're
    // going to just make up some arbitrary unique names using our node path.
    UT_String path;
    getFullPath(path);

    GU_AgentRigPtr rig = sopCreateRig(path);
    if (!rig)
	return nullptr;

    GU_AgentShapeLibPtr shapelib = sopCreateShapeLib(path, *rig);
    if (!shapelib)
	return nullptr;

    GU_AgentLayerPtr default_layer = sopCreateDefaultLayer(path,
							   rig, shapelib);
    if (!default_layer)
	return nullptr;

    GU_AgentLayerPtr collision_layer = sopCreateCollisionLayer(path,
							       rig, shapelib);
    if (!collision_layer)
	return nullptr;

    CL_Clip chans(CHgetManager()->getSample(CLIPLENGTH(t)));
    chans.setSampleRate(CHgetManager()->getSamplesPerSec());
    GU_AgentClipPtr clip = sopCreateBounceClip(chans, rig, HEIGHT(t));
    if (!clip)
	return nullptr;

#if SOP_SAVE_AGENT_DEFINITION
    // Once we have the definition, we can save out the files for loading with
    // the Agent SOP as well. Or, we can examine them for debugging purposes.
    {
	UT_AutoJSONWriter writer("bouncy_rig.rig", /*binary*/false);
	rig->save(writer);
    }
    {
	UT_AutoJSONWriter writer("bouncy_shapelib.bgeo", /*binary*/true);
	shapelib->save(writer);
    }
    {
	UT_AutoJSONWriter writer("bouncy_layer.default.lay", /*binary*/false);
	default_layer->save(writer);
    }
    {
	UT_AutoJSONWriter writer("bouncy_layer.collision.lay", /*binary*/false);
	collision_layer->save(writer);
    }
    {
	chans.save("bouncy_bounce.bclip");
    }
#endif

    // The agent definition is used to create agent primitives. Many agent
    // primitives can share the same definition.
    GU_AgentDefinitionPtr def(new GU_AgentDefinition(rig, shapelib));
    if (!def)
	return nullptr;
    // The definition has a number of layers that be can assigned
    def->addLayer(default_layer);
    def->addLayer(collision_layer);
    // ... and a number of clips that can be assigned to specific agent prims
    def->addClip(clip);

    return def;
}

/*static*/ int
SOP_BouncyAgent::onReload(
	void *data, int index, fpreal t, const PRM_Template *tplate)
{
    SOP_BouncyAgent* sop = static_cast<SOP_BouncyAgent*>(data);
    if (!sop->getHardLock()) // only allow reloading if we're not locked
    {
	sop->myDefinition.reset();
	sop->forceRecook();
    }
    return 1;
}

// Compute the output geometry for the SOP.
OP_ERROR
SOP_BouncyAgent::cookMySop(OP_Context &context)
{
    fpreal t = context.getTime();

    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    // Duplicate the input geometry, but only if it was changed
    int input_changed;
    duplicateChangedSource(/*input*/0, context, &input_changed);

    // Detect if we need to rebuild the agent definition. For simplicity, we'll
    // rebuild if any of our agent parameters changed. Note the use of the
    // bitwise or operator to ensure that isParmDirty() is always called for
    // all parameters.
    bool agent_changed = isParmDirty(sopAgentName.getToken(), t);
    agent_changed = isParmDirty(sopHeight.getToken(), t);
    agent_changed |= isParmDirty(sopClipLength.getToken(), t);

    if (!myDefinition)
    {
	agent_changed = true;
	input_changed = true;
    }

    if (agent_changed)
    {
	myDefinition = createDefinition(t);
	if (!myDefinition)
	{
	    addError(SOP_MESSAGE, "Failed to create definition");
	    return error();
	}
    }

    if (input_changed)
    {
	// Delete all the primitives, keeping only the points
	gdp->destroyPrimitives(gdp->getPrimitiveRange());

	// These UT_StringHolder objects make the hash table look ups more
	// efficient.
        UT_StringHolder default_layer(
			    UTmakeUnsafeRef(GU_AGENT_LAYER_DEFAULT));
        UT_StringHolder collision_layer(
			    UTmakeUnsafeRef(GU_AGENT_LAYER_COLLISION));

	// Create the agent primitives
	myPrims.clear();
	for (GA_Offset ptoff : gdp->getPointRange())
	{
            myPrims.append(GU_Agent::agent(*gdp, default_layer,
                                           collision_layer, ptoff));
        }

	// Bumping these 2 attribute owners is what we need to do when adding
	// pack agent prims because it has a single vertex.
	gdp->getAttributes().bumpAllDataIds(GA_ATTRIB_VERTEX);
	gdp->getAttributes().bumpAllDataIds(GA_ATTRIB_PRIMITIVE);
        gdp->getPrimitiveList().bumpDataId(); // modified primitives
    }

    if (agent_changed || input_changed)
    {
	// Create a name attribute for the agents
	GA_RWHandleS name_attrib(gdp->addStringTuple(GA_ATTRIB_PRIMITIVE,
						     "name", 1));

	// Set the agent definition to the agent prims
	UT_WorkBuffer name;
	UT_String agent_name;
	AGENTNAME(agent_name, t);
	int name_i = 0;
	for (GU_PrimPacked *pack : myPrims)
	{
	    GU_Agent* agent = UTverify_cast<GU_Agent*>(pack->implementation());
	    agent->setDefinition(myDefinition);

	    // We only have 1 clip that can be used in the definition here.
	    UT_StringArray clips;
	    clips.append(myDefinition->clip(0).name());
	    agent->setClipsByNames(clips);

	    // Convention for the agent primitive names is agentname_0,
	    // agentname_1, agentname_2, etc.
	    name.sprintf("%s_%d", agent_name.buffer(), name_i);
	    name_attrib.set(pack->getMapOffset(), name.buffer());
	    ++name_i;
	}

	// Mark what modified
        gdp->getPrimitiveList().bumpDataId();
	name_attrib.bumpDataId(); 
    }

    // Set the clip information for the agents. In general, agents can be set
    // to evaluate an blended array of clips to evaluated at a specific clip
    // offset.
    for (GU_PrimPacked *pack : myPrims)
    {
	GU_Agent* agent = UTverify_cast<GU_Agent*>(pack->implementation());
	agent->setClipTime(/*clip index*/0, CLIPOFFSET(t));
    }
    gdp->getPrimitiveList().bumpDataId(); // we modified primitives

    return error();
}

// Provide input labels.
const char *
SOP_BouncyAgent::inputLabel(unsigned /*input_index*/) const
{
    return "Points to attach agents";
}

