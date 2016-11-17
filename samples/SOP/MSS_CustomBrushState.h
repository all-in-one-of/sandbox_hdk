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

#ifndef __MSS_CustomBrushState_H__
#define __MSS_CustomBrushState_H__

#include <MSS/MSS_SingleOpState.h>
#include <DM/DM_Detail.h>
#include <GU/GU_Detail.h>

class JEDI_View;

namespace HDK_Sample {
class MSS_CustomBrushState : public MSS_SingleOpState
{
public:
	     MSS_CustomBrushState(JEDI_View &view, PI_StateTemplate &templ,
			        BM_SceneManager *scene,
			        const char *cursor = BM_DEFAULT_CURSOR);
    virtual ~MSS_CustomBrushState();

    /// used by DM to create our state
    static BM_State	*ourConstructor(BM_View &view, PI_StateTemplate &templ,
					BM_SceneManager *scene);

    /// parameters for this state
    static PRM_Template	*ourTemplateList;

    /// The name and type of this class:
    virtual const char	*className() const;

protected:
    /// called when the user enters the state
    virtual int		 enter(BM_SimpleState::BM_EntryType how);

    /// called when the user leaves the state
    virtual void	 exit();

    /// called when the user temporarily leaves the state (mouse leaves the
    /// viewport)
    virtual void	 interrupt(BM_SimpleState * = 0);

    /// called when the user returns to the state after leaving temporarily
    /// (mouse re-enters the viewport)
    virtual void	 resume(BM_SimpleState * = 0);

    /// Respond to mouse or keyboard events.
    virtual int		 handleMouseEvent(UI_Event *event);

    /// Render the brush "cursor" geometry:
    virtual void	 doRender(RE_Render *r, int x, int y, int ghost);

    /// sets the prompt's text
    virtual void	 updatePrompt();

    /// repositions the brush's guide geometry
    void		 updateBrush(int x, int y);

private:
    bool		 myIsBrushVisible;
    DM_Detail		 myBrushHandle;
    GU_Detail		 myBrushCursor;
    UT_Matrix4		 myBrushCursorXform;
    float		 myBrushRadius;

    /// These are used track the resizing of the cursor in the viewport
    bool		 myResizingCursor;
    short		 myLastCursorX, myLastCursorY;
    short		 myResizeCursorX, myResizeCursorY;
};
} // End HDK_Sample namespace

#endif
