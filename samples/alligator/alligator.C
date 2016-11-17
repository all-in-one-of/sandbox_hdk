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

/// Alligator Noise is provided by Side Effects Software Inc. and is licensed
/// under a Creative Commons Attribution-ShareAlike 4.0 International License.
///
/// {
///  "dct:Title"  : "Alligator Noise",
///  "dct:Source" : "http://www.sidefx.com/docs/hdk15.0/alligator_2alligator_8_c-example.html"
///  "license"    : "http://creativecommons.org/licenses/by-sa/4.0/",
///  "cc:attributionName" : "Side Effects Software Inc",
/// }
/// 

///
/// This code is intended to be reference implementation of Houdini's Alligator
/// Noise algorithm.  It is not an optimal implementation by any means.
///
/// @note When using the Houdini's HDK, it's much easier to simply call @code
/// #include <UT/UT_Noise.h>
/// static UT_Noise	alligatorNoise(0, UT_Noise::ALLIGATOR);
/// static double
/// alligator(const UT_Vector3 &pos)
/// {
///    return alligatorNoise.turbulence(pos, 0);
/// }
/// @endcode

#include <math.h>
#include <queue>
#include <boost/functional/hash.hpp>
#include <stdlib.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Vector3.h>
#include <VEX/VEX_VexOp.h>

namespace HDK_Sample {

static double
frandom(std::size_t hash)
{
#if 1
    // For VEX, we need something that produces the same results in a
    // thread-safe fashion (thus random() and even random_r() are not
    // possibilities).
    uint	seed = hash & (0xffffffff);
    return SYSfastRandom(seed);
#else
    srandom(hash);
    return double(random()) * (1.0/RAND_MAX);
#endif
}

static double
hash(int ix, int iy, int iz)
{
    std::size_t	hash = 0;
    boost::hash_combine(hash, ix);
    boost::hash_combine(hash, iy);
    boost::hash_combine(hash, iz);
    return frandom(hash);
}

// These hash functions permute indices differently to make different values
static double hash_1(int ix, int iy, int iz) { return hash(ix, iy, iz); }
static double hash_2(int ix, int iy, int iz) { return hash(iy, iz, ix); }
static double hash_3(int ix, int iy, int iz) { return hash(iz, ix, iy); }
static double hash_4(int ix, int iy, int iz) { return hash(ix, iz, iy); }

static double
rbf(double d)
{
    // Radial basis function
    auto smooth = [](double d) { return d*d*(3 - 2*d); };
    return d < 1 ? smooth(1-d) : 0;
}

static void
getCenter(int ix, int iy, int iz, const int ipos[3], double center[3])
{
    // Compute the center point for the given noise cell
    // hash_* is a function which takes the seeds and returns a random double
    // between 0 and 1.
    center[0] = hash_1(ix+ipos[0], iy+ipos[1], iz+ipos[2]) + ix;
    center[1] = hash_2(ix+ipos[0], iy+ipos[1], iz+ipos[2]) + iy;
    center[2] = hash_3(ix+ipos[0], iy+ipos[1], iz+ipos[2]) + iz;
}

double
noiseValue(int ix, int iy, int iz, const int ipos[3])
{
    return hash_4(ix+ipos[0], iy+ipos[1], iz+ipos[2]);
}

static double
distance(const double *p0, const double *p1)
{
    // Distance from one point to another
    double	sum = 0;
    for (int i = 0; i < 3; ++i)
	sum += (p0[i]-p1[i]) * (p0[i]-p1[i]);
    return sqrt(sum);
}

static double
alligator(const double pos[3])
{
    int				ipos[3]; // Integer coordinates
    double			fpos[3]; // Fractional coordinates
    double			vpos[3]; // Sparse point
    int				idx = 0;
    std::priority_queue<double>	nvals;

    for (int i = 0; i < 3; ++i)
    {
	ipos[i] = floor(pos[i]);
	fpos[i] = pos[i] - ipos[i];
    }
    for (int ix = -1; ix <= 1; ++ix)
    {
	for (int iy = -1; iy <= 1; ++iy)
	{
	    for (int iz = -1; iz <= 1; ++iz, ++idx)
	    {
		getCenter(ix, iy, iz, ipos, vpos);
		double	d = distance(fpos, vpos);
		if (d < 1)
		{
		    // Scale value by noise associated with the point.
		    double	v = noiseValue(ix, iy, iz, ipos) * rbf(d);
		    nvals.push(v);
		}
	    }
	}
    }
    if (!nvals.size())
	return 0;

    double max = nvals.top();
    nvals.pop();
    if (nvals.size())
	max -= nvals.top();
    return max;
}

/// VEX callback to implement: float alligator(vector pos);
static void
alligator_Evaluate(int, void *argv[], void *)
{
    auto result = (float *)argv[0];
    auto fpos = ((const UT_Vector3 *)argv[1])->data();
    double	pos[3];
    std::copy(fpos, fpos+3, pos);
    *result = alligator(pos);
}

}

using namespace HDK_Sample;

void
newVEXOp(void *)
{
    new VEX_VexOp("alligator@&FV",
		    alligator_Evaluate,
		    VEX_ALL_CONTEXT,
		    nullptr,
		    nullptr);
}
