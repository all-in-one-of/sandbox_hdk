
#include "SOP_TemplateB.h"

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
        "templateB",
        "TemplateB",
        SOP_TemplateB::myConstructor,
		SOP_TemplateB::myTemplateList,
        1,
        1,
        NULL));
}

PRM_Template
SOP_TemplateB::myTemplateList[] = {
    PRM_Template(),
};

OP_Node *
SOP_TemplateB::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_TemplateB(net, name, op);
}

SOP_TemplateB::SOP_TemplateB(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op)
{
//    mySopFlags.setManagesDataIDs(true);

    mySopFlags.setNeedGuide1(false);
}

SOP_TemplateB::~SOP_TemplateB() {}

OP_ERROR
SOP_TemplateB::cookMySop(OP_Context &context)
{
    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    duplicateSource(0, context);


    return error();
}

const char *
SOP_TemplateB::inputLabel(unsigned) const
{
    return "Geometry to Analyze";
}
