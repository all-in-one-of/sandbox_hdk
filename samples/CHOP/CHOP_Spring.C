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

#include <UT/UT_DSOVersion.h>
#include <OP/OP_OperatorTable.h>

#include <CH/CH_LocalVariable.h>
#include <PRM/PRM_Include.h>
#include <CHOP/PRM_ChopShared.h>

#include <UT/UT_Interrupt.h>
#include <UT/UT_IStream.h>
#include <UT/UT_OStream.h>

#include "CHOP_Spring.h"

using namespace HDK_Sample;

CHOP_SWITCHER(7, "Spring");

static PRM_Name	names[] = {
    PRM_Name("springk",	 	"Spring Constant"),
    PRM_Name("mass",	 	"Mass"),
    PRM_Name("dampingk", 	"Damping Constant"),
    PRM_Name("method",	 	"Input Effect"),
    PRM_Name("condfromchan",	"Initial Conditions From Channel"),
    PRM_Name("initpos", 	"Initial Position"),
    PRM_Name("initspeed", 	"Initial Speed"),
    PRM_Name(0),
};

static PRM_Name springMethodItems[] = {
    PRM_Name("disp",	"Position"),
    PRM_Name("force",	"Force"),
    PRM_Name(0),
};

static PRM_ChoiceList springMethodMenu((PRM_ChoiceListType)
	                               (PRM_CHOICELIST_EXCLUSIVE | 
					PRM_CHOICELIST_REPLACE),
				       springMethodItems);

static PRM_Range springConstantRange(PRM_RANGE_RESTRICTED, 0.0, 
				     PRM_RANGE_UI, 1000.0);

static PRM_Range massRange(PRM_RANGE_UI, 0.1, 
			   PRM_RANGE_UI, 10.0);

static PRM_Range dampingConstantRange(PRM_RANGE_RESTRICTED, 0.0, 
				      PRM_RANGE_UI, 10.0);

static PRM_Range initDispRange(PRM_RANGE_UI, -10.0, 
			       PRM_RANGE_UI,  10.0);

static PRM_Range initVelRange(PRM_RANGE_UI, -100.0, 
			      PRM_RANGE_UI,  100.0);

static PRM_Default     springConstantDefault(100.0);
static PRM_Default     massDefault(1.0);
static PRM_Default     dampingConstantDefault(1.0);
static PRM_Default     initDispDefault(0.0);
static PRM_Default     initVelDefault(0.0);

PRM_Template
CHOP_Spring::myTemplateList[] =
{
    PRM_Template(PRM_SWITCHER,	2, &PRMswitcherName, switcher),

    // First Page
    PRM_Template(PRM_FLT,	1, &names[0], &springConstantDefault, 0,
	    			   &springConstantRange),
    PRM_Template(PRM_FLT,	1, &names[1], &massDefault, 0,
	    			   &massRange),
    PRM_Template(PRM_FLT,	1, &names[2], &dampingConstantDefault, 0,
	    			   &dampingConstantRange),
    PRM_Template(PRM_ORD,	1, &names[3], PRMzeroDefaults,
	    			   &springMethodMenu),
    PRM_Template(PRM_TOGGLE,	1, &names[4], PRMoneDefaults),
    PRM_Template(PRM_FLT,	1, &names[5], &initDispDefault, 0,
	    			   &initDispRange),
    PRM_Template(PRM_FLT,	1, &names[6], &initVelDefault, 0,
	    			   &initVelRange),
    PRM_Template(),
};

OP_TemplatePair CHOP_Spring::myTemplatePair(
    CHOP_Spring::myTemplateList, &CHOP_Node::myTemplatePair);

bool
CHOP_Spring::updateParmsFlags()
{
    bool changes = CHOP_Node::updateParmsFlags();
    bool grab = GRAB_INITIAL();

    changes |= enableParm("initpos",	!grab);
    changes |= enableParm("initspeed",  !grab);

    return changes;
}


