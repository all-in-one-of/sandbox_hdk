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
 */

#include <SYS/SYS_Math.h>
#include <UT/UT_DSOVersion.h>
#include <OP/OP_OperatorTable.h>

#include <CH/CH_LocalVariable.h>
#include <PRM/PRM_Include.h>
#include <CHOP/PRM_ChopShared.h>

#include <CHOP/CHOP_Handle.h>
#include <OP/OP_Channels.h>
#include "CHOP_Stair.h"

using namespace HDK_Sample;

CHOP_SWITCHER2(4, "Stair", 8, "Channel");

// Names of gadgets
static PRM_Name	       names[] =
{
    // Stair tab
    PRM_Name("number",		"Number of Stairs"),
    PRM_Name("height",		"Stair Height"),
    PRM_Name("offset",		"Offset"),
    PRM_Name("direction",	"Direction"),
    
    // Channel tab
    PRM_Name("channelname",     "Channel Name"),
    // RangeName
    // StartName,
    // EndName,
    // SampleRateName
    // ExtendLeftName
    // ExtendRightName
    // DefaultValueName
};

// Menus
static PRM_Name CHOPstairMenu[] = {
    PRM_Name("up",	"Up"),
    PRM_Name("down",	"Down"),
    PRM_Name(0),
};

static PRM_ChoiceList stairMenu((PRM_ChoiceListType)(PRM_CHOICELIST_EXCLUSIVE
				| PRM_CHOICELIST_REPLACE),CHOPstairMenu);

// Defaults
static PRM_Default nameDefault(0,"chan1");
static PRM_Range   stairRange(PRM_RANGE_RESTRICTED, 0, PRM_RANGE_UI, 10);

PRM_Template
CHOP_Stair::myTemplateList[] =
{
    PRM_Template(PRM_SWITCHER,	3, &PRMswitcherName, switcher),

    // page 1
    PRM_Template(PRM_INT_J,     1, &names[0], PRMoneDefaults, 0, &stairRange),
    PRM_Template(PRM_FLT_J,     1, &names[1], PRMoneDefaults, 0,
	    	 &CHOP_DefaultValueRange),
    PRM_Template(PRM_FLT_J,     1, &names[2], PRMzeroDefaults, 0,
	    	 &CHOP_DefaultValueRange),
    PRM_Template(PRM_ORD,       1, &names[3], PRMzeroDefaults, &stairMenu),

    // page 2
    PRM_Template(PRM_STRING,    1, &names[4], &nameDefault,0),
    PRM_Template(PRM_ORD,	1, &CHOP_RangeName, PRMtwoDefaults,
				   &CHOP_RangeMenu),
    PRM_Template(PRM_FLT,       1, &CHOP_StartName, &CHOP_StartDefault,0,
		 &CHOP_FrameRange),
    PRM_Template(PRM_FLT,       1, &CHOP_EndName, &CHOP_EndDefault, 0,
		 &CHOP_FrameRange),
    PRM_Template(PRM_FLT,       1, &CHOP_SampleRateName,
		 &CHOP_SampleRateDefault,0,&CHOP_SampleRateRange),
    PRM_Template(PRM_ORD,       1, &CHOP_ExtendLeftName,0, &CHOP_ExtendMenu),
    PRM_Template(PRM_ORD,       1, &CHOP_ExtendRightName,0,&CHOP_ExtendMenu),
    PRM_Template(PRM_FLT,	1, &CHOP_DefaultValueName, PRMzeroDefaults,0,
		 &CHOP_DefaultValueRange),

    PRM_Template(),
};
OP_TemplatePair CHOP_Stair::myTemplatePair(
    CHOP_Stair::myTemplateList, &CHOP_Node::myTemplatePair);



enum
{
    VAR_C		= 200,
    VAR_NC		= 201
};

CH_LocalVariable
CHOP_Stair::myVariableList[] = {
    { "C",		VAR_C, 0 },
    { "NC",		VAR_NC, 0 },
    { 0, 0, 0 }
};
OP_VariablePair CHOP_Stair::myVariablePair(
    CHOP_Stair::myVariableList, &CHOP_Node::myVariablePair);

