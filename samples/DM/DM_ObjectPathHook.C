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

#include <UT/UT_Matrix4.h>
#include <UT/UT_Vector4.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_VectorTypes.h>

#include <RE/RE_Font.h>
#include <RE/RE_Geometry.h>
#include <RE/RE_Render.h>
#include <RE/RE_Shader.h>
#include <RE/RE_VertexArray.h>

#include <CH/CH_Manager.h>
#include <OP/OP_Context.h>
#include <OBJ/OBJ_Node.h>

#include <GUI/GUI_DisplayOption.h>

#include <DM/DM_GeoDetail.h>
#include <DM/DM_RenderTable.h>
#include <DM/DM_SceneHook.h>
#include <DM/DM_VPortAgent.h>

namespace HDK_Sample {
    
// This sample shows the animation path of the current object, shaded with the
// object's speed and showing the up vector.


// A simple shader to position and color vertices.
const char *vert_shader =
    "#version 150 \n"
    "uniform mat4 glH_ProjectMatrix; \n"
    "uniform mat4 glH_ViewMatrix; \n"
    "in vec3 P; \n"
    "in vec4 Cd; \n"
    "out vec4 clr; \n"
    "void main() \n"
    "{ \n"
    "  clr = Cd; \n"
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
	

// The part of the hook that is intantiated per viewport and does the work
class DM_ObjectPathRenderHook : public DM_SceneRenderHook
{
public:
    DM_ObjectPathRenderHook(DM_VPortAgent &vport)
	: DM_SceneRenderHook(vport, DM_VIEWPORT_ALL_3D),
	  myShader(NULL),
	  myGeo(NULL),
	  myStartFrame(0),
	  myEndFrame(0),
	  myCurrentObjId(OP_INVALID_NODE_ID),
	  myNormalScale(0.0),
	  myVectorScale(0.0)
	{}
    
    virtual ~DM_ObjectPathRenderHook() { delete myShader; delete myGeo; }
    
    virtual bool	render(RE_Render *r,
			       const DM_SceneHookData &hook_data);
    virtual void	viewportClosed();

    void	 rebuildPathGeometry(RE_Render *r,
				     OBJ_Node *obj,
				     int start_frame,
				     int end_frame,
				     RE_CacheVersion version,
				     fpreal vector_scale,
				     fpreal normal_scale);
    OBJ_Node    *getCurrentObject() const;
    
    void	 drawPathGeometry(RE_Render *r);

private:
    RE_Shader	*myShader;
    RE_Geometry *myGeo;

    // Cache previous parms to determine if an update is required
    int		 myStartFrame;
    int		 myEndFrame;
    int		 myCurrentObjId;
    fpreal	 myNormalScale;
    fpreal	 myVectorScale;
};

// Hook allocation and management
class DM_ObjectPathHook : public DM_SceneHook
{
public:
    DM_ObjectPathHook() : DM_SceneHook("Object path display", 0)
	{}

