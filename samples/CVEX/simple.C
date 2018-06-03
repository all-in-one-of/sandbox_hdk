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
 * Sample of the CVEX interface to call VEX.
 *
 * CVEX is most efficient when operating on arrays of data.  The interface
 * allows you to create a VEX function to perform some computation.  Instead of
 * hard-coding the algorithm in your C++ code, you can alter the algorithm by
 * running different VEX code.  It allows flexibility in design of your code.
 */

#include <CVEX/CVEX_Context.h>
#include <UT/UT_Vector3.h>
#include <MOT/MOT_Director.h>

namespace HDK_Sample {

#define SIZE	32

static void
initializeP(UT_Vector3 *P)
{
    int		i;
    uint	seed = 0;
    for (i = 0; i < SIZE; i++)
	P[i].assign(i/(fpreal)SIZE, SYSfastRandom(seed), 1);
}

static void
dumpLen(const fpreal32 *len)
{
    int		i;
    printf("Length = [");
    for (i = 0; i < SIZE; i++)
    {
	printf("%g,", len[i]);
    }
    printf("]\n");
}

}

using namespace HDK_Sample;

int
main(int argc, char *argv[])
{
    CVEX_Context	cvex;
    CVEX_Value		*P_val, *len_val;
    UT_Vector3		Pbuffer[SIZE];
    int32		seed = 1;
    fpreal32		len[SIZE];

    cvex.addInput("P", CVEX_TYPE_VECTOR3, true);        // Varying value
    cvex.addInput("seed", CVEX_TYPE_INTEGER, &seed, 1); // Uniform value

    // Pass arguments to CVEX
    if (!cvex.load(argc-1, argv+1))
	return 0;

    // Check to see whether VEX function has a "vector P" parameter
    P_val = cvex.findInput("P", CVEX_TYPE_VECTOR3);
    if (P_val)
    {
	initializeP(Pbuffer);           // Set the values for "P"
	P_val->setTypedData(Pbuffer, SIZE);  // "bind" the buffer to the variable
    }
    // Check to see whether VEX function has a "export float len" parameter
    len_val = cvex.findOutput("len", CVEX_TYPE_FLOAT);
    if (len_val)
	len_val->setTypedData(len, SIZE);

    // Run the CVEX program
    cvex.run(SIZE, false);

    if (len_val)
	dumpLen(len);

    return 0;
}
