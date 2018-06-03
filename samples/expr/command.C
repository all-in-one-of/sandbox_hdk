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

#include <string.h>
#include <malloc.h>
#include <math.h>
#include <UT/UT_DSOVersion.h>
#include <CMD/CMD_Args.h>
#include <CMD/CMD_Manager.h>

// NOTE: Please consider extending HOM rather than hscript

static void
cmd_args(CMD_Args &args)
{
    // Command callback
    int		i;
    if (args.found('e'))
    {
	args.err() << "Found -e option with: " << args.argp('e') << "\n";
	args.showUsage();
    }
    else
    {
	args.out() << "Arguments:\n";
	for (i = 1; i < args.argc(); i++)
	    args.out() << i << ":  " << args(i) << "\n";
    }
}

void
CMDextendLibrary(CMD_Manager *cman)
{
    // Extend the hscript command language by adding a simple command which
    // prints the arguments.
    cman->installCommand("args",	"e:",	cmd_args);
}
