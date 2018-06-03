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
 * This is a sample VEX operator DSO
 */

#if defined(LINUX)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Thread.h>
#include <VEX/VEX_VexOp.h>

using namespace UT::Literal;

namespace HDK_Sample {
//
// Callback for time() function
//
static void
pid_Evaluate(int, void *argv[], void *)
{
    VEXint	*result = (VEXint *)argv[0];
    *result = getpid();
}

static void
tid_Evaluate(int, void *argv[], void *)
{
    VEXint	*result = (VEXint *)argv[0];
    *result = UT_Thread::getMyThreadId();
}

static void
printStuff(int, void *argv[], void *)
{
    VEXint	*result = (VEXint *)argv[0];
    static int	callCount = 0;
    fprintf(stderr, "Still here %d\n", callCount++);
    result[0] = 0;
}

static void *
initFunc()
{
    fprintf(stderr, "Init\n");
    return 0;
}

static void
cleanupFunc(void *data)
{
    fprintf(stderr, "Cleanup:  Data = %p\n", data);
}

}

//
// Installation function
//
using namespace HDK_Sample;
void
newVEXOp(void *)
{
    //
    //		Returns a random number based on the seed passed in
    new VEX_VexOp("getpid@&I"_sh, pid_Evaluate,
		VEX_OP_CONTEXT|VEX_SHADING_CONTEXT,
		initFunc, cleanupFunc, VEX_OPTIMIZE_1);
    new VEX_VexOp("gettid@&I"_sh, tid_Evaluate,
		VEX_OP_CONTEXT|VEX_SHADING_CONTEXT,
		0, 0, VEX_OPTIMIZE_1);
    new VEX_VexOp("sticky@&I"_sh, printStuff,
		VEX_OP_CONTEXT|VEX_SHADING_CONTEXT,
		0, 0, VEX_OPTIMIZE_0);
    new VEX_VexOp("rcode@&I*F*FFF"_sh, printStuff,
		VEX_OP_CONTEXT|VEX_SHADING_CONTEXT,
		0, 0, VEX_OPTIMIZE_0, true);
    new VEX_VexOp("rcode@&I*F*FS"_sh, printStuff,
		VEX_OP_CONTEXT|VEX_SHADING_CONTEXT,
		0, 0, VEX_OPTIMIZE_0);
}

#endif
