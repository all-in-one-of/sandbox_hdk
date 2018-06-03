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
 * Custom selector for the BrushHairLen SOP.
 */

#include <UT/UT_DSOVersion.h>
#include <SOP/SOP_BrushBase.h>
#include <BM/BM_ResourceManager.h>
#include "MSS_BrushHairLenSelector.h"

using namespace HDK_Sample;

void
newSelector(BM_ResourceManager * /*m*/)
{
    PI_SelectorTemplate		*sel;
    OP_Operator			*op;
    
    // We create a PI_SelectorTemplate here to represent our new selector
    // As we want to use this with our SOP, and as the selector .dso
    // loads after the sop .dso, we also bind the selector here.

    // Create & register the selector.
    sel = new PI_SelectorTemplate("brushhairlenselect", // Internal name
				  "Brush Hair Selector", // English name
				  "Sop");		// Where it works.
    sel->constructor((void *) &MSS_BrushHairLenSelector::ourConstructor);
    sel->data(OP3DthePrimSelTypes);
    PIgetResourceManager()->registerSelector(sel);

    // Bind the selector.
    op = OP_Network::getOperatorTable(SOP_TABLE_NAME)->getOperator("proto_brushhairlen");
    if (!op)
    {
	UT_ASSERT(!"Could not find required op!");
	return;
    }

    PIgetResourceManager()->bindSelector(
	    op,				// OP_Operator to bind to.
	    "brushhairlenselect",	// Internal name of selector.
	    "Brush Hair Selector",	// English name of selector.
	    "Select the area to grow hair from.",	// Prompt.
	    "group",			// Parameter to write group to.
	    0,				// Input number to wire up.
	    1,				// 1 means this input is required.
	    "0x000000ff",		// Prim/point mask selection.
	    0,				// Can the user drag the object?
	    0,				// Optional menu.
	    0,				// Must * be used to select all?
	    0,				// Extra info string passed to selector
	    false);			// true if this from a ScriptOp.
}

MSS_BrushHairLenSelector::MSS_BrushHairLenSelector(OP3D_View &viewer, 
				     PI_SelectorTemplate &templ)
    : MSS_ReusableSelector(viewer, templ, "proto_brushhairlen", "group", 
			    0, 		// No group type field.
			    true)	// We want to allow blank group names.
{
    // Nothing needed.
}

MSS_BrushHairLenSelector::~MSS_BrushHairLenSelector()
{
    // Nothing needed.
}

BM_InputSelector *
MSS_BrushHairLenSelector::ourConstructor(BM_View &viewer,
				 PI_SelectorTemplate &templ)
{
    return new MSS_BrushHairLenSelector((OP3D_View &)viewer, templ);
}

const char *
MSS_BrushHairLenSelector::className() const
{
    return "MSS_BrushHairLenSelector";
}