// 2 local variables, 'C' (the currently cooking track), 'NC' total # tracks.
enum
{
    VAR_C		= 200,
    VAR_NC		= 201
};
CH_LocalVariable
CHOP_Spring::myVariableList[] = {
    { "C",		VAR_C, 0 },
    { "NC",		VAR_NC, 0 },
    { 0, 0, 0 }
};
OP_VariablePair CHOP_Spring::myVariablePair(
    CHOP_Spring::myVariableList, &CHOP_Node::myVariablePair);

bool
CHOP_Spring::evalVariableValue(fpreal &val, int index, int thread)
{
    switch(index)
    {
    case VAR_C:
	// C marks the parameter as channel dependent - it must be re-eval'd
	// for each track processed.
	myChannelDependent=1;
	val = (fpreal)my_C;
	return true;
	
    case VAR_NC:
	val = (fpreal)my_NC;
	return true;
	
    }
    
    return CHOP_Node::evalVariableValue(val, index, thread);
}

//----------------------------------------------------------------------------

OP_Node *
CHOP_Spring::myConstructor(OP_Network	*net,
			const char	*name,
			OP_Operator	*op)
{
    return new CHOP_Spring(net, name, op);
}


CHOP_Spring::CHOP_Spring( OP_Network	*net,
		    const char	*name,
		    OP_Operator	*op)
	   : CHOP_Realtime(net, name, op)
{
    myParmBase = getParmList()->getParmIndex( names[0].getToken() );
    mySteady = 0;
}

CHOP_Spring::~CHOP_Spring()
{
}

// Regular cook method
OP_ERROR
CHOP_Spring::cookMyChop(OP_Context &context)
{
    const CL_Clip	*clip  = 0;
    const CL_Track	*track = 0;
    CL_Track		*new_track = 0;
    int			 force_method;
    int			 i, j,length, num_tracks, animated_parms;
    fpreal		 spring_constant;
    fpreal		 d1,d2,f,inc,d;
    fpreal		 mass;
    fpreal		 damping_constant;
    fpreal		 initial_displacement;
    fpreal		 initial_velocity;
    fpreal		 acc, vel;
    UT_Interrupt	*boss;
    int			 stop;
    int			 count = 0xFFFF;
    bool		 grab_init = GRAB_INITIAL();

    // Copy the structure of the input, but not the data itself.
    clip = copyInput(context, 0, 0, 1);	
    if (!clip)
	return error();

    force_method	 = METHOD();

    // Initialize local variables
    my_NC = clip->getNumTracks();
    my_C= 0;
    
    // evaluate all parameters and check if any are animated per channel with C
    myChannelDependent=0;
    spring_constant      = SPRING_CONSTANT(context.getTime());
    mass                 = MASS(context.getTime());
    damping_constant     = DAMPING_CONSTANT(context.getTime());
    animated_parms = myChannelDependent;
    
    inc = 1.0 / myClip->getSampleRate();

    // If 'grab initial' is true, we determine the initial values from the
    // input channel data instead of using the parameter settings (using
    // the position and slope of the track at the first sample).
    if(!grab_init)
    {
	initial_displacement = INITIAL_DISPLACEMENT(context.getTime());
	initial_velocity     = INITIAL_VELOCITY(context.getTime());
    }

    // An evaluation error occurred; exit.
    if (error() >= UT_ERROR_ABORT)
	return error();

    // Mass must be > 0. 
    if (mass < 0.001)
    {
	mass = 0.001;
	SET_MASS(context.getTime(), mass);
    }

    // Begin the interruptable operation
    boss = UTgetInterrupt();
    stop = 0;
    if(boss->opStart("Calculating Spring"))
    {
	if (clip)
	{
	    num_tracks = clip->getNumTracks();
	    length     = clip->getTrackLength();

	    // Loop over all the tracks
	    for (i=0; i<num_tracks; i++)
	    {
		// update the local variable 'C' with the track number
		my_C = i;
		
		track     = clip->getTrack(i);
		new_track = myClip->getTrack(i);

		// If the track is not scoped, copy it and continue to the next
		if (!isScoped(track->getName()))
		{
		    *new_track = *track;
		    continue;
		}
		
		if(grab_init || animated_parms)
		{
		    // re-evaluate parameters if one of them was determined to
		    // depend on the local var 'C' (track number)
		    if(animated_parms)
		    {
			spring_constant  = SPRING_CONSTANT(context.getTime());
			mass             = MASS(context.getTime());
			if (mass < 0.001)
			    mass = 0.001;
			damping_constant = DAMPING_CONSTANT(context.getTime());
		    }

		    // If determining the position and speed from the track,
		    // evaluate the first 2 samples and difference them.
		    if(grab_init)
		    {
			initial_displacement = clip->evaluateSingle(track,0);
			initial_velocity=(clip->evaluateSingle(track,1) -
					  initial_displacement);
		    }
		}

		// Run the spring algorithm on the track's data.
		d1 = initial_displacement; 
		d2 = d1 - initial_velocity * inc;
		
		for(j=0; j<length; j++)
		{
		    // Periodically check for interrupts.
		    if(count--==0 && boss->opInterrupt())
		    {
			stop = 1;
			break;
		    }

		    // run the spring equation
		    f = track->getData()[j];
 		    if(!force_method)
 			f *= spring_constant;

		    vel = (d1-d2) / inc;
		    
		    acc = (f - vel*damping_constant - d1*spring_constant)/mass;
		    vel += acc * inc;
		    d = d1 + vel * inc; 
		    
		    new_track->getData()[j] = d;

		    // update the previous displacements
		    d2 = d1;
		    d1 = d;
		}

		if(stop || boss->opInterrupt())
		{
		    stop = 1;
		    break;
		}
	    }
	}
    }
    // opEnd must always be called, even if opStart() returned 0.
    boss->opEnd();
    
    return error();
}

