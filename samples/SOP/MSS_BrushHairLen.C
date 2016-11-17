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
 * This code is for creating the state to go with this op.
*/

#include <UT/UT_DSOVersion.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPoly.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_ChoiceList.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>

#include <SOP/SOP_Guide.h>
#include <BM/BM_ResourceManager.h>
#include <SOP/SOP_Node.h>
#include <SOP/SOP_BrushBase.h>
#include <MSS/MSS_BrushBaseState.h>
#include <MSS/MSS_KeyBindings.h>
#include "MSS_BrushHairLen.h"

#define PRM_MENU_CHOICES	(PRM_ChoiceListType)(PRM_CHOICELIST_EXCLUSIVE |\
						     PRM_CHOICELIST_REPLACE)

// Prototype for function to create our state.
PI_StateTemplate *sopBrushHairLenState();

using namespace HDK_Sample;

// Define the new state...
void
newModelState(BM_ResourceManager *m)
{
    m->registerState(sopBrushHairLenState());
}

enum MSS_PaintOp {
    MSS_PAINT,
    MSS_EYEDROP,
    MSS_SMOOTH,
    MSS_CALLBACK,
    MSS_ERASE
};

static PRM_Name		mssOpMenuNames[] = {
	PRM_Name("paint",	"Paint"),
	PRM_Name("eyedrop",	"Eye Dropper"),
	PRM_Name("smooth",	"Smooth"),
	PRM_Name("callback",	"Callback"),
	PRM_Name("erase",	"Erase Changes"),
	PRM_Name(0)
};

#define PRM_MENU_CHOICES	(PRM_ChoiceListType)(PRM_CHOICELIST_EXCLUSIVE |\
						     PRM_CHOICELIST_REPLACE)

PRM_ChoiceList	MSS_BrushHairLen::theLMBMenu(PRM_MENU_CHOICES, mssOpMenuNames);
PRM_ChoiceList	MSS_BrushHairLen::theMMBMenu(PRM_MENU_CHOICES, mssOpMenuNames);

PRM_Template
MSS_BrushHairLen::ourTemplateList[] = 
{
    PRM_Template(PRM_ORD, 1, &PRMlmbName, PRMzeroDefaults, &theLMBMenu),
    PRM_Template(PRM_ORD, 1, &PRMmmbName, PRMzeroDefaults, &theMMBMenu),
    PRM_Template(),
};

BM_State *
MSS_BrushHairLen::ourConstructor(BM_View &view, PI_StateTemplate &templ,
			        BM_SceneManager *scene)
{
    return new MSS_BrushHairLen((JEDI_View &)view, templ, scene);
}

MSS_BrushHairLen::MSS_BrushHairLen(JEDI_View &view, PI_StateTemplate &templ,
			         BM_SceneManager *scene, const char *cursor)
    : MSS_BrushBaseState(view, templ, scene, cursor)
{
    // Nothing needed.
}

MSS_BrushHairLen::~MSS_BrushHairLen()
{
    // Nothing needed.
}

const char *
MSS_BrushHairLen::className() const
{
    return "MSS_BrushHairLen";
}

void
MSS_BrushHairLen::getUIFileName(UT_String &uifilename) const
{
    uifilename = "MSS_BrushHairLen.ui";
}

int
MSS_BrushHairLen::handleKeyTypeEvent(UI_Event *event, BM_Viewport &viewport)
{
    int	 key = (int)*event->value;

    if (!KEYCMP(MSS_KEY_BRUSH_P_MAIN))
    {
	getPrimaryVal() = int(MSS_PAINT);
	getPrimaryVal().changed(0); // we want it too to set the state PRM
	return 1;
    }
    if (!KEYCMP(MSS_KEY_BRUSH_S_MAIN))
    {
	getSecondaryVal() = int(MSS_PAINT);
	getSecondaryVal().changed(0); // we want it too to set the state PRM
	return 1;
    }
    
    if (!KEYCMP(MSS_KEY_PAINT_P_EYEDROP))
    {
	getPrimaryVal() = int(MSS_EYEDROP);
	getPrimaryVal().changed(0); // we want it too to set the state PRM
	return 1;
    }
    if (!KEYCMP(MSS_KEY_PAINT_S_EYEDROP))
    {
	getSecondaryVal() = int(MSS_EYEDROP);
	getSecondaryVal().changed(0); // we want it too to set the state PRM
	return 1;
    }

    if (!KEYCMP(MSS_KEY_BRUSH_P_SMOOTH))
    {
	getPrimaryVal() = int(MSS_SMOOTH);
	getPrimaryVal().changed(0); // we want it too to set the state PRM
	return 1;
    }
    if (!KEYCMP(MSS_KEY_BRUSH_S_SMOOTH))
    {
	getSecondaryVal() = int(MSS_SMOOTH);
	getSecondaryVal().changed(0); // we want it too to set the state PRM
	return 1;
    }
    
    if (!KEYCMP("h.pane.gview.state.sop.brush.p_callback"))
    {
	getPrimaryVal() = int(MSS_CALLBACK);
	getPrimaryVal().changed(0); // we want it too to set the state PRM
	return 1;
    }
    if (!KEYCMP("h.pane.gview.state.sop.brush.s_callback"))
    {
	getSecondaryVal() = int(MSS_CALLBACK);
	getSecondaryVal().changed(0); // we want it too to set the state PRM
	return 1;
    }
    
    if (!KEYCMP(MSS_KEY_BRUSH_P_ERASE))
    {
	getPrimaryVal() = int(MSS_ERASE);
	getPrimaryVal().changed(0); // we want it too to set the state PRM
	return 1;
    }
    if (!KEYCMP(MSS_KEY_BRUSH_S_ERASE))
    {
	getSecondaryVal() = int(MSS_ERASE);
	getSecondaryVal().changed(0); // we want it too to set the state PRM
	return 1;
    }

    return MSS_BrushBaseState::handleKeyTypeEvent(event, viewport);
}

SOP_BrushOp
MSS_BrushHairLen::menuToBrushOp(const UI_Value &v) const
{
    switch ((MSS_PaintOp)(int)v)
    {
	case MSS_PAINT:		return SOP_BRUSHOP_PAINT;
	case MSS_EYEDROP:	return SOP_BRUSHOP_EYEDROP;
	case MSS_SMOOTH:	return SOP_BRUSHOP_SMOOTHATTRIB;
	case MSS_CALLBACK:	return SOP_BRUSHOP_CALLBACK;
	case MSS_ERASE:
	default:		return SOP_BRUSHOP_ERASE;
    }
}


PI_StateTemplate *
sopBrushHairLenState()
{
    return new PI_StateTemplate("proto_brushhairlen",  // State Name.
	    "Brush Hair Len", // English name
	    "SOP_proto_brushhairlen", // Icon name
	    (void *)MSS_BrushHairLen::ourConstructor,
	    MSS_BrushHairLen::ourTemplateList, PI_VIEWER_SCENE,
	    PI_NETMASK_SOP,		// Marks this as a SOP state.
	    0);
}