bool
CHOP_Stair::evalVariableValue(fpreal &v, int index, int thread)
{
    switch( index )
    {
	case VAR_C:
	    v = (fpreal) my_C;
	    return true;

	case VAR_NC:
	    v = (fpreal) my_NC;
	    return true;
    }

    return CHOP_Node::evalVariableValue(v, index, thread);
}

OP_Node *
CHOP_Stair::myConstructor(OP_Network		*net,
			     const char		*name,
			     OP_Operator	*op)
{
    return new CHOP_Stair(net, name, op);
}

CHOP_Stair::CHOP_Stair( OP_Network	*net,
			  const char	*name,
			  OP_Operator	*op)
	   : CHOP_Node(net, name, op),
	     myExpandArray()
{
    myParmBase = getParmList()->getParmIndex( names[0].getToken() );
    my_C = 0;
    my_NC = 0;
}

CHOP_Stair::~CHOP_Stair()
{
}

bool
CHOP_Stair::updateParmsFlags()
{
    bool    changes = CHOP_Node::updateParmsFlags();

    bool use_startend = (RANGE(0.0) == RANGE_USER_ENTERED);
    changes |= enableParm("start", use_startend);
    changes |= enableParm("end", use_startend);

    if(LEXTEND() == CL_TRACK_DEFAULT || REXTEND() == CL_TRACK_DEFAULT)
	changes |= enableParm(CHOP_DefaultValueName.getToken(),1);
    else
	changes |= enableParm(CHOP_DefaultValueName.getToken(),0);

    return changes;
}


#define START_HANDLE	1
#define END_HANDLE	2
#define OFFSET_HANDLE	3

void
CHOP_Stair::cookMyHandles(OP_Context &context)
{
    CHOP_Handle         *handle;
    CHOP_HandleLook      hlook;
    fpreal		 start, end;
    fpreal		 xoffset, yoffset;

    destroyHandles();

    // Grab the values for our handles
    getParmIntervalInSamples(context.getTime(), start, end);
    xoffset = (end - start) * 0.75 + start;
    yoffset = OFFSET(context.getTime());

    if (RANGE(context.getTime()) == RANGE_USER_ENTERED)
    {
	// If the parameter has a channel, use a guide rather than a handle.
	hlook = myChannels->getChannel("start") ? HANDLE_GUIDE : HANDLE_BOX;

	// This creates a new handle for the start of the clip, and appends it
	// to the list of handles. It will have the label 'start' to the
	// bottom-right of the handle, and appear in bar view mode as well. It
	// moves horizontally along the frame axis.
	handle = new CHOP_Handle(this, "start", START_HANDLE, start, 0.0,
				 hlook, HANDLE_HORIZONTAL, (HANDLE_BAR |
				 HANDLE_LABEL_RIGHT | HANDLE_LABEL_BOTTOM));
	myHandles.append(handle);

	// The end of the clip also has a handle for adjusting the clip. Other
	// than the position, it is similar to the start handle.
	hlook = myChannels->getChannel("end") ? HANDLE_GUIDE : HANDLE_BOX;
	handle = new CHOP_Handle(this, "end", END_HANDLE, end, 0.0,
				 hlook, HANDLE_HORIZONTAL, (HANDLE_BAR |
				 HANDLE_LABEL_RIGHT | HANDLE_LABEL_BOTTOM |
				 HANDLE_WIDTH_END));
	myHandles.append(handle);
    }

    // This is a horizontal handle which moves along the value axis to adjust
    // the height of a stair. This handle only appears in graph mode.
    hlook = myChannels->getChannel("offset") ? HANDLE_GUIDE : HANDLE_BOX;
    handle = new CHOP_Handle(this, "offset", OFFSET_HANDLE, xoffset, yoffset,
	    		     hlook, HANDLE_VERTICAL, 
			     (HANDLE_LABEL_RIGHT | HANDLE_LABEL_BOTTOM));
    myHandles.append(handle);
}

