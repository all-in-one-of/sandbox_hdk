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
 * Defines the expressions that can be performed on sets of
 * euclidean objects.
 */

#include "EUC_Expression.h"

#include <SYS/SYS_Math.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_RootFinder.h>

using namespace HDK_Sample;

EUC_ExpressionList EUC_Expression::ourExpressionList;
int		   EUC_Expression::ourEvaluateTime = 0;

//
// EUC_Expression
// 
EUC_Expression::EUC_Expression()
{
    myRefCount = 0;
    myUid = ourExpressionList.size();
    myLastEvaluateTime = 0;
    myCd = 1;
    myVisible = true;
    ourExpressionList.append(this);
}

EUC_Expression::~EUC_Expression()
{
    // Eliminate ourselves from the expression list.
    ourExpressionList(getUid()) = 0;
}

void
EUC_Expression::addRef()
{
    myRefCount++;
}

void
EUC_Expression::removeRef()
{
    myRefCount--;
    if (myRefCount <= 0)
	delete this;
}

EUC_Expression *
EUC_Expression::getExprFromUid(int uid)
{
    // Out of bound is not created.
    if (uid < 0 || uid >= ourExpressionList.size())
	return 0;

    return ourExpressionList(uid);
}

void
EUC_Expression::setLook(bool visible, const UT_Vector3 &cd)
{
    myVisible = visible;
    myCd = cd;
}

void
EUC_Expression::applyLook(EUC_Object *obj)
{
    obj->setLook(myVisible, myCd);
}

void
EUC_Expression::evaluate(EUC_ObjectList &results)
{
    // Reset the global evaluate time so we clear all the caches.
    ourEvaluateTime++;

    // Call the recursive evaluate.  User wants total number of objects.
    EUC_ObjectList		tmp;

    evaluateRecurse(tmp, results);
}

void
EUC_Expression::evaluateRecurse(EUC_ObjectList &result,
				EUC_ObjectList &totalobj)
{
    // Check to see if we are already cached.
    if (myLastEvaluateTime == ourEvaluateTime)
    {
	result = myObjectCache;
	return;
    }
    // Call subclass & cache.
    myLastEvaluateTime = ourEvaluateTime;
    myObjectCache.setSize(0);
    evaluateSubclass(myObjectCache, totalobj);
    result = myObjectCache;
}

//
// EUC_ExprPoint
//
EUC_ExprPoint::EUC_ExprPoint(const UT_Vector2 &pos) : EUC_Expression()
{
    myPos = pos;
}

void
EUC_ExprPoint::evaluateSubclass(EUC_ObjectList &result,
				EUC_ObjectList &totalobj)
{
    EUC_Point	*pt;

    pt = new EUC_Point(myPos);
    applyLook(pt);
    result.setSize(0);
    result.append(pt);
    totalobj.append(pt);
}

//
// EUC_ExprPointFromObject
//
EUC_ExprPointFromObject::EUC_ExprPointFromObject(EUC_Expression *src, int idx) : EUC_Expression()
{
    src->addRef();
    mySource = src;
    myIndex = idx;
}

EUC_ExprPointFromObject::~EUC_ExprPointFromObject()
{
    mySource->removeRef();
}

void
EUC_ExprPointFromObject::evaluateSubclass(EUC_ObjectList &result,
				EUC_ObjectList &totalobj)
{
    EUC_Point		*pt;
    EUC_ObjectList	 objlist;
    EUC_Object		*obj;
    int			 i, n;
    UT_Vector2		 pos;
    bool		 haspos;

    result.setSize(0);
    mySource->evaluateRecurse(objlist, totalobj);
    n = objlist.size();
    for (i = 0; i < n; i++)
    {
	obj = objlist(i);
	haspos = false;
	switch (obj->getType())
	{
	    case EUC_PointType:
		if (myIndex == 0)
		{
		    haspos = true;
		    pos = ((EUC_Point *)obj)->getPos();
		}
		break;
	    case EUC_LineType:
	    case EUC_CircleType:
		if (myIndex >= 0 && myIndex <= 1)
		{
		    haspos = true;
		    pos = ((EUC_Line *)obj)->getPt(myIndex);
		}
		break;
	}

	if (haspos)
	{
	    pt = new EUC_Point(pos);
	    applyLook(pt);
	    result.append(pt);
	    totalobj.append(pt);
	}
    }
}

//
// EUC_ExprLineFromPoints
//
EUC_ExprLineFromPoints::EUC_ExprLineFromPoints(EUC_Expression *pta, EUC_Expression *ptb) : EUC_Expression()
{
    pta->addRef();
    ptb->addRef();
    myPtA = pta;
    myPtB = ptb;
}

