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

#ifndef __EUC_Expression__
#define __EUC_Expression__

#include "EUC_Object.h"

#include <UT/UT_ValArray.h>

namespace HDK_Sample {

class EUC_Expression;
typedef UT_ValArray<EUC_Expression *> EUC_ExpressionList;

// Generic expression.
class EUC_Expression
{
public:
	     EUC_Expression();
protected:
    virtual ~EUC_Expression();
public:
    // This is a refence counted object.
    void	addRef();
    void	removeRef();

    // Each expression gets a global id which is constant for this
    // runtime invocation.  This lets one marshal an expression pointer
    // as an id and safely determine if it still exists.
    int		getUid() const { return myUid; }
    static EUC_Expression	*getExprFromUid(int uid);

    void	applyLook(EUC_Object *obj);
    void	setLook(bool visible, const UT_Vector3 &cd);

    // Evaluates the set of rules given, generating a bunch of
    // EUC_Objects.
    void	evaluate(EUC_ObjectList &results);
    
protected:
    // Evaluates this expression.  Initializes result to the output
    // of this expression.  totalobj gets any net-new objects
    // added to it.
    virtual void	evaluateSubclass(EUC_ObjectList &result,
					 EUC_ObjectList &totalobj) = 0;

public:
    // Handles the recursive evaluation including caching the results
    // so we don't evaluate an expression twice in a single invocation.
    void		evaluateRecurse(EUC_ObjectList &result,
					EUC_ObjectList &totalobj);
private:
    int		myRefCount;
    int		myUid;

    // Our look for net-new objects
    bool	myVisible;
    UT_Vector3  myCd;

    // Global list of expressions
    static EUC_ExpressionList	ourExpressionList;

    // Our cache of results
    EUC_ObjectList	myObjectCache;
    int		myLastEvaluateTime;
    static int  ourEvaluateTime;
};

// Creates a point.
class EUC_ExprPoint : public EUC_Expression
{
public:
	    EUC_ExprPoint(const UT_Vector2 &pos);
	    
protected:
    virtual void	evaluateSubclass(EUC_ObjectList &result,
					 EUC_ObjectList &totalobj);
protected:
    UT_Vector2		myPos;
};

// Creates a point from an object, extracting it as necessary.
class EUC_ExprPointFromObject : public EUC_Expression
{
public:
	     EUC_ExprPointFromObject(EUC_Expression *src, int idx);
    virtual ~EUC_ExprPointFromObject();
	    
protected:
    virtual void	evaluateSubclass(EUC_ObjectList &result,
					 EUC_ObjectList &totalobj);
protected:
    EUC_Expression	*mySource;
    int			 myIndex;
};

// Creates a line from a pair of points
class EUC_ExprLineFromPoints : public EUC_Expression
{
public:
	     EUC_ExprLineFromPoints(EUC_Expression *pta, EUC_Expression *ptb);
    virtual ~EUC_ExprLineFromPoints();
	    
protected:
    virtual void	evaluateSubclass(EUC_ObjectList &result,
					 EUC_ObjectList &totalobj);
protected:
    EUC_Expression	*myPtA, *myPtB;
};

// Creates a circle from a pair of points
class EUC_ExprCircleFromPoints : public EUC_Expression
{
public:
	     EUC_ExprCircleFromPoints(EUC_Expression *center, EUC_Expression *pt);
    virtual ~EUC_ExprCircleFromPoints();
	    
protected:
    virtual void	evaluateSubclass(EUC_ObjectList &result,
					 EUC_ObjectList &totalobj);
protected:
    EUC_Expression	*myCenter, *myPoint;
};

// Intersects two objects.
class EUC_ExprIntersect : public EUC_Expression
{
public:
	     EUC_ExprIntersect(EUC_Expression *expra, EUC_Expression *exprb);
    virtual ~EUC_ExprIntersect();

protected:
    virtual void	evaluateSubclass(EUC_ObjectList &result,
					 EUC_ObjectList &totalobj);
protected:
    EUC_Expression	*myExprA, *myExprB;
};

// Selects on object out of an expressions result list.
class EUC_ExprSelect : public EUC_Expression
{
public:
	     EUC_ExprSelect(EUC_Expression *src, int idx);
    virtual ~EUC_ExprSelect();
	    
protected:
    virtual void	evaluateSubclass(EUC_ObjectList &result,
					 EUC_ObjectList &totalobj);
protected:
    EUC_Expression	*mySource;
    int			 myIndex;
};

}

#endif
