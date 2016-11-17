/*
 * Copyright (c) 2006
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
 * Defines the atomic objects defined in euclidean geometry,
 * the circle, the line, and the point
 */

#include "EUC_Object.h"

using namespace HDK_Sample;

//
// EUC_Object
// 
EUC_Object::EUC_Object()
{
    myCd = 1;
    myVisible = true;
}

EUC_Object::~EUC_Object()
{
}

void
EUC_Object::setLook(bool visible, const UT_Vector3 &cd)
{
    myVisible = visible;
    myCd = cd;
}

//
// EUC_Point
//
EUC_Point::EUC_Point() : EUC_Object()
{
    myPos = 0;
}

EUC_Point::EUC_Point(const UT_Vector2 &pos) : EUC_Object()
{
    myPos = pos;
}

EUC_Point::~EUC_Point()
{
}

//
// EUC_Line
//
EUC_Line::EUC_Line() : EUC_Object()
{
    myPts[0] = 0;
    myPts[1] = 0;
}

EUC_Line::~EUC_Line()
{
}

//
// EUC_Circle
//
EUC_Circle::EUC_Circle() : EUC_Line()
{
}

EUC_Circle::~EUC_Circle()
{
}

float
EUC_Circle::getRadius() const
{
    UT_Vector2		diff;

    diff = myPts[0] - myPts[1];
    return diff.length();
}