EUC_ExprLineFromPoints::~EUC_ExprLineFromPoints()
{
    myPtA->removeRef();
    myPtB->removeRef();
}

void
EUC_ExprLineFromPoints::evaluateSubclass(EUC_ObjectList &result,
				EUC_ObjectList &totalobj)
{
    EUC_Line		*line;
    EUC_ObjectList	 ptalist, ptblist;
    int			 i, n;
    UT_Vector2		 a, b;

    result.setSize(0);
    myPtA->evaluateRecurse(ptalist, totalobj);
    myPtB->evaluateRecurse(ptblist, totalobj);
    n = SYSmin(ptalist.size(), ptblist.size());
    for (i = 0; i < n; i++)
    {
	if (ptalist(i)->getType() == EUC_PointType &&
	    ptblist(i)->getType() == EUC_PointType)
	{
	    a = ((EUC_Point *)ptalist(i))->getPos();
	    b = ((EUC_Point *)ptblist(i))->getPos();
	    line = new EUC_Line();
	    applyLook(line);
	    line->setPt(0, a);
	    line->setPt(1, b);
	    result.append(line);
	    totalobj.append(line);
	}
    }
}

//
// EUC_ExprCircleFromPoints
//
EUC_ExprCircleFromPoints::EUC_ExprCircleFromPoints(EUC_Expression *center, EUC_Expression *pt) : EUC_Expression()
{
    center->addRef();
    pt->addRef();
    myCenter = center;
    myPoint = pt;
}

EUC_ExprCircleFromPoints::~EUC_ExprCircleFromPoints()
{
    myCenter->removeRef();
    myPoint->removeRef();
}

void
EUC_ExprCircleFromPoints::evaluateSubclass(EUC_ObjectList &result,
				EUC_ObjectList &totalobj)
{
    EUC_Circle		*circle;
    EUC_ObjectList	 ptalist, ptblist;
    int			 i, n;
    UT_Vector2		 a, b;

    result.setSize(0);
    myCenter->evaluateRecurse(ptalist, totalobj);
    myPoint->evaluateRecurse(ptblist, totalobj);
    n = SYSmin(ptalist.size(), ptblist.size());
    for (i = 0; i < n; i++)
    {
	if (ptalist(i)->getType() == EUC_PointType &&
	    ptblist(i)->getType() == EUC_PointType)
	{
	    a = ((EUC_Point *)ptalist(i))->getPos();
	    b = ((EUC_Point *)ptblist(i))->getPos();
	    circle = new EUC_Circle();
	    applyLook(circle);
	    circle->setPt(0, a);
	    circle->setPt(1, b);
	    result.append(circle);
	    totalobj.append(circle);
	}
    }
}

//
// EUC_ExprIntersect
//
EUC_ExprIntersect::EUC_ExprIntersect(EUC_Expression *expra, EUC_Expression *exprb) : EUC_Expression()
{
    expra->addRef();
    exprb->addRef();
    myExprA = expra;
    myExprB = exprb;
}

EUC_ExprIntersect::~EUC_ExprIntersect()
{
    myExprA->removeRef();
    myExprB->removeRef();
}

