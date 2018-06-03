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
 * This code is for creating the state to go with this op.
*/

#include "MSS_CustomBrushState.h"

#include <DM/DM_Defines.h>
#include <DM/DM_ViewportType.h>
#include <GR/GR_DisplayOption.h>
#include <GU/GU_PrimCircle.h>
#include <MSS/MSS_SingleOpState.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Parm.h>
#include <RE/RE_Render.h>
#include <SOP/SOP_Node.h>
#include <UT/UT_DSOVersion.h>

#define MSS_CLICK_BUTTONS (DM_PRIMARY_BUTTON|DM_SECONDARY_BUTTON)

using namespace HDK_Sample;

// register the state
void
newModelState(BM_ResourceManager *m)
{
    m->registerState(
	new PI_StateTemplate("proto_custombrush", // state name
			     "Custom Brush", // English name
			     "SOP_proto_custombrush", // icon name
			     (void *)MSS_CustomBrushState::ourConstructor,
			     MSS_CustomBrushState::ourTemplateList,
			     PI_VIEWER_SCENE,
			     PI_NETMASK_SOP, // marks this as a SOP state
			     0));
}

// our state has no parameters
PRM_Template *
MSS_CustomBrushState::ourTemplateList = 0;

BM_State *
MSS_CustomBrushState::ourConstructor(BM_View &view, PI_StateTemplate &templ,
			           BM_SceneManager *scene)
{
    return new MSS_CustomBrushState((JEDI_View &)view, templ, scene);
}

MSS_CustomBrushState::MSS_CustomBrushState(
    JEDI_View &view,
    PI_StateTemplate &templ,
    BM_SceneManager *scene,
    const char *cursor)
    : MSS_SingleOpState(view, templ, scene, cursor),
      myBrushHandle((DM_SceneManager &)workbench(), "MSS_CustomBrushState")
{
    myIsBrushVisible	= false;
    myResizingCursor	= false;

    // create brush geometry
    GU_PrimCircleParms	cparms;
    cparms.gdp = &myBrushCursor;
    cparms.order = 3; // quadratic
    cparms.imperfect = 0; // rational
    cparms.xform.identity();
#if defined(HOUDINI_11)
    GU_PrimCircle::build(cparms, GEOPRIMBEZCURVE); // Bezier
#else
    GU_PrimCircle::build(cparms, GEO_PRIMBEZCURVE); // Bezier
#endif

    myBrushCursorXform.identity();
    myBrushRadius = 0.1;

    // only use this state in 3D viewports
    setViewportMask(DM_VIEWPORT_PERSPECTIVE);
}

MSS_CustomBrushState::~MSS_CustomBrushState()
{
    // Nothing needed.
}

const char *
MSS_CustomBrushState::className() const
{
    return "MSS_CustomBrushState";
}

int
MSS_CustomBrushState::enter(BM_SimpleState::BM_EntryType how)
{
    int result = MSS_SingleOpState::enter(how);
    // ask for handleMouseEvent to be called when the mouse moves or a
    // mouse button is clicked
    wantsLocates(1);
    addClickInterest(MSS_CLICK_BUTTONS);
    updatePrompt();

    // turn off the highlight so we can see the color we are painting
    OP_Node *op = getNode();
    if(op)
	op->setHighlight(0);
    return result;
}

void
MSS_CustomBrushState::exit()
{
    // cleanup
    wantsLocates(0);
    removeClickInterest(MSS_CLICK_BUTTONS);
    myIsBrushVisible = false;
    redrawScene();
    MSS_SingleOpState::exit();
}

void
MSS_CustomBrushState::resume(BM_SimpleState *state)
{
    MSS_SingleOpState::resume(state);
    wantsLocates(1);
    addClickInterest(MSS_CLICK_BUTTONS);
    updatePrompt();

    // turn off the highlight so we can see the color we are painting
    OP_Node *op = getNode();
    if(op)
	op->setHighlight(0);
}

void
MSS_CustomBrushState::interrupt(BM_SimpleState *state)
{
    wantsLocates(0);
    removeClickInterest(MSS_CLICK_BUTTONS);
    myIsBrushVisible = false;
    redrawScene();
    MSS_SingleOpState::interrupt(state);
}

