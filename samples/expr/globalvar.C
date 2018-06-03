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
#include <CH/CH_LocalVariable.h>
#include <CMD/CMD_Manager.h>

static bool
getMSValueFloat(fpreal &f, int varid, int thread)
{
    f = CHgetManager()->getEvaluateTime(thread) * 1000.0;

    return true;
}

static bool
getProjValueStr(UT_String &str, int varid, int thread)
{
    str.harden("My Project Name");
    return true;
}

void
CMDextendLibrary(CMD_Manager *mgr)
{
    // Pass zero for the index value int the CH_LocalVariable initializers,
    // since it will just be replaced anyway. The actual variable id is
    // returned, in case you want several variables to be evaluated through the
    // same function.  In this case we don't care, so just throw the values
    // away.
    mgr->addVariable({"PROJ", 0, CH_VARIABLE_STRVAL},
	    getProjValueStr, NULL, NULL);
    mgr->addVariable({"MS", 0, CH_VARIABLE_TIME},
	    NULL, getMSValueFloat, NULL);
}

