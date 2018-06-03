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

#include <UT/UT_VectorTypes.h>
#include <UT/UT_Vector2.h>

#include <RE/RE_Geometry.h>
#include <RE/RE_Render.h>
#include <RE/RE_Texture.h>
#include <RE/RE_VertexArray.h>

#include <GUI/GUI_DisplayOption.h>
#include <GUI/GUI_ViewParameter.h>
#include <GUI/GUI_ViewState.h>
#include <DM/DM_VPortAgent.h>
#include <DM/DM_RenderTable.h>
#include <DM/DM_SceneHook.h>

#define CHECKER_CONNECT_GROUP 0

namespace HDK_Sample
{
static const char *theVert =
    "#version 150\n"
    "in vec2 P;\n"
    "out vec2 uv;\n"
    "uniform int width;\n"
    "uniform int height;\n"
    "uniform float check_size;\n"
    "void main() {\n"
    "  gl_Position = vec4(P,0.,1.);\n"
    "  uv = P * 0.5;\n"
    "  uv.x *= width  / check_size;\n"
    "  uv.y *= height / check_size;\n"
    "}\n";

static const char *theFrag = 
    "#version 150\n"
    "in vec2 uv;\n"
    "out vec4 color;\n"
    "uniform sampler2D checker;\n"
    "void main() {\n"
    "  color = texture(checker, uv);\n"
    "}\n";

    
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
    static RE_Shader *theBGShader;
};

RE_Shader *DM_BackgroundRenderHook::theBGShader = NULL;

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

    if(!theBGShader)
    {
	theBGShader = RE_Shader::create("Checker BG");
	theBGShader->addShader(r, RE_SHADER_VERTEX, theVert, "vert", 0);
	theBGShader->addShader(r, RE_SHADER_FRAGMENT, theFrag, "frag", 0);
	if(!theBGShader->linkShaders(r))
	    return false;
    }
    
    RE_VertexArray *pos = NULL;
	    
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
	pos = myQuad->createAttribute(r, "P", RE_GPU_FLOAT32, 2, NULL);
	myQuad->connectAllPrims(r,
				CHECKER_CONNECT_GROUP,
				RE_PRIM_TRIANGLE_STRIP);
    }
    else
	pos = myQuad->getAttribute("P");

    UT_Vector2FArray p(4,4);

    // full viewport quad
    p(0).assign(-1, -1);
    p(1).assign(-1,  1);
    p(2).assign( 1, -1);
    p(3).assign( 1,  1);

    pos->setArray(r, p.array());

    // Determine checker size, based off of the FOV.
    const GUI_ViewParameter &viewparms =
	viewport().getViewStateRef().getViewParameterRef();
    fpreal check_size = 16.0;
    
    if(viewparms.getOrthoFlag())
    {
	check_size =  100.0 / viewparms.getOrthoWidth();
    }
    else
    {
	const fpreal ap = viewparms.getAperture();
	const fpreal fc = viewparms.getFocalLength();
	const fpreal w = (ap / fc);
	check_size = 13.25 / w;
    }

    // Draw the textured quad
    const int tex_bind = theBGShader->getUniformTextureUnit("checker");
    r->pushTextureState(tex_bind);
    r->bindTexture(myCheckerTex, tex_bind);
    r->pushShader(theBGShader);
    theBGShader->bindInt(r, "width", hook_data.view_width);
    theBGShader->bindInt(r, "height", hook_data.view_height);
    theBGShader->bindFloat(r, "check_size", check_size);
    myQuad->draw(r, CHECKER_CONNECT_GROUP);
    r->popShader();
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
