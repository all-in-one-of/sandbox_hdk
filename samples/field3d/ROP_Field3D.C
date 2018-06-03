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
 */

#include <UT/UT_DSOVersion.h>
#include <CH/CH_LocalVariable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_Director.h>
#include <SOP/SOP_Node.h>
#include <ROP/ROP_Error.h>
#include <ROP/ROP_Templates.h>
#include <UT/UT_IOTable.h>
#include "ROP_Field3D.h"

#include "f3d_io.h"
#include <Field3D/InitIO.h>

using namespace	HDK_Sample;

static PRM_Name		sopPathName("soppath",	"SOP Path");
static PRM_Name		 theFileName("file", "Output File");
static PRM_Default	 theFileDefault(0, "$HIP/$F.f3d");
static PRM_Name		alfprogressName("alfprogress", "Alfred Style Progress");

static PRM_Name		theGridTypes[] =
{
    PRM_Name("dense", "Dense"),
    PRM_Name("sparse", "Sparse"),
    PRM_Name()
};
static PRM_ChoiceList theGridTypeMenu(PRM_CHOICELIST_SINGLE, theGridTypes);
static PRM_Name gridtypeName("gridtype", "Grid Type");

static PRM_Name		theBitDepths[] =
{
    PRM_Name("auto", "Auto Detect"),
    PRM_Name("fp16", "16bit Float"),
    PRM_Name("fp32", "Float"),
    PRM_Name("fp64", "Double"),
    PRM_Name()
};
static PRM_ChoiceList theBitDepthMenu(PRM_CHOICELIST_SINGLE, theBitDepths);
static PRM_Name bitdepthName("bitdepth", "Bit Depth");

static PRM_Name collateName("collatevector", "Collate Vector Fields");

static PRM_Template	 f3dTemplates[] = {
    PRM_Template(PRM_STRING, PRM_TYPE_DYNAMIC_PATH, 1, &sopPathName,
				0, 0, 0, 0, &PRM_SpareData::sopPath),
    PRM_Template(PRM_FILE,	1, &theFileName, &theFileDefault,0,
				0, 0, &PRM_SpareData::fileChooserModeWrite),
    PRM_Template(PRM_TOGGLE, 1, &alfprogressName, PRMzeroDefaults),

    PRM_Template(PRM_ORD, 1, &gridtypeName, PRMoneDefaults,
			    &theGridTypeMenu),
    PRM_Template(PRM_ORD, 1, &bitdepthName, 0,
			    &theBitDepthMenu),
    PRM_Template(PRM_TOGGLE, 1, &collateName, PRMoneDefaults),
    PRM_Template()
			    
};

static PRM_Template *
getTemplates()
{
    static PRM_Template	*theTemplate = 0;

    if (theTemplate)
	return theTemplate;

    theTemplate = new PRM_Template[ROP_F3D_MAXPARMS+1];
    theTemplate[ROP_F3D_RENDER] = theRopTemplates[ROP_RENDER_TPLATE];
    theTemplate[ROP_F3D_RENDER_CTRL] = theRopTemplates[ROP_RENDERDIALOG_TPLATE];
    theTemplate[ROP_F3D_TRANGE] = theRopTemplates[ROP_TRANGE_TPLATE];
    theTemplate[ROP_F3D_FRANGE] = theRopTemplates[ROP_FRAMERANGE_TPLATE];
    theTemplate[ROP_F3D_TAKE] = theRopTemplates[ROP_TAKENAME_TPLATE];
    theTemplate[ROP_F3D_SOPPATH] = f3dTemplates[0];
    theTemplate[ROP_F3D_SOPOUTPUT] = f3dTemplates[1];
    theTemplate[ROP_F3D_GRIDTYPE] = f3dTemplates[3];
    theTemplate[ROP_F3D_BITDEPTH] = f3dTemplates[4];
    theTemplate[ROP_F3D_COLLATE] = f3dTemplates[5];
    theTemplate[ROP_F3D_INITSIM] = theRopTemplates[ROP_INITSIM_TPLATE];
    theTemplate[ROP_F3D_ALFPROGRESS] = f3dTemplates[2];
    theTemplate[ROP_F3D_TPRERENDER] = theRopTemplates[ROP_TPRERENDER_TPLATE];
    theTemplate[ROP_F3D_PRERENDER] = theRopTemplates[ROP_PRERENDER_TPLATE];
    theTemplate[ROP_F3D_LPRERENDER] = theRopTemplates[ROP_LPRERENDER_TPLATE];
    theTemplate[ROP_F3D_TPREFRAME] = theRopTemplates[ROP_TPREFRAME_TPLATE];
    theTemplate[ROP_F3D_PREFRAME] = theRopTemplates[ROP_PREFRAME_TPLATE];
    theTemplate[ROP_F3D_LPREFRAME] = theRopTemplates[ROP_LPREFRAME_TPLATE];
    theTemplate[ROP_F3D_TPOSTFRAME] = theRopTemplates[ROP_TPOSTFRAME_TPLATE];
    theTemplate[ROP_F3D_POSTFRAME] = theRopTemplates[ROP_POSTFRAME_TPLATE];
    theTemplate[ROP_F3D_LPOSTFRAME] = theRopTemplates[ROP_LPOSTFRAME_TPLATE];
    theTemplate[ROP_F3D_TPOSTRENDER] = theRopTemplates[ROP_TPOSTRENDER_TPLATE];
    theTemplate[ROP_F3D_POSTRENDER] = theRopTemplates[ROP_POSTRENDER_TPLATE];
    theTemplate[ROP_F3D_LPOSTRENDER] = theRopTemplates[ROP_LPOSTRENDER_TPLATE];
    theTemplate[ROP_F3D_MAXPARMS] = PRM_Template();

    UT_ASSERT(PRM_Template::countTemplates(theTemplate) == ROP_F3D_MAXPARMS);

    return theTemplate;
}

