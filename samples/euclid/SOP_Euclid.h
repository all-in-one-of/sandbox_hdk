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
 * Defines sops that attach euclidean expressions to gdps.
 */

#ifndef __SOP_Euclid_h__
#define __SOP_Euclid_h__

#include <SOP/SOP_Node.h>

#include "EUC_Expression.h"

namespace HDK_Sample {

class SOP_EuclidBase : public SOP_Node
{
protected:
	     SOP_EuclidBase(OP_Network *net, const char *name, OP_Operator *op);
    virtual ~SOP_EuclidBase();

    virtual OP_ERROR		 cookMySop(OP_Context &context);

    virtual EUC_Expression	*cookExpression(OP_Context &context) = 0;

    EUC_Expression		*getInputExpression(int idx) const;

protected:
    bool			 HIDE(fpreal t) { return (bool)evalInt(0, 0, t); }
    fpreal			 CR(fpreal t) { return evalFloat(1, 0, t); }
    fpreal			 CG(fpreal t) { return evalFloat(1, 1, t); }
    fpreal			 CB(fpreal t) { return evalFloat(1, 2, t); }
    
    EUC_Expression		*myExpression;
};

class SOP_EuclidPoint : public SOP_EuclidBase
{
public:
    static OP_Node		*myConstructor(OP_Network*, const char *,
							    OP_Operator *);
    static PRM_Template		 myTemplateList[];
protected:
	     SOP_EuclidPoint(OP_Network *net, const char *name, OP_Operator *op);
	     
    virtual EUC_Expression	*cookExpression(OP_Context &context);

    fpreal			 TX(fpreal t) { return evalFloat(2, 0, t); }
    fpreal			 TY(fpreal t) { return evalFloat(2, 1, t); }
};

class SOP_EuclidPointFromObject : public SOP_EuclidBase
{
public:
    static OP_Node		*myConstructor(OP_Network*, const char *,
							    OP_Operator *);
    static PRM_Template		 myTemplateList[];
protected:
	 SOP_EuclidPointFromObject(OP_Network *net, const char *name, OP_Operator *op);
	     
    virtual EUC_Expression	*cookExpression(OP_Context &context);

    int 			 IDX(fpreal t) { return evalInt(2, 0, t); }
};

class SOP_EuclidLineFromPoints : public SOP_EuclidBase
{
public:
    static OP_Node		*myConstructor(OP_Network*, const char *,
							    OP_Operator *);
    static PRM_Template		 myTemplateList[];
protected:
	 SOP_EuclidLineFromPoints(OP_Network *net, const char *name, OP_Operator *op);
	     
    virtual EUC_Expression	*cookExpression(OP_Context &context);
};

class SOP_EuclidCircleFromPoints : public SOP_EuclidBase
{
public:
    static OP_Node		*myConstructor(OP_Network*, const char *,
							    OP_Operator *);
    static PRM_Template		 myTemplateList[];
protected:
	 SOP_EuclidCircleFromPoints(OP_Network *net, const char *name, OP_Operator *op);
	     
    virtual EUC_Expression	*cookExpression(OP_Context &context);
};

class SOP_EuclidIntersect : public SOP_EuclidBase
{
public:
    static OP_Node		*myConstructor(OP_Network*, const char *,
							    OP_Operator *);
    static PRM_Template		 myTemplateList[];
protected:
	 SOP_EuclidIntersect(OP_Network *net, const char *name, OP_Operator *op);
	     
    virtual EUC_Expression	*cookExpression(OP_Context &context);
};

class SOP_EuclidSelect : public SOP_EuclidBase
{
public:
    static OP_Node		*myConstructor(OP_Network*, const char *,
							    OP_Operator *);
    static PRM_Template		 myTemplateList[];
protected:
	 SOP_EuclidSelect(OP_Network *net, const char *name, OP_Operator *op);
	     
    virtual EUC_Expression	*cookExpression(OP_Context &context);

    int 			 IDX(fpreal t) { return evalInt(2, 0, t); }
};

}

#endif
