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

namespace HDK_Sample {

// This is the array length for the sample program.
#define CV_SIZE	8

// Static methods to fill input variables to the VEX function
static void
fillP(UT_Vector3 *P)
{
    int		i;
    fpreal	t;
    for (i = 0; i < CV_SIZE; i++)
    {
	t = (fpreal)i / (CV_SIZE-1);
	P[i].assign(t, SYSsin(t*M_PI), 0);
    }
}

static void
fillN(UT_Vector3 *N)
{
    int		i;
    for (i = 0; i < CV_SIZE; i++)
	N[i].assign(0, 1, 0);
}

static void
fillST(fpreal32 *s, fpreal32 *t)
{
    int		i;
    for (i = 0; i < CV_SIZE; i++)
    {
	s[i] = (fpreal)i / (CV_SIZE-1);
	t[i] = 1-s[i];
    }
}

//
// Convenience methods to print output from the function
//
static void
dumpFloat(const fpreal32 *v, int n)
{
    int		i;

    printf("%g", v[0]);
    for (i = 1; i < n; i++)
	printf(", %g", v[i]);
}

static void
dumpVector(const UT_Vector3 *v, int n)
{
    int		i;
    printf("{%g,%g,%g}", v[0].x(), v[0].y(), v[0].z());
    for (i = 1; i < n; i++)
	printf(", {%g,%g,%g}", v[i].x(), v[i].y(), v[i].z());
}

static void
dumpValue(CVEX_Value *value)
{
    if (!value || !value->isExport())
	return;
    printf("%s = [", (const char *) value->getName());
    switch (value->getType())
    {
	case CVEX_TYPE_FLOAT:
	    dumpFloat((fpreal32 *)value->getRawData(), value->getArraySize());
	    break;
	case CVEX_TYPE_VECTOR3:
	    dumpVector((UT_Vector3 *)value->getRawData(), value->getArraySize());
	    break;
	default:
	    printf("No output supported currently\n");
    }
    printf("]\n");
}

static void
dumpValueList(const char *label, CVEX_ValueList &list)
{
    int		 i;
    CVEX_Value	*value;
    printf("%s:\n", label);
    for (i = 0; i < list.entries(); i++)
    {
	value = list.getValue(i);
	printf("\t%2d.", i+1);
	if (value->isExport())
	    printf("export ");
	switch (value->getType())
	{
	    case CVEX_TYPE_INTEGER:	printf("int"); break;
	    case CVEX_TYPE_FLOAT:	printf("float"); break;
	    case CVEX_TYPE_VECTOR3:	printf("vector"); break;
	    case CVEX_TYPE_VECTOR4:	printf("vector4"); break;
	    case CVEX_TYPE_MATRIX3:	printf("matrix3"); break;
	    case CVEX_TYPE_MATRIX4:	printf("matrix"); break;
	    case CVEX_TYPE_STRING:	printf("string"); break;
	    default:			printf("unknown"); break;
	}
	printf(" %s[%d]\n", (const char *) value->getName(), value->getArraySize());
    }
}
}

using namespace HDK_Sample;