int
MSS_CustomBrushState::handleMouseEvent(UI_Event *event)
{
    SOP_Node *sop = (SOP_Node *)getNode();
    if (!sop)
	return 1; // consumed but useless

    fpreal t = getTime();
    int x = event->state.values[X];
    int y = event->state.values[Y];

    if (event->reason == UI_VALUE_START &&
	    (event->state.altFlags & UI_ALT_KEY ||
	     event->state.altFlags & UI_SHIFT_KEY))
    {
	// prepare for resizing the brush
	myResizeCursorX = x;
	myResizeCursorY = y;
	myResizingCursor = true;
    }
    else if (myResizingCursor)
    {
	// scale the brush's radius
	fpreal dist = x - myLastCursorX +
		      y - myLastCursorY;

	myBrushRadius *= powf(1.01, dist);

	if (event->reason == UI_VALUE_CHANGED)
	    myResizingCursor = false;

	updateBrush(myResizeCursorX, myResizeCursorY);
    }
    else if (event->reason == UI_VALUE_LOCATED)
    {
	// re-position the brush
	updateBrush(x, y);
    }
    else
    {
	// Apply a stroke
	//
	// The set*() method calls below will automatically record the undo
	// actions. Since we do automatic matching of the sop node type via the
	// same name as the state, we're assuming here for simplicity that the
	// parameters exist.

	UT_Vector3 rayorig, dir;
	mapToWorld(x, y, dir, rayorig);

	bool begin = (event->reason == UI_VALUE_START ||
		      event->reason == UI_VALUE_PICKED);
	if(begin)
	    beginDistributedUndoBlock("Stroke", ANYLEVEL);

	sop->setFloat("origin", 0, t, rayorig.x());
	sop->setFloat("origin", 1, t, rayorig.y());
	sop->setFloat("origin", 2, t, rayorig.z());

	sop->setFloat("direction", 0, t, dir.x());
	sop->setFloat("direction", 1, t, dir.y());
	sop->setFloat("direction", 2, t, dir.z());

	sop->setFloat("radius", 0, t, myBrushRadius);

	{
	    UT_String str = (event->state.values[W] == DM_SECONDARY_BUTTON)
				?  "erase" : "paint";
	    sop->setString(str, CH_STRING_LITERAL, "operation", 0, t);
	}

	OP_Context context(t);

	bool set_op = false;
	if (begin)
	{
	    // indicate the begin of a stroke
	    set_op = true;
	    sop->setString(UT_String("begin"), CH_STRING_LITERAL,"event",0,t);
	}

	// indicate a stroke is active
	bool active = (event->reason == UI_VALUE_ACTIVE ||
		       event->reason == UI_VALUE_PICKED);
	if (active)
	{
	    if(set_op)
	    {
		// trigger a cook of the CustomBrush SOP so it can cook with
		// the current stroke values
		sop->getCookedGeo(context);
	    }

	    set_op = true;
	    sop->setString(UT_String("active"), CH_STRING_LITERAL, "event",0,t);
	}
	// If the brush event is an end, we need to close the undo block.
	if (event->reason == UI_VALUE_CHANGED ||
	    event->reason == UI_VALUE_PICKED)
	{
	    if(set_op)
	    {
		// trigger a cook of the CustomBrush SOP so it can cook
		// with the current stroke values
		sop->getCookedGeo(context);
	    }

	    // now change the stroke parameter to indicate a no-op.
	    sop->setString(UT_String("end"), CH_STRING_LITERAL, "event", 0, t);

	    // trigger a cook of the CustomBrush SOP so it can cook with
	    // the end stroke values
	    sop->getCookedGeo(context);

	    // now change the stroke parameter to indicate a no-op.
	    sop->setString(UT_String("nop"), CH_STRING_LITERAL, "event", 0, t);

	    endDistributedUndoBlock();
	}

	updateBrush(x, y);
    }

    myLastCursorX = x;
    myLastCursorY = y;

    return 1;
}

void
MSS_CustomBrushState::doRender(RE_Render *r, int, int, int ghost)
{
    if (!isPreempted() && myIsBrushVisible)
    {
	UT_Color	 clr;

	r->pushMatrix();
	r->multiplyMatrix(myBrushCursorXform);

	if(ghost)
	{
	    // color for obstructed parts of the brush
	    clr = UT_Color(UT_RGB, 0.625,0.4,0.375);
	}
	else
	{
	    // color for unobstructed parts of the brush
	    clr = UT_Color(UT_RGB, 1, 0.1, 0);
	}

	myBrushHandle.renderWire(*r, 0, 0, 0, clr, &myBrushCursor);

	r->popMatrix();
    }
}

void
MSS_CustomBrushState::updatePrompt()
{
    showPrompt("LMB to apply stroke.  MMB to erase.  Shift-LMB to adjust radius.");
}

void
MSS_CustomBrushState::updateBrush(int x, int y)
{
    // get cameraspace to worldspace transform
    getViewportItransform(myBrushCursorXform);

    // determine the direction the camera a facing
    UT_Vector3 forward = rowVecMult3(UT_Vector3(0, 0, -1), myBrushCursorXform);

    // position the brush under the pointer and one unit away from the camera
    UT_Vector3 rayorig, dir;
    mapToWorld(x, y, dir, rayorig);
    UT_Vector3 delta(1.0 / dot(dir, forward) * dir);
    myBrushCursorXform.translate(delta.x(), delta.y(), delta.z());

    // scale the brush
    myBrushCursorXform.prescale(myBrushRadius, myBrushRadius, 1);

    // ensure the brush is visible
    myIsBrushVisible = true;
    redrawScene();
}