    virtual DM_SceneRenderHook *newSceneRender(DM_VPortAgent &vport,
					       DM_SceneHookType type,
					       DM_SceneHookPolicy policy)
	{
	    return new DM_ObjectPathRenderHook(vport);
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

bool
DM_ObjectPathRenderHook::render(RE_Render *r,
				const DM_SceneHookData &hook_data)
{
    // Check our display option, if not enabled, exit.
    if(!hook_data.disp_options->isSceneOptionEnabled("object_path"))
	return false;

    OBJ_Node *cur_obj = getCurrentObject();
    if(cur_obj)
    {
	RE_CacheVersion version;
	int obj_id   = cur_obj->getUniqueId();
	int start_fr = CHgetManager()->getGlobalStartFrame();
	int end_fr   = CHgetManager()->getGlobalEndFrame();
	fpreal nml_scale = hook_data.disp_options->common().normalScale();
	fpreal vec_scale = hook_data.disp_options->common().vectorScale();
	
	RE_VertexArray *pos = myGeo ? myGeo->getAttribute("P") : 0;
	RE_VertexArray *col = myGeo ? myGeo->getAttribute("Cd") : 0;

	version.setElement(0, cur_obj->getVersionParms());

	// update check - obj, frame range, transforms changed
	if(!pos || !col ||
	   pos->getCacheVersion() != version ||
	   col->getCacheVersion() != version ||
	   obj_id   != myCurrentObjId ||
	   start_fr != myStartFrame ||
	   end_fr != myEndFrame ||
	   !SYSisEqual(vec_scale, myVectorScale) ||
	   !SYSisEqual(nml_scale, myNormalScale))
	{
	    // update check failed, rebuild path
	    rebuildPathGeometry(r, cur_obj, start_fr, end_fr, version,
				vec_scale, nml_scale);
	    
	    myStartFrame = start_fr;
	    myEndFrame = end_fr;
	    myCurrentObjId = obj_id;
	    myNormalScale = nml_scale;
	    myVectorScale = vec_scale;
	}

	// draw path if the geometry exists.
	if(myGeo)
	    drawPathGeometry(r);
    }
    else
    {
	// no object, ensure geometry is cleared.
	if(myGeo)
	{
	    myGeo->purgeBuffers();
	    delete myGeo;
	    myGeo = NULL;
	}
    }

    return false;
}

OBJ_Node *
DM_ObjectPathRenderHook::getCurrentObject() const
{
    int i;
    DM_GeoDetail itr;
    DM_GeoDetail cur_obj;

    // Look for the current object in the various object lists
    for(i=0; i<viewport().getNumOpaqueObjects() && !cur_obj.isValid(); i++)
    {
	itr = viewport().getOpaqueObject(i);
	if(itr.isCurrentObject())
	    cur_obj = itr;
    }		    

    for(i=0; i<viewport().getNumTransparentObjects() && !cur_obj.isValid(); i++)
    {
	itr = viewport().getTransparentObject(i);
	if(itr.isCurrentObject())
	    cur_obj = itr;
    }
	    
    for(i=0; i<viewport().getNumUnlitObjects() && !cur_obj.isValid(); i++)
    {
	itr = viewport().getUnlitObject(i);
	if(itr.isCurrentObject())
	    cur_obj = itr;
    }

    OBJ_Node *obj =
	dynamic_cast<OBJ_Node *>(cur_obj.isValid()?cur_obj.getObject() : NULL);
    if(obj)
    {
	OP_Context context(CHgetEvalTime());

	// do not draw the path if the object isn't moving
	obj->cook(context);
	if(obj->isTimeDependent(context))
	    return obj;
    }

    return NULL;
}

void
DM_ObjectPathRenderHook::rebuildPathGeometry(RE_Render *r,
					     OBJ_Node *obj,
					     int start_frame,
					     int end_frame,
					     RE_CacheVersion version,
					     fpreal vector_scale,
					     fpreal normal_scale)
{
    // Create a unique cache name for the cached buffers.
    UT_String cachename;
   
    obj->getFullPath(cachename);
    cachename += "-obj-path";
    
    if(!myGeo)
	myGeo = new RE_Geometry;
    else
	myGeo->purgeBuffers();

    // Enables caching of the vertex buffers in the GL cache under the given
    // base name
    myGeo->cacheBuffers(cachename);

    // Pre-compute number of vertices in the geometry (1 line per position plus
    // a line between points).
    int n = (end_frame-start_frame) * 4 + 2;
    myGeo->setNumPoints( n );

    UT_Vector3FArray pos( n );
    UT_Vector4FArray col( n );
    UT_IntArray point_index;
    UT_Vector3F prev_pos;
    UT_Vector4F prev_clr;
    bool first = true;

    // build the up vectors and object path
    for(int fr = start_frame; fr<=end_frame; fr++)
    {
	OP_Context context;
	UT_Matrix4D trans;
	UT_Vector4D p(0.0, 0.0, 0.0, 1.0);
	UT_Vector4D pn(0.0, normal_scale, 0.0, 1.0);
	UT_Vector3F p3, pn3;
	context.setFrame(long(fr));

	obj->getLocalToWorldTransform(context, trans);

	// create the point along the path
	p *= trans;
	p.homogenize();

	// create an up vector point off the path (+Y)
	pn *= trans;
	pn.homogenize();
	
	p3.assign(p.x(), p.y(), p.z());
	pn3.assign(pn.x(), pn.y(), pn.z());

	pos.append(p3);

	if(fr % 10 == 0)
	    point_index.append( pos.entries() -1 );
	
	pos.append(pn3);
	
	if(!first)
	{
	    UT_Vector4F clr, clr_tip;
	    UT_Vector3F diff = p3 - prev_pos;
	    fpreal speed = diff.length() * vector_scale;

	    clr.assign( SYSmin(speed, 1.0), 0.5, 1.0-SYSmin(speed, 1.0), 1.0);
	    clr_tip = clr;
	    clr_tip.w() = 0.2;

	    col.append(clr);
	    col.append(clr_tip);

	    // the first normal doesn't have its color assigned yet, use the
	    // same color as the 2nd. 
	    if(fr == start_frame + 1)
	    {
		col.append(clr);
		col.append(clr_tip);
	    }
	    
	    // line segment
	    pos.append(prev_pos);
	    col.append(prev_clr);
	    pos.append(p3);
	    col.append(clr);

	    prev_clr = clr;
	}
	else
	    first = false;

	prev_pos = p3;
    }

    // Create the vertex and color arrays 
    RE_VertexArray *pa;
    pa = myGeo->createAttribute(r, "P", RE_GPU_FLOAT32, 3, pos.array());
    if(pa)
	pa->setCacheVersion(version);
    
    RE_VertexArray *ca;
    ca = myGeo->createAttribute(r, "Cd", RE_GPU_FLOAT32, 4, col.array());
    if(ca)
	ca->setCacheVersion(version);

    // Connect all the vertices using lines.
    if(ca && pa)
    {
	myGeo->connectAllPrims(r, 0, RE_PRIM_LINES, NULL,
				true /*replace_exiting*/);
	myGeo->connectIndexedPrims(r, 1, RE_PRIM_POINTS, point_index.entries(),
				    (uint *)point_index.array(), NULL,
				    true /*replace_exiting*/);
    }
    else
    {
	myGeo->purgeBuffers();
	delete myGeo;
	myGeo = NULL;
    }
}

void
DM_ObjectPathRenderHook::drawPathGeometry(RE_Render *r)
{
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

    // Use blending (Sa, 1-Sa) to draw a tapered looking normal.
    r->pushBlendState();
    r->blendAlpha(1);

    r->pushPointSize(4.0);
    myGeo->draw(r, 1); // points
    r->popPointSize();
    
    myGeo->draw(r, 0); // lines

    r->popBlendState();
    r->popShader();
}

void
DM_ObjectPathRenderHook::viewportClosed()
{
    // Clear our geometry to save some VRAM when viewport is inactive.
    if(myGeo)
    {
	myGeo->purgeBuffers();
	delete myGeo;
	myGeo = NULL;
    }
}


// Register the render hook and its corresponding display option

void
newRenderHook(DM_RenderTable *table)
{
    // Register the scene hook in the post-render pass, before any Houdini
    // post-render elements are drawn.
    table->registerSceneHook(new DM_ObjectPathHook,
			     DM_HOOK_POST_RENDER,
			     DM_HOOK_BEFORE_NATIVE);

    // add a new scene option called 'Object path' to control visibility of this
    // hook
    table->installSceneOption("object_path", "Object path");
}