// -------------------------------------------------------------------------
// Time Slice cooking

// ---------------------------------------------------------------------------
// Realtime data block for stashing intermediate values between realtime cooks
// Only used in Time Slice mode.

namespace HDK_Sample {
class ut_SpringData : public ut_RealtimeData
{
public:
		ut_SpringData(const char *name,fpreal d,fpreal v);
    virtual    ~ut_SpringData() {}

    fpreal	myDn1;
    fpreal	myDn2;

    virtual bool	loadStates(UT_IStream &is, int version);
    virtual bool	saveStates(UT_OStream &os);
};
}

ut_SpringData::ut_SpringData(const char *name, fpreal d1, fpreal d2)
    : ut_RealtimeData(name),
      myDn1(d1),
      myDn2(d2)
{
}
 
bool
ut_SpringData::loadStates(UT_IStream &is, int version)
{
    if (!ut_RealtimeData::loadStates(is, version))
	return false;

    if (!is.read<fpreal64>(&myDn1))
	return false;
    if (!is.read<fpreal64>(&myDn2))
	return false;
    return true;
}
 
bool
ut_SpringData::saveStates(UT_OStream &os)
{
    ut_RealtimeData::saveStates(os);

    os.write<fpreal64>(&myDn1);
    os.write<fpreal64>(&myDn2);
    return true;
}
    
int
CHOP_Spring::isSteady() const
{
    // 'Steady' indicates that the CHOP has reached a steady state and can
    // stop cooking every frame. An input must change in order to begin
    // springing again.
    return mySteady;
}

