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
 * This example shows how array types can be accessed inside custom vex
 * operators.  The hdksort() function will sort an array object in-place.
 * The hdkdecimate() function shows how the size of an array can be
 * changed, and how string memory is managed with VEX arrays.
 */

#include <UT/UT_DSOVersion.h>
#include <UT/UT_Array.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_Vector4.h>
#include <UT/UT_Matrix3.h>
#include <UT/UT_Matrix4.h>
#include <UT/UT_Assert.h>
#include <VEX/VEX_VexOp.h>

namespace HDK_Sample {

template <typename T>
int compareValues(const T *a, const T *b)
{
    return *a > *b;
}

template <>
int compareValues<const char *>(const char * const *a, const char * const *b)
{
    return strcmp(*a, *b);
}

template <>
int compareValues<UT_Vector3>(const UT_Vector3 *a, const UT_Vector3 *b)
{
    return a->length2() > b->length2();
}

template <>
int compareValues<UT_Vector4>(const UT_Vector4 *a, const UT_Vector4 *b)
{
    return a->length2() > b->length2();
}

template <>
int compareValues<UT_Matrix3>(const UT_Matrix3 *a, const UT_Matrix3 *b)
{
    return a->determinant() > b->determinant();
}

template <>
int compareValues<UT_Matrix4>(const UT_Matrix4 *a, const UT_Matrix4 *b)
{
    return a->determinant() > b->determinant();
}

template <typename T>
static void
sort(int argc, void *argv[], void *)
{
    UT_Array<T>	*arr = (UT_Array<T> *)argv[0];

    // This will work with all types (including strings).  Since no strings
    // are created or destroyed by this method, it's not necessary to call
    // VEX_VexOp::stringAlloc() or VEX_VexOp::stringFree() to free them -
    // the pointers are just reordered in the array.
    arr->sort(compareValues<T>);
}

static void
decimate(int argc, void *argv[], void *)
{
    const UT_Array<const char *>	&src = *(UT_Array<const char *> *)argv[1];
    UT_Array<const char *>		&dst = *(UT_Array<const char *> *)argv[0];

    UT_ASSERT(dst.entries() == 0);
    for (int i = 0; i < src.entries()/2; i++)
    {
	// Acquire a new reference to the string for storage in the
	// destination array.
	const char	*str = VEX_VexOp::stringAlloc(src(i*2));

	dst.append(str);
    }
}

}

//
// Installation function
//
using namespace HDK_Sample;
void
newVEXOp(void *)
{
    // Sort the array by value for scalars, length for vectors, or
    // determinant for matrices.
    new VEX_VexOp("hdksort@*[I",	// Signature
		sort<int>);		// Evaluator
    new VEX_VexOp("hdksort@*[F",	// Signature
		sort<float>);		// Evaluator
    new VEX_VexOp("hdksort@*[S",	// Signature
		sort<const char *>);	// Evaluator
    new VEX_VexOp("hdksort@*[V",	// Signature
		sort<UT_Vector3>);	// Evaluator
    new VEX_VexOp("hdksort@*[P",	// Signature
		sort<UT_Vector4>);	// Evaluator
    new VEX_VexOp("hdksort@*[3",	// Signature
		sort<UT_Matrix3>);	// Evaluator
    new VEX_VexOp("hdksort@*[4",	// Signature
		sort<UT_Matrix4>);	// Evaluator

    // Remove every second string in the source array and store the result
    // in the destination.
    new VEX_VexOp("hdkdecimate@&[S[S",	// Signature
		decimate);		// Evaluator
}

