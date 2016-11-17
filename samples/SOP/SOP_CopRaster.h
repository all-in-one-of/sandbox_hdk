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

#ifndef __SOP_CopRaster_h__
#define __SOP_CopRaster_h__

#include <UT/UT_String.h>
#include <IMG/IMG_Raster.h>
#include <SOP/SOP_Node.h>

namespace HDK_Sample {
class SOP_CopRaster : public SOP_Node
{
public:
    static OP_Node	*myConstructor(OP_Network*, const char *, OP_Operator*);
    static PRM_Template	 myTemplateList[];

    /// This static methods is used to build a menu for the UI.
    static void	buildColorMenu(void *data, PRM_Name *, int,
				const PRM_SpareData *, const PRM_Parm *);

protected:
	     SOP_CopRaster(OP_Network*, const char *, OP_Operator*);
    virtual ~SOP_CopRaster();

    virtual OP_ERROR		 cookMySop(OP_Context &context);

    virtual bool	 updateParmsFlags();

    /// Splits a full cop2 path into the net and node portions.
    void	splitCOP2Path(const char *path, UT_String &net,
			      UT_String &node);

    /// This takes a path relative to this node, and finds the full path
    /// that should be sent to the CopResolver.  Flag dependent is set if
    /// an interest in the flag of the destination should be set.
    int		getFullCOP2Path(const char *relpath, UT_String &fullpath,
				int &flagdependent);

private:
    int			updateRaster(fpreal t);

    UT_String		myCurrentName;
    int                 myPrevXRes;
    int                 myPrevYRes;
    IMG_Raster		myRaster;

    int		USEDISK()			
		{ return evalInt("usedisk", 0, 0); }
    void	COPPATH(UT_String &str, fpreal t)
		{ evalString(str, "coppath", 0, t); }
    void	CPLANE(UT_String &str, fpreal t) 
		{ evalString(str, "copcolor", 0, t); }
    void	FNAME(UT_String &str, fpreal t)	
		{ evalString(str, "file", 0, t); }
    fpreal	COPFRAME(fpreal t)		
		{ return evalFloat("copframe", 0, t); }
};
} // End HDK_Sample namespace

#endif
