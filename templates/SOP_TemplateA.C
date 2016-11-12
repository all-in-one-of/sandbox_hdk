
#include "SOP_TemplateA.h"

#include <SOP/SOP_Guide.h>
#include <GU/GU_Detail.h>
#include <GA/GA_Iterator.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_Matrix3.h>
#include <UT/UT_Matrix4.h>
#include <UT/UT_Vector3.h>
#include <SYS/SYS_Math.h>
#include <stddef.h>

#include <GU/GU_PrimPoly.h>
#include <GU/GU_PrimTube.h>
#include <GU/GU_PrimMetaBall.h>
#include <GU/GU_PrimNURBCurve.h>

void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        "templateA",
        "TemplateA",
        SOP_TemplateA::myConstructor,
		SOP_TemplateA::myTemplateList,
        1,
        1,
        NULL));
}

PRM_Template
SOP_TemplateA::myTemplateList[] = {
    PRM_Template(),
};

OP_Node *
SOP_TemplateA::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_TemplateA(net, name, op);
}

SOP_TemplateA::SOP_TemplateA(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op)
{
//    mySopFlags.setManagesDataIDs(true);

    mySopFlags.setNeedGuide1(false);
}

SOP_TemplateA::~SOP_TemplateA() {}

void SOP_TemplateA::buildPolygon() {
	GEO_PrimPoly* poly = GU_PrimPoly::build(gdp, 3, false, true);
	GA_Offset poffset;
	poffset = poly->getPointOffset(0);
	gdp->setPos3(poffset, UT_Vector3(1, 0, 0));
	poffset = poly->getPointOffset(1);
	gdp->setPos3(poffset, UT_Vector3(0, 1, 0));
	poffset = poly->getPointOffset(2);
	gdp->setPos3(poffset, UT_Vector3(0, 0, 1));

	UT_Matrix3 mat;
	mat.identity();
//	mat.setTranslates(UT_Vector3(5,0,0));
	mat.scale(UT_Vector3(5,5,5));

//	poly->transform(mat);
	poly->setLocalTransform(mat);
}

void SOP_TemplateA::buildTube() {
	GEO_Primitive* tube = GU_PrimTube::build(GU_PrimTubeParms(gdp),
			GU_CapOptions());

	UT_Matrix4 mat;
	mat.identity();
	mat.setTranslates(UT_Vector3(5,0,0));
//	mat.scale(UT_Vector3(5,5,5));
	mat.rotate(10,20,30,UT_XformOrder(UT_XformOrder::RST,UT_XformOrder::XYZ));
	tube->transform(mat);

}

void SOP_TemplateA::buildMetaBall() {
	GU_PrimMetaBall* metaball = GU_PrimMetaBall::build(
			GU_PrimMetaBallParms(gdp));
}

void SOP_TemplateA::buildNurbsCurve() {
	GU_PrimNURBCurve* nurbs = GU_PrimNURBCurve::build(gdp, 5);
	GA_Offset poffset;
	poffset = nurbs->getPointOffset(0);
	gdp->setPos3(poffset, UT_Vector3(0, 0, 0));
	poffset = nurbs->getPointOffset(1);
	gdp->setPos3(poffset, UT_Vector3(1, 0, -1));
	poffset = nurbs->getPointOffset(2);
	gdp->setPos3(poffset, UT_Vector3(2, 0, 1));
	poffset = nurbs->getPointOffset(3);
	gdp->setPos3(poffset, UT_Vector3(3, 0, -2));
	poffset = nurbs->getPointOffset(4);
	gdp->setPos3(poffset, UT_Vector3(4, 0, 2));
}

OP_ERROR
SOP_TemplateA::cookMySop(OP_Context &context)
{
    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

//    duplicateSource(0, context);
    gdp->clearAndDestroy();

	buildPolygon();
	buildTube();
	buildMetaBall();
	buildNurbsCurve();

    return error();
}

const char *
SOP_TemplateA::inputLabel(unsigned) const
{
    return "Geometry to Analyze";
}
