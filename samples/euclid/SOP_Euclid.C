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

#include "SOP_Euclid.h"

#include <GU/GU_Detail.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>

#include "EUC_Expression.C"
#include "EUC_Object.C"

using namespace HDK_Sample;

void
newSopOperator(OP_OperatorTable *table)
{
     table->addOperator(new OP_Operator("euclidpoint",
					"x Point",
					 SOP_EuclidPoint::myConstructor,
					 SOP_EuclidPoint::myTemplateList,
					 0,
					 0,
					 0,
					 OP_FLAG_GENERATOR));
     table->addOperator(new OP_Operator("euclidpointfromobject",
					"x Point From Object",
					 SOP_EuclidPointFromObject::myConstructor,
					 SOP_EuclidPointFromObject::myTemplateList,
					 1,
					 1,
					 0));
     table->addOperator(new OP_Operator("euclidlinefrompoints",
					"x Line From Points",
					 SOP_EuclidLineFromPoints::myConstructor,
					 SOP_EuclidLineFromPoints::myTemplateList,
					 2,
					 2,
					 0));
     table->addOperator(new OP_Operator("euclidcirclefrompoints",
					"x Circle From Points",
					 SOP_EuclidCircleFromPoints::myConstructor,
					 SOP_EuclidCircleFromPoints::myTemplateList,
					 2,
					 2,
					 0));
     table->addOperator(new OP_Operator("euclidintersect",
					"x Intersect",
					 SOP_EuclidIntersect::myConstructor,
					 SOP_EuclidIntersect::myTemplateList,
					 2,
					 2,
					 0));
     table->addOperator(new OP_Operator("euclidselect",
					"x Select",
					 SOP_EuclidSelect::myConstructor,
					 SOP_EuclidSelect::myTemplateList,
					 1,
					 1,
					 0));
}


static PRM_Name names[] =
{
    PRM_Name("index", 	"Index"),
    PRM_Name("hide",	"Hide"),
    PRM_Name("color",	"Color"),
    PRM_Name(0)
};

//
// SOP_Euclid
//
SOP_EuclidBase::SOP_EuclidBase(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op)
    , myExpression(NULL)
{
    // This SOP always generates fresh geometry, so setting this flag
    // is a bit redundant, but one could change it to check for the old
    // attribute and only bump its data ID if the value changed.
    mySopFlags.setManagesDataIDs(true);
}

SOP_EuclidBase::~SOP_EuclidBase()
{
    if (myExpression)
	myExpression->removeRef();
}

EUC_Expression *
SOP_EuclidBase::getInputExpression(int idx) const
{
    const GU_Detail *igdp = inputGeo(idx);
    GA_ROHandleI attrib(igdp->findIntTuple(GA_ATTRIB_GLOBAL, "euclid", 1));
    if (attrib.isInvalid())
	return 0;

    // NOTE: The detail is *always* at GA_Offset(0)
    int euc = attrib.get(GA_Offset(0));
    EUC_Expression *expr = EUC_Expression::getExprFromUid(euc);
    return expr;
}

OP_ERROR
SOP_EuclidBase::cookMySop(OP_Context &context)
{
    fpreal now = context.getTime();

    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    // Reset our old expression
    if (myExpression)
	myExpression->removeRef();

    //  Get the new expression.
    myExpression = cookExpression(context);
    if (myExpression)
    {
	myExpression->addRef();
	UT_Vector3 cd(CR(now), CG(now), CB(now));
	myExpression->setLook(!HIDE(now), cd);
    }

    // Add to our gdp the signifier.
    // NOTE: This will bump the data IDs for P and the topology attributes.
    gdp->clearAndDestroy();

    // NOTE: This attribute will have a new data ID, because there's no
    //       previous attribute that it might reuse in this case.
    GA_RWHandleI attrib(gdp->addIntTuple(GA_ATTRIB_DETAIL, "euclid", 1, GA_Defaults(0)));

    // NOTE: The detail is *always* at GA_Offset(0)
    if (myExpression)
        attrib.set(GA_Offset(0), myExpression->getUid());
    else
        attrib.set(GA_Offset(0), -1);

    return error();
}

//
// SOP_EuclidPoint
//

PRM_Template
SOP_EuclidPoint::myTemplateList[] = {
    PRM_Template(PRM_TOGGLE,	1, &names[1]),
    PRM_Template(PRM_RGB_J,	3, &names[2], PRMoneDefaults),
    PRM_Template(PRM_XYZ_J,	3, &PRMcenterName),
    PRM_Template()
};

SOP_EuclidPoint::SOP_EuclidPoint(OP_Network *net, const char *name, OP_Operator *op) : SOP_EuclidBase(net, name, op)
{
}

OP_Node *
SOP_EuclidPoint::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_EuclidPoint(net, name, op);
}

EUC_Expression *
SOP_EuclidPoint::cookExpression(OP_Context &context)
{
    UT_Vector2 pt;
    pt.x() = TX(context.getTime());
    pt.y() = TY(context.getTime());

    EUC_Expression *expr = new EUC_ExprPoint(pt);

    return expr;
}

