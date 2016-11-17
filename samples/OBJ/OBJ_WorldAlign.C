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
 *
 * This class implements an object that aligns the inherited orientation to the
 * world axes while preserving the location in space. It also illustrates how to
 * hide all the parameters inherited from the base class.
 *
 */

#include "OBJ_WorldAlign.h"
#include <UT/UT_DSOVersion.h>
#include <UT/UT_DMatrix3.h>
#include <OP/OP_OperatorTable.h>


using namespace HDK_Sample;



// Constructor for new object class
OBJ_WorldAlign::OBJ_WorldAlign( OP_Network *net, 
				const char *name, OP_Operator *op )
	  : OBJ_Geometry(net, name, op)
{
    // initialize local member data here.
}



// virtual destructor for new object class
OBJ_WorldAlign::~OBJ_WorldAlign()
{
}



// this is the template list that defines the new parameters that are
// used by the worldalign object. In this particular case the list is empty
// because we have no new parameters.
static PRM_Template	templatelist[] =
{
    // blank terminating Template.
    PRM_Template()
};



// this function copies the original parameter and then adds an invisible flag
static void
copyParmWithInvisible(PRM_Template &src, PRM_Template &dest)
{
    PRM_Name *	new_name;
   
    new_name = new PRM_Name(src.getToken(), src.getLabel(),
			    src.getExpressionFlag());
    new_name->harden();
    dest.initialize(
	    (PRM_Type) (src.getType() | PRM_TYPE_INVISIBLE),
	    src.getTypeExtended(),
	    src.exportLevel(),
	    src.getVectorSize(),
	    new_name,
	    src.getFactoryDefaults(),
	    src.getChoiceListPtr(),
	    src.getRangePtr(),
	    src.getCallback(),
	    src.getSparePtr(),
	    src.getParmGroup(),
	    (const char *)src.getHelpText(),
	    src.getConditionalBasePtr());
}



// this function returns the OP_TemplatePair that combines the parameters
// of this object with those of its ancestors in the (object class hierarchy)
OP_TemplatePair     *
OBJ_WorldAlign::buildTemplatePair(OP_TemplatePair *prevstuff)
{
    OP_TemplatePair     *align, *geo;

    // The parm templates here are not created as a static list because
    // if that static list was built before the OBJbaseTemplate static list
    // (which it references) then that list would be corrupt. Thus we have
    // to force our static list to be created after OBJbaseTemplate.
    static PRM_Template *theTemplate = 0;

    if(!theTemplate)
    {
	PRM_Template	*obj_template;
	int		 i;
	int		 size;
	UT_String	 parm_name;

	obj_template = OBJ_Geometry::getTemplateList(OBJ_PARMS_PLAIN);
	size = PRM_Template::countTemplates( obj_template );
	theTemplate = new PRM_Template[size + 1];   // add +1 for sentinel
	for( i = 0; i < size; i++ )
	{
	    theTemplate[i] = obj_template[i];
	    theTemplate[i].getToken( parm_name );

	    // leave only the translation parameter visible (and its containing
	    // switcher)
	    if( parm_name != "t" && parm_name != "stdswitcher" )
		copyParmWithInvisible( obj_template[i], theTemplate[i] );
	}
    }

    // Here, we have to "inherit" template pairs from geometry and beyond. To
    // do this, we first need to instantiate our template list, then add the
    // base class templates.
    align = new OP_TemplatePair(templatelist, prevstuff);
    geo   = new OP_TemplatePair(theTemplate, align);

    return geo;
} 



// the myConstructor method is used to create new objects of the correct
// type from the OperatorTable. This method is passed to the OP_Operator
// constructor and then is invoked whenever a new node needs to be created.
OP_Node *
OBJ_WorldAlign::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new OBJ_WorldAlign(net, name, op);
}



OP_ERROR
OBJ_WorldAlign::cookMyObj(OP_Context &context)
{
    OP_ERROR	errorstatus;
    UT_DMatrix3	rotation;
    UT_DMatrix4	rotation4;

    // OBJ_Geometry::cookMyObj computes the local and global transform, and
    // dirties the inverse of the global transform matrix. These are stored
    // in myXform, myWorldXform, and myIWorldXform, respectively.
    errorstatus = OBJ_Geometry::cookMyObj(context);

    // get rid of the rotation component in the matrices
    myWorldXform.extractRotate(rotation);
    if( ! rotation.invert() )
    {
	rotation4    = rotation;
	myWorldXform = rotation4 * myWorldXform;
	myXform	     = rotation4 * myXform;
    }

    return errorstatus;
}



// this function installs the new object in houdini's object table. 
// It is automatically called by Houdini when this dynamic library is loaded.
void
newObjectOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
		"hdk_worldalign",   // operator unique name
		"World Align",	    // operator label
		OBJ_WorldAlign::myConstructor,	// node instance constructor
		OBJ_WorldAlign::buildTemplatePair(0), // parameters
		0,	// minimum inputs
		1,	// maximum inputs 
		0));	// variables
}
