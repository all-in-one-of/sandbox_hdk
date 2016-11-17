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

#ifndef __EUC_Object__
#define __EUC_Object__

#include <UT/UT_Vector2.h>
#include <UT/UT_ValArray.h>

namespace HDK_Sample {

enum EUC_ObjType
{
    EUC_PointType,
    EUC_LineType,
    EUC_CircleType
};

class EUC_Object
{
public:
	     EUC_Object();
    virtual ~EUC_Object();

    void	setLook(bool visible, const UT_Vector3 &cd);
    bool	getVisible() const { return myVisible; }
    UT_Vector3  getColor() const { return myCd; }

    virtual EUC_ObjType	getType() const = 0;
protected:
    bool		myVisible;
    UT_Vector3		myCd;
};

class EUC_Point : public EUC_Object
{
public:
	     EUC_Point();
	     EUC_Point(const UT_Vector2 &pos);
    virtual ~EUC_Point();

    virtual EUC_ObjType	 getType() const { return EUC_PointType; }

    const UT_Vector2	&getPos() const { return myPos; }
    void		 setPos(const UT_Vector2 &pos) { myPos = pos; }

protected:
    UT_Vector2		myPos;
};

class EUC_Line : public EUC_Object
{
public:
	     EUC_Line();
    virtual ~EUC_Line();

    virtual EUC_ObjType	 getType() const { return EUC_LineType; }

    const UT_Vector2 &getPt(int idx) const { return myPts[idx]; }
    void	      setPt(int idx, const UT_Vector2 &pt) 
			{ myPts[idx] = pt; }
protected:
    UT_Vector2		myPts[2];
};

class EUC_Circle : public EUC_Line
{
public:
	     EUC_Circle();
    virtual ~EUC_Circle();
    
    virtual EUC_ObjType	 getType() const { return EUC_CircleType; }

    // Circle specific methods
    const UT_Vector2 &getCenter() const { return myPts[0]; }
    void	      setCenter(const UT_Vector2 &pt) { myPts[0] = pt; }

    float	     getRadius() const;
};

typedef UT_ValArray<EUC_Object *> EUC_ObjectList;

}

#endif
