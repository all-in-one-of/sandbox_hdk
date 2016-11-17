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

#include <string.h>
#include <malloc.h>
#include <math.h>
#include <UT/UT_DSOVersion.h>
#include <EXPR/EXPR.h>
#include <OP/OP_Director.h>
#include <CH/CH_Support.h>
#include <CH/CH_Channel.h>
#include <CMD/CMD_Manager.h>


static void
getTimeValues(int thread,
	      fpreal &t, fpreal &t0, fpreal &t1, fpreal &v0, fpreal &v1,
	      fpreal *m0=0, fpreal *m1=0, fpreal *a0=0, fpreal *a1=0)
{
    // Get the current time and the values for the function
    CH_Channel::getGlueTime(t, t0, t1, v0, v1, thread);

    // Now, get the slope & accell
    if (m0 || m1 || a0 || a1)
	CH_Channel::getGlueSlope(m0, m1, a0, a1, thread);
}

#ifndef EV_START_FNNA
#define EV_START_FNNA(name) \
	    static void name(EV_FUNCTION *, EV_SYMBOL *result, \
			     EV_SYMBOL **, int thread)
#endif // EV_START_FNNA

EV_START_FNNA(fn_myLinear)
{
    fpreal	t, t0, t1, v0, v1;
    fpreal	dt;

    getTimeValues(thread, t, t0, t1, v0, v1);
    dt = t1 - t0;
    result->value.fval = (dt == 0) ? (v0+v1)*0.5F
				   : v0 + (v1-v0)*(t - t0)/dt;
}

#define EVF	EV_TYPEFLOAT
#define EVS	EV_TYPESTRING
#define EVV	EV_TYPEVECTOR

//
//  These flags are important so that JIVE will display the correct handles for
//  the curve.
#define VAL_FLAGS	(CH_EXPRVALUE)
#define VALSLOPE_FLAGS	(CH_EXPRVALUE | CH_EXPRSLOPE)

static EV_FUNCTION funcTable[] = {
    EV_FUNCTION(VAL_FLAGS, "myLinear",	0, EVF,	0,	fn_myLinear),
    EV_FUNCTION(),
};

void
CMDextendLibrary(CMD_Manager *)
{
    int		i;

    for (i = 0; funcTable[i].getName(); i++)
	ev_AddFunction(&funcTable[i]);
}
