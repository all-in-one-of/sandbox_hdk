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

#ifndef __MSS_BrushHairLen__
#define __MSS_BrushHairLen__

namespace HDK_Sample {
class MSS_BrushHairLen : public MSS_BrushBaseState
{
public:
	     MSS_BrushHairLen(JEDI_View &view, PI_StateTemplate &templ,
			    BM_SceneManager *scene,
			    const char *cursor = BM_DEFAULT_CURSOR);
    virtual ~MSS_BrushHairLen();

    /// This constructor and parameter template list go into the
    /// DM_StateTemplate for this state.
    static BM_State	*ourConstructor(BM_View &view, PI_StateTemplate &templ,
					BM_SceneManager *scene);
    static PRM_Template	 ourTemplateList[];

    /// The name and type of this class:
    virtual const char	*className() const;

    /// This method is used to get the name of the UI file.
    /// Normally it is prefixed by the UI directory, but as that varies
    /// from build to build, we hard code it.
    virtual void	 getUIFileName(UT_String &uifilename) const;

protected:
    /// Respond to keyboard events.
    virtual int		 handleKeyTypeEvent(UI_Event *event, BM_Viewport &);

    /// Convert an op menu entry to a brush operation:
    virtual SOP_BrushOp	 menuToBrushOp(const UI_Value &menuvalue) const;

    static PRM_ChoiceList	 theLMBMenu;
    static PRM_ChoiceList	 theMMBMenu;
};
} // End HDK_Sample namespace

#endif

