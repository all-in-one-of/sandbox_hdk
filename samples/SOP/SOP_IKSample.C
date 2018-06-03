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
 *
 * The IKSample SOP
 *
 * Demonstrates example use of the Inverse Kinematics (IK) Solver found in the
 * KIN library.
 *
 */

#include "SOP_IKSample.h"
#include <KIN/KIN_Bone.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_Array.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_Matrix4.h>
#include <UT/UT_Quaternion.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_WorkBuffer.h>
#include <stdlib.h>

using namespace HDK_Sample;

// Provide entry point for installing this SOP.
void
newSopOperator(OP_OperatorTable *table)
{
    OP_Operator *op = new OP_Operator(
        "hdk_iksample",
        "IKSample",
        SOP_IKSample::myConstructor,
        SOP_IKSample::myTemplateList,
        1,      // min inputs
        1       // max inputs
        );
    table->addOperator(op);
}

OP_Node *
SOP_IKSample::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_IKSample(net, name, op);
}

// SOP Parameters.
static PRM_Name names[] =
{
    PRM_Name("goal",        "Goal Position"),
    PRM_Name("twist",       "Twist"),
    PRM_Name("dampen",      "Dampening"),
    PRM_Name("straighten",  "Straighten Solution"),
    PRM_Name("threshold",   "Threshold"),
};
static PRM_Default  sopThresholdDefault(1e-03f);

PRM_Template SOP_IKSample::myTemplateList[] =
{
    PRM_Template(PRM_FLT_J,     3, &names[0]),
    PRM_Template(PRM_ANGLE_J,   1, &names[1], 0, 0, &PRMangleRange),
    PRM_Template(PRM_FLT_J,     1, &names[2]),
    PRM_Template(PRM_TOGGLE_J,  1, &names[3]),
    PRM_Template(PRM_FLT_J,     1, &names[4], &sopThresholdDefault),
    PRM_Template() // sentinel
};

// Constructor
SOP_IKSample::SOP_IKSample(OP_Network *net, const char *name, OP_Operator *op)
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
SOP_IKSample::~SOP_IKSample()
{
}

// Evaluate parameters for the solver.
bool
SOP_IKSample::evaluateSolverParms(OP_Context &context, KIN_InverseParm &parms)
{
    fpreal  t = context.getTime();

    // the goal position is relative to chain root
    GOAL(t, parms.myEndAffectorPos);
    UT_Vector3 origin = gdp->getPos3(gdp->pointOffset(0));
    parms.myEndAffectorPos -= origin;

    parms.myTwist = TWIST(t);
    parms.myDampen = DAMPEN(t);
    parms.myResistStraight = STRAIGHTEN(t);
    parms.myTrackingThresholdFactor = THRESHOLD(t);
    parms.myTwistAffectorFlag = false;
    // if myTwistAffectorFlag is true, then also set the twist affector pos
    // parms.myInverseParms.myTwistAffectorPos = ...

    // Return true only if we didn't encounter any errors during parameter
    // evaluation.
    return (error() < UT_ERROR_ABORT);
}

// Setup myRestChain. The rest chain is used by the IK solver to determine
// the initial chain that is iteratively solved towards the goal position.
bool
SOP_IKSample::setupRestChain()
{
    GA_Size num_points = gdp->getNumPoints();
    GA_Size num_bones = num_points - 1;

    if (num_bones < 1)
    {
	UT_WorkBuffer str;
	str.sprintf("%d", 2 - (int)num_points);
	addError(SOP_NEED_MORE_POINTS, str.buffer());
	return false;
    }

    myRestChain.setNbones(num_bones);
    UT_Vector3R prev_pos = gdp->getPos3(gdp->pointOffset(0));
    UT_Vector3R prev_dir(0, 0, -1);
    for (GA_Size i = 0; i < num_bones; i++)
    {
	UT_Vector3R	pos = gdp->getPos3(gdp->pointOffset(GA_Index(i+1)));
	UT_Vector3R	dir = pos - prev_pos;
	fpreal		length = dir.length();
	fpreal		damp = 0;	// only used by "constraint" solver
	UT_Matrix4R	pre_xform(1);	// assume identity
	void		*data = NULL;	// not used
	UT_Vector3R	rot;
	UT_Quaternion	quat;

	// Since we're dealing with only point positions, calculate a natural
	// twist rotation using quaternions from the previous bone.
	dir.normalize();
	quat.updateFromVectors(prev_dir, dir);
	rot = quat.computeRotations(KIN_Chain::getXformOrder());
	rot.radToDeg();

	// Update the bone in the chain.
	myRestChain.updateBone(i, length, rot.data(), damp, pre_xform, data);

	// If we're dealing with the "constraint" solver, then we need to
	// do more setup here.
	//myRestChain.setConstraint(i, ...);

	// Update position and direction for next iteration.
	prev_pos = pos;
	prev_dir = dir;
    }

    return true;
}

