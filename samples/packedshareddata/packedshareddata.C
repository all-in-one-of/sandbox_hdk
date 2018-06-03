/*
 * Copyright (c) 2016
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

#include <UT/UT_Args.h>
#include <UT/UT_Options.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPacked.h>

static void
usage(const char *program)
{
    fprintf(stderr, R"(Usage: %s [options]
Options:
  -o file  Save geometry to file  [default: "stdout.geo"]
  -v vlod  Viewport LOD           [default: "full"]
           Choose one of:\n)", program);
    for (int i = 0; i < GEO_VIEWPORT_NUM_MODES; ++i)
	fprintf(stderr, "\t\t- %s\n", GEOviewportLOD((GEO_ViewportLOD)i));
}

int
main(int argc, char *argv[])
{
    // Make sure to install the GU plug-ins
    GU_Detail::loadIODSOs();

    // Process command line arguments
    UT_Args	args;
    args.initialize(argc, argv);
    args.stripOptions("l:o:v:h");

    if (args.found('h'))
    {
	usage(argv[0]);
	return 1;
    }

    const char	*output = args.found('o') ? args.argp('o') : "stdout.geo";
    const char	*viewportLOD = args.found('v') ? args.argp('v') : "full";
    GU_Detail	 gdp;

    // Create a packed shared data primitive
    GU_PrimPacked	*pack = GU_PrimPacked::build(gdp, "PackedSharedData");
    if (!pack)
	fprintf(stderr, "Can't create a packed shared data primitive\n");
    else
    {
	// Set the location of the packed primitive's point.
	UT_Vector3 pivot(0, 0, 0);
	pack->setPivot(pivot);
	gdp.setPos3(pack->getPointOffset(0), pivot);

	pack->setViewportLOD(GEOviewportLOD(viewportLOD));

	// Save the geometry.  With the .so file installed, this should load
	// into Houdini and should be rendered by mantra.
	if (!gdp.save(output, nullptr).success())
	    fprintf(stderr, "Error saving to: %s\n", output);
    }
    return 0;
}
