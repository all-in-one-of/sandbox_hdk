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

/// @file TS_cosKernel.C
/// @brief Sample metaball kernel function
///
/// This file demonstrates how to add a custom metaball kernel function that
/// will show up in all the Kernel Function parameter menus.
///
/// Once this file is compiled, register this dso into a @c MetaKernels text
/// file that is found in your HOUDINI_PATH environment variable. For example:
/// - <tt>$HOME/Library/Preferences/houdini/X.Y/MetaKernels</tt> <b>(OSX)</b>
/// - <tt>$HOME/houdiniX.Y/MetaKernels</tt> <b>(other platforms)</b>
///
/// The @c MetaKernels file should contain a list of .so/.dll paths that are
/// located relative to the search paths in the HOUDINI_DSO_PATH environment
/// variable. For example:
/// @code
///	# MetaKernels : Kernel extension table
///	TS_MyKernel.so
/// @endcode
/// In this case, you should install the TS_MyKernel.so file into the @c dso
/// subdirectory under where @c MetaKernels is found.
///
#include <UT/UT_DSOVersion.h>
#include <TS/TS_Expression.h>

#include <TS/TS_KernelList.h>

static float
evalPosition(float t)
{
    // Since distance passed in is squared, we must take the root...
    t = sqrt(t);
    return cos(t*M_PI)*.5 + .5;
}

static float
evalGradient(float t)
{
    t = sqrt(t);
    return .5*sin(t*M_PI);
}

static float
cosFunc(float t)
{
    if (t <= 0) return 1;
    if (t >= 1) return 0;
    return evalPosition(t);
}

static UT_Interval
cosFuncRange(const UT_Interval &t)
{
    if (t.max <= 0) return UT_Interval(1, 1);
    if (t.min >= 1) return UT_Interval(0, 0);

    float min = cosFunc(t.max);
    float max = cosFunc(t.min);
    return (min > max) ? UT_Interval(max, min) : UT_Interval(min, max);
}

static float
cosDFunc(float t)
{
    if (t <= 0) return 0;
    if (t >= 1) return 0;
    return evalGradient(t);
}

static UT_Interval
cosDFuncRange(const UT_Interval &t)
{
    if (t.max <= 0 || t.min >= 1) return UT_Interval(0, 0);
    float min = cosDFunc(t.min);
    float max = cosDFunc(t.max);
    return (min > max) ? UT_Interval(max, min) : UT_Interval(min, max);
}

/// The kernel table definition
static TS_MetaKernel cosKernel = {
    "cosine",		// Token
    "Half Cosine",	// Label
    cosFunc,
    cosFuncRange,
    cosDFunc,
    cosDFuncRange
};

/// Registration function which installs the kernel
void
newMetaKernel()
{
    TS_KernelList::getList()->addKernel(&cosKernel);
}

