
#include "SOP_Laplacian_Smooth.h"

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
#include <GU/GU_PrimPacked.h>
#include <GU/GU_PackedGeometry.h>

#include <GU/GU_Smooth.h>

void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        "laplacian_smooth",
        "Laplacian Smooth",
        SOP_Laplacian_Smooth::myConstructor,
		SOP_Laplacian_Smooth::myTemplateList,
        1,
        1,
        NULL));
}

static PRM_Name names[] = {
	PRM_Name("attribute_name", "Attribute Name"),
    PRM_Name("frequency", "Cut-off Frequency"),
    PRM_Name("iterations","Smoothing Iterations"),
};

PRM_Template
SOP_Laplacian_Smooth::myTemplateList[] = {
	    PRM_Template(PRM_STRING,1, &names[0]),
	    PRM_Template(PRM_FLT_J,	1, &names[1], PRMpointOneDefaults, 0,&PRMunitRange),
		PRM_Template(PRM_INT_J,	1, &names[2], PRMoneDefaults, 0,&PRMnonNegativeRange),
    PRM_Template(),
};

OP_Node *
SOP_Laplacian_Smooth::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_Laplacian_Smooth(net, name, op);
}

SOP_Laplacian_Smooth::SOP_Laplacian_Smooth(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op)
{
}

SOP_Laplacian_Smooth::~SOP_Laplacian_Smooth()
{
}

OP_ERROR
SOP_Laplacian_Smooth::cookMySop(OP_Context &context)
{
    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    duplicateSource(0, context);
//    gdp->clearAndDestroy();

    UT_String attrib_name; ATTRIB_NAME(attrib_name);
    GA_Attribute* attrib = gdp->findAttribute(GA_ATTRIB_POINT,attrib_name);
    if(attrib)
    	GUsmooth(attrib,FREQUENCY(0),INTERATIONS(0));

    return error();
}

const char *
SOP_Laplacian_Smooth::inputLabel(unsigned) const
{
    return "Geometry to Analyze";
}
