#include <UT/UT_DSOVersion.h>
#include <UT/UT_Map.h>

#include <RE/RE_Font.h>
#include <RE/RE_Render.h>
#include <RE/RE_OGLFramebuffer.h>
#include <RE/RE_OcclusionQuery.h>

#include <GUI/GUI_DisplayOption.h>
#include <DM/DM_GeoDetail.h>
#include <DM/DM_RenderTable.h>
#include <DM/DM_SceneHook.h>
#include <DM/DM_VPortAgent.h>

namespace HDK_Sample
{

class DM_OverdrawRender : public DM_SceneRenderHook
{
public:
	     DM_OverdrawRender(DM_VPortAgent &vport)
		 : DM_SceneRenderHook(vport, DM_VIEWPORT_ALL_3D),
		   myDepthFBO(NULL),
		   myDoQueries(false) {}
    
    virtual ~DM_OverdrawRender()
	{
	    delete myDepthFBO;
	}

    /// GL3 only because we use an occlusion query.
    virtual bool supportsRenderer(GR_RenderVersion version)
		 { return version >= GR_RENDER_GL3; }

    /// Because this hook is bound to multiple passes, differentiate between the
    /// various passes in our render method.
    virtual bool render(RE_Render *r, const DM_SceneHookData &hook_data)
	{
	    // check if the display option for this hook is on
	    if(!hook_data.disp_options->isSceneOptionEnabled("overdraw"))
		return false;

	    if(hook_data.hook_type == DM_HOOK_BEAUTY)
	    {
		if(hook_data.hook_policy == DM_HOOK_BEFORE_NATIVE)
		{
		    // render a depth-only, depth test = always pass to
		    // determine how many samples are actually drawn.
		    // then start an occlusion query for the native beauty pass.
		    myDoQueries = doPreBeauty(r, hook_data);
		}
		else if(myDoQueries)
		{
		    // end the occlusion query for the beauty pass.
		    doPostBeauty(r);
		}
	    }
	    else if(myDoQueries) // && DM_HOOK_FOREGROUND
	    {
		// render the collected information to the foreground pass,
		// which is rendered after all the beauty pass items.
		renderInfo(r, hook_data);
	    }

	    // allow other hooks bound to this pass to render by returning false
	    return false;
	}

    // Free our resources (the FBO and associated renderbuffer) when the
    // viewport is closed.
    virtual void viewportClosed()
	{
	    delete myDepthFBO;
	    myDepthFBO = NULL;
	    
	    myQuery1.destroy();
	    myQuery2.destroy();
	}

    // Reference count so that the DM_OverdrawHook knows when to delete this
    // object.
    int		bumpRefCount(bool inc)
		{
		    myRefCount += (inc?1:-1);
		    return myRefCount;
		}
    
private:
    bool	doPreBeauty(RE_Render *r, const DM_SceneHookData &hook_data);
    void	doPostBeauty(RE_Render *r);
    void	renderInfo(RE_Render *r, const DM_SceneHookData &hook_data);

    
    RE_OGLFramebuffer	*myDepthFBO;
    RE_OcclusionQuery	 myQuery1;
    RE_OcclusionQuery	 myQuery2;
    bool		 myDoQueries;
    int			 myRefCount;
};

class DM_OverdrawHook : public DM_SceneHook
{
public:
	     DM_OverdrawHook() : DM_SceneHook("Overdraw", 0) {}
    virtual ~DM_OverdrawHook() {}
    
    virtual DM_SceneRenderHook *newSceneRender(DM_VPortAgent &vport,
					       DM_SceneHookType type,
					       DM_SceneHookPolicy policy)
	{
	    // Only for 3D views (persp, top, bottom; not the UV viewport)
	    if(!(vport.getViewType() & DM_VIEWPORT_ALL_3D))
		return NULL;
	    
	    DM_OverdrawRender *hook = NULL;

	    // Create only 1 per viewport, even though we're registered many
	    // times for different passes.
	    
	    const int id = vport.getUniqueId();
	    UT_Map<int,DM_OverdrawRender *>::iterator it=mySceneHooks.find(id);

	    if(it != mySceneHooks.end())
	    {
		// found existing hook for this viewport, reuse it.
		hook = it->second;
	    }
	    else
	    {
		// no hook for this viewport; create it.
		hook = new DM_OverdrawRender(vport);
		mySceneHooks[id] = hook;
	    }

	    // increase reference count on the render so we know when to delete
	    // it.
	    hook->bumpRefCount(true);
	    
	    return hook;
	}
    
