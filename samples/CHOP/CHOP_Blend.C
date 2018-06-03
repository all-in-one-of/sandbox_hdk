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
 *	Blends inputs 2,3,4... using the weights in input 1.
 */

#include <SYS/SYS_Math.h>
#include <UT/UT_DSOVersion.h>
#include <OP/OP_OperatorTable.h>

#include <CH/CH_LocalVariable.h>
#include <PRM/PRM_Include.h>

#include <UT/UT_Interrupt.h>

#include "CHOP_Blend.h"

using namespace HDK_Sample;

CHOP_SWITCHER(2,"Blend");

static PRM_Name methodItems[] = {
    PRM_Name("prop",	"Proportional"),
    PRM_Name("diff",	"Difference"),
    PRM_Name(0),
};

static PRM_ChoiceList methodMenu((PRM_ChoiceListType)
				 (PRM_CHOICELIST_EXCLUSIVE | 
				  PRM_CHOICELIST_REPLACE),
				 methodItems);

static PRM_Name names[] = {
    PRM_Name("method",		"Method"),
    PRM_Name("firstweight",	"Omit First Weight Channel"),
};

PRM_Template 
CHOP_Blend::myTemplateList[] =
{
    PRM_Template(PRM_SWITCHER,	2, &PRMswitcherName, switcher),
    PRM_Template(PRM_ORD,	1, &names[0], PRMoneDefaults,&methodMenu),
    PRM_Template(PRM_TOGGLE,	1, &names[1], PRMzeroDefaults),
    
    PRM_Template(),
};
OP_TemplatePair CHOP_Blend::myTemplatePair(
    CHOP_Blend::myTemplateList, &CHOP_Node::myTemplatePair);

bool
CHOP_Blend::updateParmsFlags()
{
    bool changes = CHOP_Node::updateParmsFlags();

    // can only omit the first weight when differencing.
    changes |= enableParm("firstweight", GETDIFFERENCE());

    return changes;
}



CH_LocalVariable
CHOP_Blend::myVariableList[] = {
    { 0, 0, 0 }
};
OP_VariablePair CHOP_Blend::myVariablePair(
    CHOP_Blend::myVariableList, &CHOP_Node::myVariablePair);


OP_Node *
CHOP_Blend::myConstructor(OP_Network	*net,
			  const char	*name,
			  OP_Operator	*op)
{
    return new CHOP_Blend(net, name, op);
}

//----------------------------------------------------------------------------


CHOP_Blend::CHOP_Blend( OP_Network	*net,
			const char	*name,
			OP_Operator	*op)
	   : CHOP_Node(net, name, op)
{
      myParmBase = getParmList()->getParmIndex( names[0].getToken() );
}

CHOP_Blend::~CHOP_Blend()
{
}

const CL_Clip *
CHOP_Blend::getCacheInputClip(int j)
{
    // Convenience method to return the clip cached by the method
    // CHOP_Blend::findInputClips();
    return (j>=0 && j<myInputClip.entries()) ? 
	myInputClip(j) : 0;
}