//
// SOP_EuclidPointFromObject
//

PRM_Template
SOP_EuclidPointFromObject::myTemplateList[] = {
    PRM_Template(PRM_TOGGLE,	1, &names[1]),
    PRM_Template(PRM_RGB_J,	3, &names[2], PRMoneDefaults),
    PRM_Template(PRM_INT_J,	1, &names[0]),
    PRM_Template()
};

SOP_EuclidPointFromObject::SOP_EuclidPointFromObject(OP_Network *net, const char *name, OP_Operator *op) : SOP_EuclidBase(net, name, op)
{
}

OP_Node *
SOP_EuclidPointFromObject::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_EuclidPointFromObject(net, name, op);
}

EUC_Expression *
SOP_EuclidPointFromObject::cookExpression(OP_Context &context)
{
    int idx = IDX(context.getTime());

    EUC_Expression *expr = getInputExpression(0);
    if (!expr)
	return 0;
    expr = new EUC_ExprPointFromObject(expr, idx);

    return expr;
}

//
// SOP_EuclidLineFromPoints
//

PRM_Template
SOP_EuclidLineFromPoints::myTemplateList[] = {
    PRM_Template(PRM_TOGGLE,	1, &names[1]),
    PRM_Template(PRM_RGB_J,	3, &names[2], PRMoneDefaults),
    PRM_Template()
};

SOP_EuclidLineFromPoints::SOP_EuclidLineFromPoints(OP_Network *net, const char *name, OP_Operator *op) : SOP_EuclidBase(net, name, op)
{
}

OP_Node *
SOP_EuclidLineFromPoints::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_EuclidLineFromPoints(net, name, op);
}

EUC_Expression *
SOP_EuclidLineFromPoints::cookExpression(OP_Context &/*context*/)
{
    EUC_Expression *expr1 = getInputExpression(0);
    if (!expr1)
	return 0;
    EUC_Expression *expr2 = getInputExpression(1);
    if (!expr2)
	return 0;
    return new EUC_ExprLineFromPoints(expr1, expr2);
}

//
// SOP_EuclidCircleFromPoints
//

PRM_Template
SOP_EuclidCircleFromPoints::myTemplateList[] = {
    PRM_Template(PRM_TOGGLE,	1, &names[1]),
    PRM_Template(PRM_RGB_J,	3, &names[2], PRMoneDefaults),
    PRM_Template()
};

SOP_EuclidCircleFromPoints::SOP_EuclidCircleFromPoints(OP_Network *net, const char *name, OP_Operator *op) : SOP_EuclidBase(net, name, op)
{
}

OP_Node *
SOP_EuclidCircleFromPoints::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_EuclidCircleFromPoints(net, name, op);
}

EUC_Expression *
SOP_EuclidCircleFromPoints::cookExpression(OP_Context &/*context*/)
{
    EUC_Expression *expr1 = getInputExpression(0);
    if (!expr1)
	return 0;
    EUC_Expression *expr2 = getInputExpression(1);
    if (!expr2)
	return 0;
    return new EUC_ExprCircleFromPoints(expr1, expr2);
}

//
// SOP_EuclidIntersect
//

PRM_Template
SOP_EuclidIntersect::myTemplateList[] = {
    PRM_Template(PRM_TOGGLE,	1, &names[1]),
    PRM_Template(PRM_RGB_J,	3, &names[2], PRMoneDefaults),
    PRM_Template()
};

SOP_EuclidIntersect::SOP_EuclidIntersect(OP_Network *net, const char *name, OP_Operator *op) : SOP_EuclidBase(net, name, op)
{
}

OP_Node *
SOP_EuclidIntersect::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_EuclidIntersect(net, name, op);
}

EUC_Expression *
SOP_EuclidIntersect::cookExpression(OP_Context &/*context*/)
{
    EUC_Expression *expr1 = getInputExpression(0);
    if (!expr1)
	return 0;
    EUC_Expression *expr2 = getInputExpression(1);
    if (!expr2)
	return 0;
    return new EUC_ExprIntersect(expr1, expr2);
}

//
// SOP_EuclidSelect
//

PRM_Template
SOP_EuclidSelect::myTemplateList[] = {
    PRM_Template(PRM_TOGGLE,	1, &names[1]),
    PRM_Template(PRM_RGB_J,	3, &names[2], PRMoneDefaults),
    PRM_Template(PRM_INT_J,	1, &names[0]),
    PRM_Template()
};

SOP_EuclidSelect::SOP_EuclidSelect(OP_Network *net, const char *name, OP_Operator *op) : SOP_EuclidBase(net, name, op)
{
}

OP_Node *
SOP_EuclidSelect::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_EuclidSelect(net, name, op);
}

EUC_Expression *
SOP_EuclidSelect::cookExpression(OP_Context &context)
{
    int idx = IDX(context.getTime());

    EUC_Expression *expr = getInputExpression(0);
    if (!expr)
	return 0;
    expr = new EUC_ExprSelect(expr, idx);

    return expr;
}

