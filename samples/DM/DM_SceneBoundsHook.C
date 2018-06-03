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

#include <RE/RE_Font.h>
#include <RE/RE_Geometry.h>
#include <RE/RE_Render.h>
#include <RE/RE_Shader.h>
#include <GUI/GUI_DisplayOption.h>

#include <DM/DM_GeoDetail.h>

#include <DM/DM_RenderTable.h>
#include <DM/DM_SceneHook.h>
#include <DM/DM_VPortAgent.h>

namespace HDK_Sample {

const char *vert_shader =
    "#version 150 \n"
    "uniform mat4 glH_ProjectMatrix; \n"
    "uniform mat4 glH_ViewMatrix; \n"
    "in vec3 P; \n"
    "in vec3 Cd; \n"
    "out vec4 clr; \n"
    "void main() \n"
    "{ \n"
    "  clr = vec4(Cd, 1.0); \n"
    "  gl_Position = glH_ProjectMatrix * glH_ViewMatrix * vec4(P, 1.0); \n"
    "} \n";
    
const char *frag_shader =
    "#version 150 \n"
    "in vec4 clr; \n"
    "out vec4 color; \n"
    "void main() \n"
    "{ \n"
    "  color = clr; \n"
    "} \n";
	

// This sample shows the bounding box of all objects in the scene.

class DM_SceneBoundsRenderHook : public DM_SceneRenderHook
{
public:
    DM_SceneBoundsRenderHook(DM_VPortAgent &vport)
	: DM_SceneRenderHook(vport, DM_VIEWPORT_ALL_3D),
	  myShader(NULL)
	{}
    virtual ~DM_SceneBoundsRenderHook() { delete myShader; }
    
    virtual bool	render(RE_Render *r,
			       const DM_SceneHookData &hook_data)
	{
	    // Check our display option, if not enabled, exit.
	    if(hook_data.disp_options->isSceneOptionEnabled("scene_bounds"))
		renderSceneBounds(r, hook_data.disp_options);
		
	    return false; // allow other hooks of this type to render
	}

    void renderSceneBounds(RE_Render *r, const GUI_DisplayOption *opts)
	{
	    int i;
	    DM_GeoDetail gd;
	    UT_BoundingBox box, scene_box;
	    bool init = false;

	    for(i=0; i<viewport().getNumOpaqueObjects(); i++)
	    {
		gd = viewport().getOpaqueObject(i);
		if(gd.getBoundingBox3D(box))
		{
		    if(!init)	scene_box = box;
		    else	scene_box.enlargeBounds(box);
		    init = true;
		}
	    }

	    for(i=0; i<viewport().getNumTransparentObjects(); i++)
	    {
		gd = viewport().getTransparentObject(i);
		if(gd.getBoundingBox3D(box))
		{
		    if(!init)	scene_box = box;
		    else	scene_box.enlargeBounds(box);
		    init = true;
		}
	    }
	    
	    for(i=0; i<viewport().getNumUnlitObjects(); i++)
	    {
		gd = viewport().getUnlitObject(i);
		if(gd.getBoundingBox3D(box))
		{
		    if(!init)	scene_box = box;
		    else	scene_box.enlargeBounds(box);
		    init = true;
		}
	    }

	    if(!init)
		return;

	    RE_Geometry geo(24);
	    UT_Vector3FArray pos;
	    UT_Vector3FArray col;

	    createBoundingBox(scene_box, opts->common().defaultWireColor(),
			      pos, col);

	    geo.createAttribute(r, "P", RE_GPU_FLOAT32, 3, pos.array());
	    geo.createAttribute(r, "Cd", RE_GPU_FLOAT32, 3, col.array());
	    geo.connectAllPrims(r, 0, RE_PRIM_LINES);

	    // The GL3 viewport does not set the GL builtin view matrices.
	    // Use a simple shader to display the box.
	    if(!myShader)
	    {
		myShader = RE_Shader::create("lines");
		myShader->addShader(r, RE_SHADER_VERTEX, vert_shader,
				    "vertex", 0);
		myShader->addShader(r, RE_SHADER_FRAGMENT, frag_shader,
				    "fragment", 0);
		myShader->linkShaders(r);
	    }

	    r->pushShader(myShader);

	    geo.draw(r, 0);

	    r->popShader();
	}

    void createBoundingBox(const UT_BoundingBox &box,
			   const UT_Color &color,
			   UT_Vector3FArray &pos,
			   UT_Vector3FArray &col)
	{
	    UT_Vector3 pnt[8];

	    box.getBBoxPoints(pnt);

	    // line pairs
	    pos.append(pnt[0]);	    pos.append(pnt[1]);
	    pos.append(pnt[1]);	    pos.append(pnt[3]);
	    pos.append(pnt[3]);	    pos.append(pnt[2]);
	    pos.append(pnt[2]);	    pos.append(pnt[0]);
    
	    pos.append(pnt[4]);	    pos.append(pnt[5]);
	    pos.append(pnt[5]);	    pos.append(pnt[7]);
	    pos.append(pnt[7]);	    pos.append(pnt[6]);
	    pos.append(pnt[6]);	    pos.append(pnt[4]);

	    pos.append(pnt[0]);	    pos.append(pnt[4]);
	    pos.append(pnt[1]);	    pos.append(pnt[5]);
	    pos.append(pnt[2]);	    pos.append(pnt[6]);
	    pos.append(pnt[3]);	    pos.append(pnt[7]);

	    col.appendMultiple(color.rgb(), 24);
	}
private:
    RE_Shader *myShader;
};

class DM_SceneBoundsHook : public DM_SceneHook
{
public:
    DM_SceneBoundsHook() : DM_SceneHook("Scene Bounding Box", 0)
	{}

    virtual DM_SceneRenderHook *newSceneRender(DM_VPortAgent &vport,
					       DM_SceneHookType type,
					       DM_SceneHookPolicy policy)
	{
	    return new DM_SceneBoundsRenderHook(vport);
	}

    virtual void		retireSceneRender(DM_VPortAgent &vport,
						  DM_SceneRenderHook *hook)
	{
	    // Shared hooks might deference a ref count, but this example just
	    // creates one hook per viewport.
	    delete hook;
	}
};

} // END HDK_Sample namespace

using namespace HDK_Sample;


void
newRenderHook(DM_RenderTable *table)
{
    table->registerSceneHook(new DM_SceneBoundsHook,
				       DM_HOOK_UNLIT,
				       DM_HOOK_AFTER_NATIVE);
    table->installSceneOption("scene_bounds", "Scene Bounds");
}