OP_ERROR
CHOP_Blend::cookMyChop(OP_Context &context)
{
    const CL_Clip	*blendclip;
    const CL_Clip	*clip;
    const CL_Track	*track, *blend;
    int			 num_motion_tracks;
    int			 num_clips;
    fpreal		 weight, *data, *total;
    int			 i,j,k,samples;
    fpreal		 time;
    fpreal		 val;
    int			 difference;
    int			 fweight;
    UT_Interrupt 	*boss = UTgetInterrupt();
    short int		 percent = -1;
    int			 stopped = 0;
    fpreal		 adjust[2] = {1.0, -1.0};
    const fpreal	*src, *w;

    difference = GETDIFFERENCE();
    if(difference)
	fweight = FIRST_WEIGHT();
    else
	fweight = 0;

    // This copies the attributes of the clip without copying the tracks
    // (length, start, sample rate, rotation order).
    blendclip = copyInputAttributes(context);
    if(!blendclip)
	return error();

    // Determine which clips are being blended together (the first is a control
    // clip containing weights)
    num_clips = findInputClips(context, blendclip);

    // Determine the tracks that are being blended together.
    num_motion_tracks = findFirstAvailableTracks(context);

    samples = myClip->getTrackLength();

    // Create a working array for blending containing the sum of all the blend
    // weights for a given sample.
    myTotalArray.setCapacity(samples);
    myTotalArray.entries(samples);
    total = myTotalArray.array();

    if(boss->opStart("Blending Channels"))
    {
	for(i=0; i<num_motion_tracks; i++)
	{
	    // layer the motion tracks, by looping through the clips for
	    // each track

	    // Zero out the track's data and grab a pointer to it for processing
	    myClip->getTrack(i)->constant(0);
	    data = myClip->getTrack(i)->getData();

	    // Start off with a constant track.
	    myTotalArray.constant(difference ? 1.0 : 0.0);

	    // When differencing, subtract the contributions of the weights from
	    // the initial weight of '1' for the first blend input.
	    // Otherwise, add all weights together.
	    for(j=difference?1:0; j<num_clips; j++)
	    {
		clip = getCacheInputClip(j+1);
		if(!clip)
		    continue;

		// Grab the control track for the blend track we're
		// processing. It contains an array of weights.
		if(difference && fweight)
		    blend = blendclip->getTrack(j-1);
		else
		    blend = blendclip->getTrack(j);
		track = clip->getTrack(i); 

		if(!track || !blend)
		    continue;

		// go through all samples and blend into the result.

		w = blend->getData();

		// Optimization for input clips that share the same frame range
		// as our output clip -- access the data directly.
		if (myClip->isSameRange(*clip))
		{
		    src = track->getData();
		    for(k=0; k<samples; k++)
		    {
			if (w[k])
			{
			    // Add the weighted sample into the output data.
			    data[k] += w[k]*src[k];

			    // add or subtract the weight from the total.
			    // If differencing is used, adjust[] = -1, else 1.
			    total[k] += w[k]*adjust[difference];
			}
		    }
		}
		else
		{
		    // Slightly slower; evaluate at each sample, but otherwise
		    // the same as the first case.
		    for(k=0; k<samples; k++)
		    {
			time = myClip->getTime(k + myClip->getStart());
			if (w[k])
			{
			    data[k] += w[k] * 
				clip->evaluateSingleTime(track, time);
			    total[k] += w[k]*adjust[difference];
			}
		    }
		}

		// Check for user interrupts.
		if(boss->opInterrupt())
		{
		    stopped = 1;
		    break;
		}
	    }

	    // Now normalize the results to 1.
	    if(!stopped)
	    {
		if(!difference)
		{
		    // when not difference, just divide the samples by the 
		    // total weight value for each sample.
		    j = myAvailableTracks(i);
		    if ( j!=-1 && getCacheInputClip(j))
		    {
			track = getCacheInputClip(j)->getTrack(i);
			blend = blendclip->getTrack(j);
			if(blend)
			    w = blend->getData();
			else
			    continue;
		    }
		    else
			track = 0;

		    for(k=0; k<samples; k++)
		    {
			if(!SYSequalZero(total[k], 0.001))
			{
			    // normalize so all weights add to 1
			    data[k] /= total[k];
			}
			else
			{
			    // hm... weights all zero. use 1st available input.

			    if(track && blend)
			    {
				time = myClip->getTime(k + myClip->getStart());
				val = clip->evaluateSingleTime(track,time);

				data[k] -= w[k]*val;
				total[k]-= w[k];

				weight  = 1.0 - total[k];
				total[k]+= weight;
				data[k] += weight*val;
			
				data[k] /= total[k];
			    }
			}
		    }
		}
		else
		{
		    // when differencing, add in the tracks from the first blend
		    // input using the remaining weight value (the second and
		    // subsequent blend clips subtract their weights from 1).
		    j = myAvailableTracks(i);
		    if (j != -1)
		    {
			clip = getCacheInputClip(j);
			track = clip ? clip->getTrack(i) : 0;

			if (track)
			{
			    if (myClip->isSameRange(*clip))
			    {
				const fpreal *src = track->getData();
				for(k=0; k<samples; k++)
				    data[k] += src[k] * total[k];
			    }
			    else
			    {
				for(k=0; k<samples; k++)
				{
				    time= myClip->getTime(k+myClip->getStart());
				    val = clip->evaluateSingleTime(track, time);
				    data[k] += val * total[k];
				}
			    }
			}
		    }
		    if(boss->opInterrupt(percent))
		    {
			stopped = 1;
			break;
		    }

		}
	    }
	    if(stopped)
		break;
	}
    }
    boss->opEnd();
    
    return error();
}

int
CHOP_Blend::findInputClips(OP_Context &context, const CL_Clip *blendclip)
{
    // The number of clips we use is the number of tracks in the control input.
    int num_clips = blendclip->getNumTracks();

    // When using differencing, you can omit the first channel's weight
    // assumed to be one always) and blend in other clips.
    if(GETDIFFERENCE() && FIRST_WEIGHT())
	num_clips ++;
    
    // more weights than clips? Ignore them.
    if (num_clips >= nInputs())
	num_clips = nInputs()-1;

    // marshall the blending clips into a list.
    myInputClip.setCapacity(num_clips+1);
    myInputClip.entries(num_clips+1);

    for (int i=0; i<=num_clips; i++)
	myInputClip(i) = inputClip(i, context);

    return num_clips;
}



int
CHOP_Blend::findFirstAvailableTracks(OP_Context &context)
{
    // This creates a union of all tracks from the blending clips, and creates
    // these tracks in the output clip.
    int num_motion_tracks = 0;
    for (int i=1; i<nInputs(); i++)
    {
	const CL_Clip *clip = inputClip(i, context);

	if (!clip)
	    continue;

	if (clip->getNumTracks() > num_motion_tracks)
	    num_motion_tracks = clip->getNumTracks();
    }

    
    // myAvailableTracks contains the index of the first blend input to contain
    // each track, normally 1, unless some blend clips have more tracks than
    // others.
    myAvailableTracks.setCapacity(num_motion_tracks);
    myAvailableTracks.entries(num_motion_tracks);

    for (int i=0; i<num_motion_tracks; i++)
    {
	const CL_Track *track = NULL;
	int j = 1;
	
	while (!track && j<nInputs())
	{
	    const CL_Clip *clip = inputClip(j, context);
	    if (!clip)
	    {
		j++;
		continue;
	    }
	    
	    track = clip->getTrack(i);
	    if (!track)
		j++;
	}

	myAvailableTracks(i) = track ? j : -1;
    }

    // Create new output tracks for each input track that will be blended.
    // Copy everything except the track's data.
    for (int i=0; i<num_motion_tracks; i++)
    {
	int j = myAvailableTracks(i);

	if (j == -1)
	    continue;
	
	const CL_Track *track = inputClip(j, context)->getTrack(i);
	if (track)
	    myClip->dupTrackInfo(track);
    }


    return num_motion_tracks;
}

// install the chop.
void newChopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator("hdk_blend", // node name
				       "HDK Blend", // pretty name
				       CHOP_Blend::myConstructor,
				       &CHOP_Blend::myTemplatePair,
				       2,       // min inputs
				       9999,       // max inputs 
				       &CHOP_Blend::myVariablePair));
}