OP_TemplatePair *
ROP_Field3D::getTemplatePair()
{
    static OP_TemplatePair *ropPair = 0;
    if (!ropPair)
    {
	ropPair = new OP_TemplatePair(getTemplates());
    }
    return ropPair;
}

OP_VariablePair *
ROP_Field3D::getVariablePair()
{
    static OP_VariablePair *pair = 0;
    if (!pair)
	pair = new OP_VariablePair(ROP_Node::myVariableList);
    return pair;
}

OP_Node *
ROP_Field3D::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new ROP_Field3D(net, name, op);
}

ROP_Field3D::ROP_Field3D(OP_Network *net, const char *name, OP_Operator *entry)
	: ROP_Node(net, name, entry)
{
}


ROP_Field3D::~ROP_Field3D()
{
}

//------------------------------------------------------------------------------
// The startRender(), renderFrame(), and endRender() render methods are
// invoked by Houdini when the ROP runs.

int
ROP_Field3D::startRender(int /*nframes*/, fpreal tstart, fpreal tend)
{
    int			 rcode = 1;

    myEndTime = tend;
    myStartTime = tstart;

    if (INITSIM())
    {
        initSimulationOPs();
	OPgetDirector()->bumpSkipPlaybarBasedSimulationReset(1);
    }

    if (error() < UT_ERROR_ABORT)
    {
	if( !executePreRenderScript(tstart) )
	    return 0;
    }

    return rcode;
}

ROP_RENDER_CODE
ROP_Field3D::renderFrame(fpreal time, UT_Interrupt *)
{
    SOP_Node		*sop;
    UT_String		 soppath, savepath;

    if( !executePreFrameScript(time) )
	return ROP_ABORT_RENDER;

    // From here, establish the SOP that will be rendered, if it cannot
    // be found, return 0.
    // This is needed to be done here as the SOPPATH may be time
    // dependent (ie: OUT$F) or the perframe script may have
    // rewired the input to this node.

    sop = CAST_SOPNODE(getInput(0));
    if( sop )
    {
	// If we have an input, get the full path to that SOP.
	sop->getFullPath(soppath);
    }
    else
    {
	// Otherwise get the SOP Path from our parameter.
	SOPPATH(soppath, time);
    }

    if( !soppath.isstring() )
    {
	addError(ROP_MESSAGE, "Invalid SOP path");
	return ROP_ABORT_RENDER;
    }

    sop = getSOPNode(soppath, 1);
    if (!sop)
    {
	addError(ROP_COOK_ERROR, (const char *)soppath);
	return ROP_ABORT_RENDER;
    }
    OP_Context		context(time);
    GU_DetailHandle gdh;
    gdh = sop->getCookedGeoHandle(context);

    GU_DetailHandleAutoReadLock	 gdl(gdh);
    const GU_Detail		*gdp = gdl.getGdp();

    if (!gdp)
    {
	addError(ROP_COOK_ERROR, (const char *)soppath);
	return ROP_ABORT_RENDER;
    }

    OUTPUT(savepath, time);

    f3d_fileSave(gdp, (const char *) savepath,
		(F3D_BitDepth) BITDEPTH(time),
		(F3D_GridType) GRIDTYPE(time),
		COLLATE(time));

    if (ALFPROGRESS() && (myEndTime != myStartTime))
    {
	fpreal		fpercent = (time - myStartTime) / (myEndTime - myStartTime);
	int		percent = (int)SYSrint(fpercent * 100);
	percent = SYSclamp(percent, 0, 100);
	fprintf(stdout, "ALF_PROGRESS %d%%\n", percent);
	fflush(stdout);
    }
    
    if (error() < UT_ERROR_ABORT)
    {
	if( !executePostFrameScript(time) )
	    return ROP_ABORT_RENDER;
    }

    return ROP_CONTINUE_RENDER;
}

ROP_RENDER_CODE
ROP_Field3D::endRender()
{
    if (INITSIM())
	OPgetDirector()->bumpSkipPlaybarBasedSimulationReset(-1);

    if (error() < UT_ERROR_ABORT)
    {
	if( !executePostRenderScript(myEndTime) )
	    return ROP_ABORT_RENDER;
    }
    return ROP_CONTINUE_RENDER;
}

void
newDriverOperator(OP_OperatorTable *table)
{
    Field3D::initIO();

    table->addOperator(new OP_Operator("hdk_field3d",
					"Field 3D",
					ROP_Field3D::myConstructor,
					ROP_Field3D::getTemplatePair(),
					0,
					0,
					ROP_Field3D::getVariablePair(),
					OP_FLAG_GENERATOR));
}

