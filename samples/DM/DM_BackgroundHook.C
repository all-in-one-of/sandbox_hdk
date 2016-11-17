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
#include <UT/UT_DSOVersion.h>

#include <UT/UT_VectorTypes.h>
#include <UT/UT_Vector2.h>

#include <RE/RE_Geometry.h>
#include <RE/RE_Render.h>
#include <RE/RE_Texture.h>
#include <RE/RE_VertexArray.h>

#include <GUI/GUI_DisplayOption.h>
#include <DM/DM_RenderTable.h>
#include <DM/DM_SceneHook.h>

namespace HDK_Sample {

// This hook is the actual render hook, responsible for doing the GL drawing.
class DM_BackgroundRenderHook : public DM_SceneRenderHook
{
public:
    DM_BackgroundRenderHook(DM_VPortAgent &vport)
	: DM_SceneRenderHook(vport, DM_VIEWPORT_ALL),
	  myCheckerTex(NULL),
	  myQuad(NULL)
	{}

    virtual ~DM_BackgroundRenderHook()
	{
	    delete myCheckerTex;
	    delete myQuad;
	}
    
    virtual bool		render(RE_Render *r,
				       const DM_SceneHookData &hook_data);
    virtual void		viewportClosed();
    
private:
    RE_Texture	*myCheckerTex;
    RE_Geometry *myQuad;
};


// This hook is responsible for creating and deleting render hooks per viewport.
// This is the hook that is registered with the DM_RenderTable.    
class DM_BackgroundHook : public DM_SceneHook
{
public:
	     DM_BackgroundHook() : DM_SceneHook("Checkered Background", 0) {}
    virtual ~DM_BackgroundHook() {}
    
    virtual DM_SceneRenderHook *newSceneRender(DM_VPortAgent &vport,
					       DM_SceneHookType type,
					       DM_SceneHookPolicy policy);
    virtual void		retireSceneRender(DM_VPortAgent &vport,
						  DM_SceneRenderHook *hook);
};

} // end HDK_Sample namespace



using namespace HDK_Sample;

// Hook management code

DM_SceneRenderHook *
DM_BackgroundHook::newSceneRender(DM_VPortAgent &vport,
				  DM_SceneHookType type,
				  DM_SceneHookPolicy policy)
{
    return new DM_BackgroundRenderHook(vport);
}

void
DM_BackgroundHook::retireSceneRender(DM_VPortAgent &vport,
				     DM_SceneRenderHook *hook)
{
    // Shared hooks might deference a ref count, but this example just
    // createhowever,s one hook per viewport.
    delete hook;
}



// Drawing code for hook

bool
DM_BackgroundRenderHook::render(RE_Render *r,
				const DM_SceneHookData &hook_data)
{
    // Check if the scene option this hook installed is enabled. If not, allow
    // other hooks to render (or the Houdini background to render) by returning
    // false.
    if(!hook_data.disp_options->isSceneOptionEnabled("checker_bg"))
	return false;
    
    RE_VertexArray *pos, *tex;
	    
    if(!myCheckerTex)
    {
	// Checker pattern (2x2 RGB, 8b fixed)
	const uint8 data[] = { 190,190,190, 180,180,180,
			       180,180,180, 190,190,190 };

	// Create a 2x2 repeating texture with point (nearest) filtering for
	// the texture.
	myCheckerTex = RE_Texture::newTexture(RE_TEXTURE_2D);
	myCheckerTex->setResolution(2,2);
	myCheckerTex->setFormat(RE_GPU_UINT8, 3);
	myCheckerTex->setMinFilter(r, RE_FILT_NEAREST);
	myCheckerTex->setMagFilter(r, RE_FILT_NEAREST);
	myCheckerTex->setTextureWrap(r,RE_CLAMP_REPEAT,RE_CLAMP_REPEAT);
	myCheckerTex->setTexture(r, data);

	// Create a viewport-sized quad with texture coords.
	myQuad = new RE_Geometry(4);
	pos = myQuad->createAttribute(r, "P", RE_GPU_FLOAT32, 3, NULL);
	tex = myQuad->createAttribute(r, "uv", RE_GPU_FLOAT32, 2, NULL);
	myQuad->connectAllPrims(r, 0, RE_PRIM_TRIANGLE_STRIP);
    }
    else
    {
	pos = myQuad->getAttribute("P");
	tex = myQuad->getAttribute("uv");
    }

    UT_Vector3FArray p(4,4);
    UT_Vector2FArray t(4,4);

    // full viewport quad
    p(0).assign(0,			0, 0);
    p(1).assign(hook_data.view_width-1, 0, 0);
    p(2).assign(0,			hook_data.view_height-1, 0);
    p(3).assign(hook_data.view_width-1, hook_data.view_height-1, 0);

    // texture coords (8x8 checker size, tex size = 2, so 1/(2*8) scale).
    t(0) = UT_Vector2(p(0).x(), p(0).y()) * 0.0675;
    t(1) = UT_Vector2(p(1).x(), p(1).y()) * 0.0675;
    t(2) = UT_Vector2(p(2).x(), p(2).y()) * 0.0675;
    t(3) = UT_Vector2(p(3).x(), p(3).y()) * 0.0675;
	    
    pos->setArray(r, p.array());
    tex->setArray(r, t.array());

    // Draw the textured quad
    r->pushTextureState(0);
    r->bindTexture(myCheckerTex, 0, RE_TEXTURE_REPLACE);
	    
    myQuad->draw(r, 0);

    r->popTextureState();

    // Returning true indicates that no other background replacement hooks
    // should run. If false was returned, the next highest priority hook would
    // be run. If all hooks return false, Houdini draws the background.
    return true;
}
    
void
DM_BackgroundRenderHook::viewportClosed()
{
    // With a bigger GL resource, this would be a good way to keep
    // VRAM usage low. It isn't really necessary in this example but
    // is here as an illustation of the process.
    
    delete myCheckerTex;
    myCheckerTex = NULL;
    
    delete myQuad;
    myQuad = NULL;
}

// DSO hook registration function
void
newRenderHook(DM_RenderTable *table)
{
    // Register this hook as a replacement for the background rendering. 
    table->registerSceneHook(new DM_BackgroundHook,
			     DM_HOOK_BACKGROUND,
			     DM_HOOK_REPLACE_NATIVE);

    // Note that the name or label doesn't have to match the hook's name.
    table->installSceneOption("checker_bg", "Checkered Background");
}