fpreal
CHOP_Stair::handleChanged(CHOP_Handle *handle, CHOP_HandleData *hdata)
{
    fpreal               result = 0.0;
    OP_Network          *net = 0;
    fpreal		 offset;

    if (hdata->shift)
    {
	setCurrent(1);
	if (net = getParent())
	    net->pickRequest(this, 0);
    }

    switch(handle->getId())
    {
	case START_HANDLE:
	    offset = CL_RINT(hdata->xoffset);
	    if (!hdata->shift)
		SET_START(myHandleCookTime, offset);
	    result = toUnit(offset, hdata->unit);
	    break;

        case END_HANDLE:
	    offset = CL_RINT(hdata->xoffset);
	    if (!hdata->shift)
		SET_END(myHandleCookTime, offset);
	    result = toUnit(offset, hdata->unit, 1);
	    break;

	case OFFSET_HANDLE:
	    if (!hdata->shift)
		SET_OFFSET(myHandleCookTime, hdata->yoffset);
	    result = hdata->yoffset;
	    break;
    }

    return result;
}


fpreal
CHOP_Stair::shiftStart(fpreal new_offset, fpreal t)
{
    SET_START(t, new_offset);
    return new_offset;
}

OP_ERROR
CHOP_Stair::cookMyChop(OP_Context &context)
{
    CL_Track		*track;
    fpreal		 samplerate;
    fpreal		 start, end;
    int			 left, right;
    fpreal		*data;
    UT_String		 name;
    fpreal		 value;
    fpreal		 defvalue;
    int			 nchan;
    fpreal		 stepheight;
    fpreal		 index, idxstep;
    int			 i;

    destroyClip();
    samplerate = RATE(context.getTime());
    if(SYSequalZero(samplerate))
    {
	addError(CHOP_ERROR_ZERO_SAMPLE_RATE);
	return error();
    }
    myClip->setSampleRate(samplerate);

    // Read non-expression parms
    
    CHAN_NAME(name, context.getTime());
    nchan = myExpandArray.setPattern(name);

    my_NC = nchan;
    
    getParmIntervalInSamples(context.getTime(), start, end);

    left  = LEXTEND();
    right = REXTEND();
    
    myClip->setTrackLength((int)(end-start+1));
    myClip->setStart(start);

    // Create the tracks
    
    for (my_C=0; my_C < nchan; my_C++)
    {
	defvalue   = DEFAULT(context.getTime());
	value      = OFFSET(context.getTime());	// reread for local vars
	idxstep    = (end-start+1) / (NUMBER(context.getTime()) + 1);
	stepheight = HEIGHT(context.getTime());
	if (DIRECTION() == 1)
	    stepheight *= -1;

	track = myClip->addTrack(myExpandArray(my_C));

	track->setLeft((CL_TrackOutside)left);
	track->setRight((CL_TrackOutside)right);
	if(left == CL_TRACK_DEFAULT || right == CL_TRACK_DEFAULT)
	    track->setDefault(defvalue);

	data = track->getData();
	track->constant(0);
	index = idxstep;
	
	for(i=0; i < (end-start+1); i++)
	{
	    if (i && i > index)
	    {
		value += stepheight;
		index += idxstep;
	    }

	    data[i] = value;
	}
    }

    my_C = 0;

    return error();
}

//----------------------------------------------------------------------------

void
newChopOperator(OP_OperatorTable *table)
    {
    table->addOperator(
	new OP_Operator("hdk_stair",                 // Internal name
			"HDK Stair",                 // UI name
			 CHOP_Stair::myConstructor,  // CHOP constructor
			&CHOP_Stair::myTemplatePair, // Parameters
			 0,                          // Min # of inputs
			 0,                          // Max # of inputs
			&CHOP_Stair::myVariablePair, // Local variables
			 OP_FLAG_GENERATOR)          // generator/source
    );
}
