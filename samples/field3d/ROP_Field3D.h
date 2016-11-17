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
 */

#ifndef __ROP_Field3D_h__
#define __ROP_Field3D_h__

#include <ROP/ROP_Node.h>

#define STR_PARM(name, vi, t) \
		{ evalString(str, name, vi, t); }
#define INT_PARM(name, vi, t) \
                { return evalInt(name, vi, t); }

class OP_TemplatePair;
class OP_VariablePair;

namespace HDK_Sample {

enum {
    ROP_F3D_RENDER,
    ROP_F3D_RENDER_CTRL,
    ROP_F3D_TRANGE,
    ROP_F3D_FRANGE,
    ROP_F3D_TAKE,
    ROP_F3D_SOPPATH,
    ROP_F3D_SOPOUTPUT,
    ROP_F3D_GRIDTYPE,
    ROP_F3D_BITDEPTH,
    ROP_F3D_COLLATE,
    ROP_F3D_INITSIM,
    ROP_F3D_ALFPROGRESS,
    ROP_F3D_TPRERENDER,
    ROP_F3D_PRERENDER,
    ROP_F3D_LPRERENDER,
    ROP_F3D_TPREFRAME,
    ROP_F3D_PREFRAME,
    ROP_F3D_LPREFRAME,
    ROP_F3D_TPOSTFRAME,
    ROP_F3D_POSTFRAME,
    ROP_F3D_LPOSTFRAME,
    ROP_F3D_TPOSTRENDER,
    ROP_F3D_POSTRENDER,
    ROP_F3D_LPOSTRENDER,

    ROP_F3D_MAXPARMS
};
class ROP_Field3D : public ROP_Node {
public:

    /// Provides access to our parm templates.
    static OP_TemplatePair	*getTemplatePair();
    static OP_VariablePair	*getVariablePair();
    /// Creates an instance of this node.
    static OP_Node		*myConstructor(OP_Network *net, const char*name,
						OP_Operator *op);

protected:
	     ROP_Field3D(OP_Network *net, const char *name, OP_Operator *entry);
    virtual ~ROP_Field3D();

    /// Called at the beginning of rendering to perform any intialization 
    /// necessary.
    /// @param	nframes	    Number of frames being rendered.
    /// @param	s	    Start time, in seconds.
    /// @param	e	    End time, in seconds.
    /// @return		    True of success, false on failure (aborts the render).
    virtual int			 startRender(int nframes, fpreal s, fpreal e);

    /// Called once for every frame that is rendered.
    /// @param	time	    The time to render at.
    /// @param	boss	    Interrupt handler.
    /// @return		    Return a status code indicating whether to abort the
    ///			    render, continue, or retry the current frame.
    virtual ROP_RENDER_CODE	 renderFrame(fpreal time, UT_Interrupt *boss);

    /// Called after the rendering is done to perform any post-rendering steps
    /// required.
    /// @return		    Return a status code indicating whether to abort the
    ///			    render, continue, or retry.
    virtual ROP_RENDER_CODE	 endRender();

public:

    /// A convenience method to evaluate our custom file parameter.
    void  OUTPUT(UT_String &str, fpreal t)
    { STR_PARM("file",  0, t) }
    int		INITSIM(void)
		    { INT_PARM("initsim", 0, 0) }
    bool	ALFPROGRESS()
		    { INT_PARM("alfprogress", 0, 0) }
    void	SOPPATH(UT_String &str, fpreal t)
		    { STR_PARM("soppath", 0, t)}

    int		GRIDTYPE(double t)
		    { INT_PARM("gridtype", 0, t) }
    int		BITDEPTH(double t)
		    { INT_PARM("bitdepth", 0, t) }

    bool	COLLATE(double t)
		    { INT_PARM("collatevector", 0, t) }

private:
    fpreal		 myEndTime;
    fpreal		 myStartTime;
};

}	// End HDK_Sample namespace


#undef STR_PARM
#undef STR_SET
#undef STR_GET

#endif