// Compute the output geometry for the SOP.
OP_ERROR
SOP_IKSample::cookMySop(OP_Context &context)
{
    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    // Setup the rest chain if needed.
    int input_changed;
    duplicateChangedSource(/*input*/0, context, &input_changed);

    GA_RWHandleQ orient_attrib;
    if (input_changed)
    {
	if (!setupRestChain())
	    return error();

	// Create the "orient" attribute for storing our solved rotations.
	GA_Defaults def(GA_STORE_REAL64, 4,
		fpreal64(0), fpreal64(0), fpreal64(0), fpreal64(1) );
	orient_attrib = GA_RWHandleQ(gdp->addFloatTuple(GA_ATTRIB_POINT,"orient", 4, def));

	// Compute the "pscale" attribute using the bone lengths for
	// completeness. This allows us to easily use the BoneLink SOP along
	// with the Copy SOP.
	GA_RWHandleF pscale_attrib(gdp->addFloatTuple(GA_ATTRIB_POINT,"pscale", 1,
					  GA_Defaults(1.0)));
	exint i = 0;
        GA_Offset ptoff;
	GA_FOR_ALL_PTOFF(gdp, ptoff)
	{
	    float length = 0;
	    if (i < myRestChain.getNbones())
		length = myRestChain.getBone(i)->getLength();
	    pscale_attrib.set(ptoff, length);
	    i++;
	}

        // NOTE: Even though we used addFloatTuple, there could have been
        //       a pscale attribute already, which could have been reused,
        //       so we need to bump its data ID to indicate that we've
        //       modified it.
        pscale_attrib.bumpDataId();
    }

    // Evaluate parameters for solver.
    const char *solver_name = "inverse";
    KIN_InverseParm parms;
    if (!evaluateSolverParms(context, parms))
	return error();

    // Perform solve.
    KIN_Chain solution;
    if (!myRestChain.solve(solver_name, &parms, solution))
    {
	addError(SOP_MESSAGE, "Failed to solve.");
	return error();
    }

    // We store the solved orientations into the "orient" attribute that is
    // understood by the Copy SOP. We also compute "pscale" as well for
    // completeness. This allows us to easily use the BoneLink SOP along with
    // the Copy SOP.
    if (!orient_attrib.isValid())
    {
	orient_attrib = gdp->findFloatTuple(GA_ATTRIB_POINT, "orient", 4);
	if (!orient_attrib.isValid())
	{
	    addError(SOP_MESSAGE, "Failed to create orient attribute.");
	    return error();
	}
    }

    // Nothing to do if 1 or fewer points, and the code below
    // may crash for 0 points.
    if (gdp->getNumPoints() <= 1)
    {
        return error();
    }

    // Output geometry.
    UT_Matrix4R xform(1); // identity, this is the world transform

    UT_Vector3R pos = gdp->getPos3(gdp->pointOffset(GA_Index(0)));
    xform.setTranslates(pos);		// set chain origin

    GA_Size num_bones = gdp->getNumPoints() - 1;
    fpreal prev_length = 0;
    for (GA_Index i = 0; i < num_bones; i++)
    {
	const KIN_Bone *bone = solution.getBone(i);

	// Since we never actually set any pre-transforms, this leftMult()
	// ends up doing nothing.
	xform.leftMult(UT_R_FROM_F(bone->getExtraXform()));

	// Take the bone length into account for the point position.
	xform.pretranslate(0, 0, -1 * prev_length);

	xform.getTranslates(pos);
        GA_Offset ptoff = gdp->pointOffset(i);
	gdp->setPos3(ptoff, pos);

	// Update our world transform with the bone rotations. Note that
	// bone->getRotates() returns them in degrees.
	UT_Vector3R rot;
	bone->getRotates(rot.data());
	rot.degToRad();
	xform.prerotate(rot.x(), rot.y(), rot.z(), KIN_Chain::getXformOrder());

	// Stash the world transform's rotation into our orient attribute.
	UT_Matrix3R rot_xform(xform);
	UT_Quaternion q;
	q.updateFromRotationMatrix(rot_xform);
	orient_attrib.set(ptoff, q);

	prev_length = bone->getLength();
    }

    // set chain end position
    xform.pretranslate(0, 0, -1 * prev_length);
    xform.getTranslates(pos);
    GA_Offset ptoff = gdp->pointOffset(num_bones);
    gdp->setPos3(ptoff, pos);

    // We've modified orient and P, so we need to bump their data IDs.
    orient_attrib.bumpDataId();
    gdp->getP()->bumpDataId();

    return error();
}

// Provide input labels.
const char *
SOP_IKSample::inputLabel(unsigned /*input_index*/) const
{
    return "Points for IK";
}