OP_ERROR
CHOP_Spring::cookMySlice(OP_Context &context, int start, int end)
{
    const CL_Clip	*clip  = inputClip(0,context);
    const CL_Track	*track = 0;
    CL_Track		*new_track = 0;
    int			 force_method;
    int			 i, j;
    fpreal		 spring_constant;
    fpreal		 mass;
    fpreal		 d1,d2,f,t,inc,d, acc,vel,oldp;
    fpreal		 damping_constant;
    ut_SpringData	*block;
    fpreal		 delta;
    int			 animated_parms;
    
    force_method	 = METHOD();

    my_NC = clip->getNumTracks();
    my_C= 0;

    // Again, evaluate the parameters and see if they depend on C
    myChannelDependent=0;
    spring_constant      = SPRING_CONSTANT(context.getTime());
    mass                 = MASS(context.getTime());
    damping_constant     = DAMPING_CONSTANT(context.getTime());
    animated_parms = myChannelDependent;
    inc			 = 1.0 / myClip->getSampleRate();
    
    if (mass < 0.001)
	mass = 0.001;

    mySteady = 1;

    for(i=0; i<myClip->getNumTracks(); i++)
    {
	my_C = i;
	
	track	  = clip->getTrack(i);
	new_track = myClip->getTrack(i);

	// If this track isn't scoped, just copy the data.
	if(!isScoped(new_track->getName()))
	{
	    clip->evaluateTime(track,
			       myClip->getTime(start+myClip->getStart()),
			       myClip->getTime(end+myClip->getStart()),
			       new_track->getData(), myClip->getTrackLength());
	    continue;
	}

	// Re-evaluate the spring parameters if one of them depends on C
	if(animated_parms)
	{
	    spring_constant  = SPRING_CONSTANT(context.getTime());
	    mass             = MASS(context.getTime());
	    if (mass < 0.001)
	        mass = 0.001;
	    damping_constant = DAMPING_CONSTANT(context.getTime());
	}

	// Create or grab the realtime data block associated with this track.
	// This will keep our results from the previous cook, in this case,
	// the previous 2 displacements.
	block = (ut_SpringData *) getDataBlock(i);
	
	d1 = block->myDn1;
	d2 = block->myDn2;

	// Loop over each sample in the track.
	for(j=0; j<myClip->getTrackLength(); j++)
	{
	    t = myClip->getTime(myClip->getStart() + j);
	    oldp = f = clip->evaluateSingleTime(track, t);

	    // Run the spring equation
	    if(!force_method)
		f *= spring_constant;
	    else
		oldp /=spring_constant;

	    vel = (d1-d2) / inc;

	    acc = (f - vel*damping_constant - d1*spring_constant)/mass;
	    vel += acc * inc;
	    d = d1 + vel * inc; 
		
	    delta = SYSabs(oldp - d);
	    if (delta > 0.001)
		mySteady = 0;

	    new_track->getData()[j] = d;

	    d2 = d1;
	    d1 = d;
	}

	// update the displacements in the realtime data block for the next cook
	// to use.
	block->myDn1 = d1;
	block->myDn2 = d2;
   }

    return error();
}

ut_RealtimeData *
CHOP_Spring::newRealtimeDataBlock(const char *name,
				  const CL_Track *track,
				  fpreal t)
{
    fpreal		 d, d1, v, rate;

    // This creates a new realtime data block to stash intermediate values into.
    // In this case, the previous two displacements.
    if(GRAB_INITIAL() && track)
    {
	const CL_Clip	*clip = track ? track->getClip() : 0;
	
	d = clip->evaluateSingle(track,clip->getIndex(t));
	v = clip->evaluateSingle(track,clip->getIndex(t)+1) - d;
    }
    else
    {
	d = INITIAL_DISPLACEMENT(t);
	v = INITIAL_VELOCITY(t);
    }

    // The n-1 displacement is extrapolated from the slope at n and the
    // displacement (position) at n.
    rate = myClip->getSampleRate();
    if(rate != 0.0)
	d1 = d - v/rate;
    else
	d1 = d;

    return new ut_SpringData(name, d,d1);
}


// install the chop.
void newChopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator("hdk_spring", // node name
				       "HDK Spring", // pretty name
				       CHOP_Spring::myConstructor,
				       &CHOP_Spring::myTemplatePair,
				       1,       // min inputs
				       1,       // max inputs 
				       &CHOP_Spring::myVariablePair));
}