void
EUC_ExprIntersect::evaluateSubclass(EUC_ObjectList &result,
				EUC_ObjectList &totalobj)
{
    EUC_Point		*pt;
    EUC_ObjectList	 ptalist, ptblist;
    EUC_Object		*obja, *objb;
    int			 i, n;
    UT_Vector2		 pos;

    result.setSize(0);
    myExprA->evaluateRecurse(ptalist, totalobj);
    myExprB->evaluateRecurse(ptblist, totalobj);
    n = SYSmin(ptalist.size(), ptblist.size());
    for (i = 0; i < n; i++)
    {
	// Sort the types so lowest is obja.
	if (ptalist(i)->getType() < ptblist(i)->getType())
	{
	    obja = ptalist(i);
	    objb = ptblist(i);
	}
	else
	{
	    objb = ptalist(i);
	    obja = ptblist(i);
	}

	// This simplifies this double-switch...
	if (obja->getType() == EUC_PointType)
	{
	    // Points never intersect anything.
	    continue;
	}

	if (obja->getType() == EUC_LineType)
	{
	    // Either Line-Line or Line-Circle
	    if (objb->getType() == EUC_LineType)
	    {
		// Line-Line intersection
		UT_Vector3		p1, p2, v1, v2, isect;
		int			retcode;

		pos = ((EUC_Line *)obja)->getPt(0);
		p1.assign(pos.x(), pos.y(), 0);
		pos = ((EUC_Line *)obja)->getPt(1);
		v1.assign(pos.x(), pos.y(), 0);
		v1 -= p1;
		
		pos = ((EUC_Line *)objb)->getPt(0);
		p2.assign(pos.x(), pos.y(), 0);
		pos = ((EUC_Line *)objb)->getPt(1);
		v2.assign(pos.x(), pos.y(), 0);
		v2 -= p2;

		retcode = isect.lineIntersect(p1, v1, p2, v2);
		// Non-parallel lines get a single intersection.
		if (retcode != -1)
		{
		    pos = isect;
		    pt = new EUC_Point(pos);
		    applyLook(pt);
		    result.append(pt);
		    totalobj.append(pt);
		}
	    }
	    else
	    {
		// Line-Circle isect.
		UT_Vector2		p, v, center, isect;
		float			radius, a, b, c, t0, t1;
		
		// x = p.x + v.x * t
		// y = p.y + v.y * t
		// Solve for:
		// (x - c.x)^2 + (y - c.y) ^ 2 = r^2
		// (v.x*t + (p.x - c.x))^2 + (v.y*t + (p.y - c.y))^2 = r^2
		// Let c = p - c
		// (v.x^2+v.y^2)*t^2 
		//   + 2*(v.x*c.x+v.y*c.y)*t 
		//   + (c.x^2+c.y^2-r^2) = 0
		// 

		p = ((EUC_Line *)obja)->getPt(0);
		v = ((EUC_Line *)obja)->getPt(1);
		v -= p;

		center = ((EUC_Circle *)objb)->getCenter();
		radius = ((EUC_Circle *)objb)->getRadius();

		center = p - center;
		a = dot(v, v);
		b = 2 * dot(v, center);
		c = dot(center, center) - radius*radius;
		if (UT_RootFinder::quadratic(a, b, c, t0, t1))
		{
		    // We consider it two intersections.
		    pos = p + t0 * v;
		    pt = new EUC_Point(pos);
		    applyLook(pt);
		    result.append(pt);
		    totalobj.append(pt);

		    pos = p + t1 * v;
		    pt = new EUC_Point(pos);
		    applyLook(pt);
		    result.append(pt);
		    totalobj.append(pt);
		}
	    }
	}
	else
	{
	    // Must be Circle-Circle.
	    // Along the line between the centers, parameterized
	    // with arclength with t, height one one circle is
	    // hA(t)^2 = rA^2 - t^2
	    // Height on the other circle, where distance between them is d,
	    // hB(t)^2 = rB^2 - (t-d)^2
	    // We want hA == hB.  
	    // We thus get
	    // rA^2 - t^2 = rB^2 - t^2 + 2td - d^2
	    // 2td = (rA^2 - rB^2 + d^2)
	    // t = (rA^2 - rB^2 + d^2) / 2d
	    // We can then sanity test t & directly compute the intersection.
	    float		rA, rB, t, d, hA2, hB2;
	    UT_Vector2		cA, cB, v, perpv;

	    rA = ((EUC_Circle *)obja)->getRadius();
	    cA = ((EUC_Circle *)obja)->getCenter();
	    rB = ((EUC_Circle *)objb)->getRadius();
	    cB = ((EUC_Circle *)objb)->getCenter();
	    v = cB;
	    v -= cA;
	    d = v.length();
	    v.normalize();
	    if (!SYSequalZero(d))
	    {
		// Non-zero difference, a chance for intersection.
		t = (rA*rA - rB*rB + d*d) / (2.0 * d);

		// Make sure neither of the squared heights are negative.
		hA2 = rA*rA - t*t;
		hB2 = rB*rB - (t-d)*(t-d);
		if (hA2 >= 0 && hB2 >= 0)
		{
		    // Get the perpindicular
		    perpv.x() = v.y();
		    perpv.y() = -v.x();

		    hA2 = SYSsqrt(hA2);
		    
		    // Compute our new positions.
		    pos = cA + t * v + hA2 * perpv;
		    pt = new EUC_Point(pos);
		    applyLook(pt);
		    result.append(pt);
		    totalobj.append(pt);

		    pos = cA + t * v - hA2 * perpv;
		    pt = new EUC_Point(pos);
		    applyLook(pt);
		    result.append(pt);
		    totalobj.append(pt);
		}
	    }
	}
    }
}

//
// EUC_ExprSelect
//
EUC_ExprSelect::EUC_ExprSelect(EUC_Expression *src, int idx) : EUC_Expression()
{
    src->addRef();
    mySource = src;
    myIndex = idx;
}

EUC_ExprSelect::~EUC_ExprSelect()
{
    mySource->removeRef();
}

void
EUC_ExprSelect::evaluateSubclass(EUC_ObjectList &result,
				EUC_ObjectList &totalobj)
{
    EUC_ObjectList	 objlist;

    result.setSize(0);
    mySource->evaluateRecurse(objlist, totalobj);

    if (myIndex >= 0 && myIndex < objlist.size())
	result.append(objlist(myIndex));
}