int
main(int argc, char *argv[])
{
    CVEX_Context	cvex;

    // Define storage for the input parameters to the CVEX function.  If the
    // CVEX function accesses these variables by name, we need to make sure
    // they are initialized.  Any parameters which aren't "bound" by inputs,
    // will be handled by the parameters the user specifies.
    UT_Vector3		P[CV_SIZE];
    UT_Vector3		N[CV_SIZE];
    fpreal32		s[CV_SIZE], t[CV_SIZE];
    fpreal32		zero[CV_SIZE];
    int32		seed = 1;
    CVEX_StringArray	map;

    // When parameters are declared as exports, we can grab the data as
    // computed by VEX.  These buffers are used to store the output of the Cf
    // and Of export parameters.
    UT_Vector3		Cf[CV_SIZE];
    UT_Vector3		Of[CV_SIZE];

    // Before we load the VEX function, we need to declare which variables are
    // defined as input parameters.  We can do "lazy" assignment, or
    // pre-computed assignment.  Lazy assignment just declares the variable
    // without specifying its value.
    //
    // Declare P, N, s, and t without computing their values.
    cvex.addInput("P", CVEX_TYPE_VECTOR3, true);
    cvex.addInput("N", CVEX_TYPE_VECTOR3, true);
    cvex.addInput("s", CVEX_TYPE_FLOAT, true);
    cvex.addInput("t", CVEX_TYPE_FLOAT, true);

    //
    // Declare parameters which have values automatically assigned.  This is
    // more of a convenience for simple parameters.
    map.append("Mandril.pic");
    memset(zero, 0, sizeof(zero));
    // seed is a "uniform" variable -- it has a single value
    // zero is a "varying" variable -- it has a different value for each
    //    element of the array.  Well, not actually, but it could.
    // map is a "uniform" variable since there is only one string in the array
    cvex.addInput("seed", CVEX_TYPE_INTEGER, &seed, 1);
    cvex.addInput("zero", CVEX_TYPE_FLOAT, zero, CV_SIZE);
    cvex.addInput("map", map);

    // Load the VEX function
    if (!cvex.load(argc-1, argv+1))
    {
	fprintf(stderr, "Unable to load cvex function: %s\n", argv[1]);
	return 1;
    }

    // Now that the function is loaded, we can find out all sorts of
    // information about the function.
    dumpValueList("Input Parameters", cvex.getInputList());
    dumpValueList("Output Parameters", cvex.getOutputList());

    // Now, we need to initialize the variables we declared.  But we only have
    // to do this if the variable was specified in the VEX function.
    CVEX_Value	*Pval, *Nval, *sval, *tval;
    Pval = cvex.findInput("P", CVEX_TYPE_VECTOR3);
    Nval = cvex.findInput("N", CVEX_TYPE_VECTOR3);
    sval = cvex.findInput("s", CVEX_TYPE_FLOAT);
    tval = cvex.findInput("t", CVEX_TYPE_FLOAT);

    if (Pval)
    {
	fillP(P);			// Initialize P
	Pval->setTypedData(P, CV_SIZE);	// Set the parameter value
    }
    if (Nval)
    {
	fillN(N);
	Nval->setTypedData(N, CV_SIZE);
    }
    if (sval || tval)
    {
	fillST(s, t);
	if (sval) sval->setTypedData(s, CV_SIZE);
	if (tval) tval->setTypedData(t, CV_SIZE);
    }

    // We can also find exported parameters.
    CVEX_Value	*CfVal, *OfVal, *sout, *tout;

    CfVal = cvex.findOutput("Cf", CVEX_TYPE_VECTOR3);
    OfVal = cvex.findOutput("Of", CVEX_TYPE_VECTOR3);

    // When an input parameter (like P, N, s or t) is specified as an export
    // parameter, CVEX will _not_ modify the data buffer assigned to the input
    // value.
    sout = cvex.findOutput("s", CVEX_TYPE_FLOAT);
    tout = cvex.findOutput("t", CVEX_TYPE_FLOAT);

    if (!CfVal && !OfVal)
	fprintf(stderr, "%s doesn't seem to write to Cf or Of\n", argv[1]);

    if (CfVal)
	CfVal->setTypedData(Cf, CV_SIZE);
    if (OfVal)
	OfVal->setTypedData(Of, CV_SIZE);

    // To get the output value, you need to grab the output variable and set
    // its data.  You can think of input variables as being "const" for the VEX
    // function.  You can obviously have VEX write to a different buffer, or it
    // can overwrite the input buffer (like below)
    if (sout)
	sout->setTypedData(s, CV_SIZE);
    if (tout)
	tout->setTypedData(t, CV_SIZE);

    // Now, call VEX
    printf("Calling VEX ----------------------------\n\n");
    cvex.run(CV_SIZE, false);
    printf("\n\nFinished VEX ----------------------------\n");

    // And print out the output values
    dumpValue(CfVal);
    dumpValue(OfVal);
    dumpValue(sout);
    dumpValue(tout);

    return 0;
}
