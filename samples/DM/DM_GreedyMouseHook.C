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

#include <DM/DM_EventTable.h>
#include <DM/DM_MouseHook.h>
#include <DM/DM_VPortAgent.h>

namespace HDK_Sample {


// This sample shows the bounding box of all objects in the scene.

// A greedy mouse event hook that simply consumes all mouse events, doing
// nothing with them.
//
// DM_GreedyMouseEventHook objects are created by DM_GreedyMouseHook.
class DM_GreedyMouseEventHook : public DM_MouseEventHook
{
public:
    DM_GreedyMouseEventHook(DM_VPortAgent &vport)
	: DM_MouseEventHook(vport, DM_VIEWPORT_ALL_3D)
	{}
    virtual ~DM_GreedyMouseEventHook() { }
    
    virtual bool	handleMouseEvent(const DM_MouseHookData &hook_data,
					 UI_Event *event)
	{
	    return true;
	}
    virtual bool	handleMouseWheelEvent(const DM_MouseHookData &hook_data,
					      UI_Event *event)
	{
	    return true;
	}
    virtual bool	handleDoubleClickEvent(
					    const DM_MouseHookData &hook_data,
					    UI_Event *event)
	{
	    return true;
	}

    virtual bool	allowRMBMenu(const DM_MouseHookData &hook_data,
				     UI_Event *event)
	{
	    return false;
	}

private:
};

// DM_GreedyMouseHook is a factory for DM_GreedyMouseEventHook objects.
class DM_GreedyMouseHook : public DM_MouseHook
{
public:
    DM_GreedyMouseHook() : DM_MouseHook("Greedy", 0)
	{}

    virtual DM_MouseEventHook *newEventHook(DM_VPortAgent &vport)
	{
	    return new DM_GreedyMouseEventHook(vport);
	}

    virtual void		retireEventHook(DM_VPortAgent &vport,
						DM_MouseEventHook *hook)
	{
	    // Shared hooks might deference a ref count, but this example just
	    // creates one hook per viewport.
	    delete hook;
	}
};

} // END HDK_Sample namespace

using namespace HDK_Sample;


void
DMnewEventHook(DM_EventTable *table)
{
    table->registerMouseHook(new DM_GreedyMouseHook);
}