    virtual void retireSceneRender(DM_VPortAgent &vport,
				   DM_SceneRenderHook *hook)
	{
	    // If the ref count is zero, we're the last retire call. delete the
	    // hook.
	    if(static_cast<DM_OverdrawRender *>(hook)->bumpRefCount(false) == 0)
	    {
		// Remove from the map and delete the hook.
		const int id = vport.getUniqueId();
		UT_Map<int,DM_OverdrawRender *>::iterator it
		    = mySceneHooks.find(id);
		
		if(it != mySceneHooks.end())
		    mySceneHooks.erase(id);
	    
		delete hook;
	    }
	}
    
private:
    UT_Map<int, DM_OverdrawRender *> mySceneHooks;
};

}; // end namespace HDK_Sample

using namespace HDK_Sample;

bool
DM_OverdrawRender::doPreBeauty(RE_Render *r, const DM_SceneHookData &hook_data)
{
    // Initialize both query objects, if not already done.
    myQuery1.init(r);
    myQuery2.init(r);

    // AA setting is 2^getSceneAntialias().
    int aa_samples = hook_data.disp_options->common().getSceneAntialias();
    aa_samples = 1 << aa_samples;

    // Create the throwaway depth buffer we'll be using as a target when doing
    // the first occlusion query.
    if(!myDepthFBO ||
       myDepthFBO->getWidth() != hook_data.view_width ||
       myDepthFBO->getWidth() != hook_data.view_height ||
       myDepthFBO->getSamples() != aa_samples)
    {
	if(myDepthFBO)
	{
	    delete myDepthFBO;
	    myDepthFBO = NULL;
	}

	myDepthFBO = new RE_OGLFramebuffer("DM_OverdrawHook");
	myDepthFBO->setResolution(hook_data.view_width, hook_data.view_height);
	myDepthFBO->setSamples(aa_samples);
	myDepthFBO->createRenderbuffer(r, RE_GPU_FLOAT32, 1, RE_DEPTH_BUFFER);

	if(!myDepthFBO->isValid(r))
	{
	    // if it cannot be created, this hook can't run.
	    delete myDepthFBO;
	    myDepthFBO = NULL;
	    return false;
	}
    }

    // determine the total number of samples drawn by rendering all geometry in
    // the scene without depth testing. Count the samples with a GL occlusion
    // query (myQuery1).
    r->pushDrawFramebuffer(myDepthFBO);
    myDepthFBO->drawToBuffer(r, RE_DEPTH_BUFFER);
    
    r->pushDepthState();
    r->setZFunction(RE_ZALWAYS);
    r->enableDepthBufferWriting();

    myQuery1.begin(r);
    viewport().renderGeometry(r, GR_RENDER_DEPTH, GR_SHADING_SOLID,
			      GR_ALPHA_PASS_OPAQUE);
    myQuery1.end(r);

    r->popDepthState();
    
    r->popDrawFramebuffer();

    // Start the query for the main beauty pass.
    myQuery2.begin(r);

    return true;
}

void
DM_OverdrawRender::doPostBeauty(RE_Render *r)
{
    // Complete the occlusion query for the main beauty pass.
    myQuery2.end(r);
}

void
DM_OverdrawRender::renderInfo(RE_Render *r, const DM_SceneHookData &hook_data)
{
    // grab the occlusion query results from the GPU.
    int num1 = myQuery1.getNumDrawn(r);
    int num2 = myQuery2.getNumDrawn(r);
    fpreal overdraw = (num1 > 0) ? fpreal(num1-num2) / fpreal(num1) : 0.0;
    UT_WorkBuffer string;

    // format into a nice info string
    string.sprintf("Overdraw %d%% (%d samples)", int(overdraw*100), num1-num2);
    
    RE_Font &font = RE_Render::getViewportFont();
    int w = font.getStringWidth(string.buffer());
    int h = font.getHeight();
   
    // Draw the info string at the bottom of the viewport, centered.
    r->pushColor(hook_data.disp_options->common().defaultWireColor());
    r->textMoveW( (hook_data.view_width - w) * 0.5, h *0.5);
    r->putString( string.buffer() );
    r->popColor();
}

void
newRenderHook(DM_RenderTable *table)
{  
    DM_OverdrawHook *hook = new DM_OverdrawHook();

    // Register the hook multiple times so that it executes both before and
    // after the beauty pass, and in the foreground pass.
    
    // This allows the hook to render a depth pass to determine all the samples
    // rendered and do an occlusion query around the native beauty pass. Then
    // the results are displayed to the user in the foreground pass.
    table->registerSceneHook(hook,
			     DM_HOOK_BEAUTY,
			     DM_HOOK_BEFORE_NATIVE);
    table->registerSceneHook(hook,
			     DM_HOOK_BEAUTY,
			     DM_HOOK_AFTER_NATIVE);
    table->registerSceneHook(hook,
			     DM_HOOK_FOREGROUND,
			     DM_HOOK_AFTER_NATIVE);
    
    table->installSceneOption("overdraw", "Overdraw info");
}
