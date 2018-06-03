/*
 * Copyright (c) 2017
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
 * PrimVOP SOP
 */

#include "SOP_PrimVOP.h"

#include <SHOP/SHOP_Node.h>
#include <VOP/VOP_CodeCompilerArgs.h>
#include <VOP/VOP_LanguageContextTypeList.h>
#include <GVEX/GVEX_GeoCommand.h>
#include <GU/GU_Detail.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Channels.h>
#include <OP/OP_Operator.h>
#include <OP/OP_Director.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_Caller.h>
#include <OP/OP_NodeInfoParms.h>
#include <OP/OP_VexFunction.h>
#include <PRM/PRM_DialogScript.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <CVEX/CVEX_Context.h>
#include <CVEX/CVEX_Value.h>
#include <VEX/VEX_Error.h>
#include <UT/UT_Array.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Interrupt.h>

using namespace HDK_Sample;
void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        "hdk_primvop",
        "PrimVOP",
        SOP_PrimVOP::myConstructor,
        SOP_PrimVOP::myTemplateList,
        SOP_PrimVOP::theChildTableName,
        1,
        1,
        VOP_CodeGenerator::theLocalVariables));
}

static PRM_Default scriptDefault(PRM_DialogSopVex, "null");


static PRM_Name names[] = {
    PRM_Name("script",      "Script"),
    PRM_Name("clear",       "Re-load VEX Functions"),
    PRM_Name("autobind",    "Autobind by Name"),
    PRM_Name("bindings",    "Number of Bindings"),
    PRM_Name("shoppath",    "Shop Path"),
    PRM_Name("vexsrc",      "Vex Source"),
};

static PRM_Name vexsrcNames[] =
{
    PRM_Name("myself",  "Myself"),
    PRM_Name("shop",    "Shop"),
    PRM_Name("script",  "Script"),
    PRM_Name(0)
};
static PRM_ChoiceList vexsrcMenu(PRM_CHOICELIST_SINGLE, vexsrcNames);

const char *SOP_PrimVOP::theChildTableName = VOP_TABLE_NAME;

PRM_Template
SOP_PrimVOP::myTemplateList[]=
{
    PRM_Template(PRM_ORD,	PRM_Template::PRM_EXPORT_MAX, 1,
			    &names[5], 0, &vexsrcMenu),
    PRM_Template(PRM_STRING, PRM_TYPE_DYNAMIC_PATH,
			    PRM_Template::PRM_EXPORT_MAX, 1,
			    &names[4], 0, 0, 0, 0,
			    &PRM_SpareData::shopCVEX),
    PRM_Template(PRM_COMMAND,	PRM_Template::PRM_EXPORT_TBX,
				1, &names[0], &scriptDefault),
    PRM_Template(PRM_STRING,	1, &VOP_CodeGenerator::theVopCompilerName,
				&VOP_CodeGenerator::theVopCompilerVexDefault),
    PRM_Template(PRM_CALLBACK,	1, &VOP_CodeGenerator::theVopForceCompileName,
				0, 0, 0, VOP_CodeGenerator::forceCompile),
    PRM_Template()
};


OP_Node *
SOP_PrimVOP::myConstructor(OP_Network *net,const char *name,OP_Operator *entry)
{
    return new SOP_PrimVOP(net, name, entry);
}


SOP_PrimVOP::SOP_PrimVOP(OP_Network *net, const char *name, OP_Operator *entry)
    : SOP_Node(net, name, entry),
        // Set up our code generator for CVEX
        myCodeGenerator(this, 
	    new VOP_LanguageContextTypeList(VOP_LANGUAGE_VEX, VOP_CVEX_SHADER),
	    1, 1)
{
    // This indicates that this SOP manually manages its data IDs,
    // so that Houdini can identify what attributes may have changed,
    // e.g. to reduce work for the viewport, or other SOPs that
    // check whether data IDs have changed.
    // By default, (i.e. if this line weren't here), all data IDs
    // would be bumped after the SOP cook, to indicate that
    // everything might have changed.
    // If some data IDs don't get bumped properly, the viewport
    // may not update, or SOPs that check data IDs
    // may not cook correctly, so be *very* careful!
    mySopFlags.setManagesDataIDs(true);
}

SOP_PrimVOP::~SOP_PrimVOP()
{
}


OP_ERROR
SOP_PrimVOP::cookMySop(OP_Context &context)
{
    fpreal t = context.getTime();

    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    duplicateSource(0, context);

    // Build our VEX script, either from the .vex file, or
    // from the SHOP, or from the contained VOPs.
    UT_String script;
    buildScript(script, context.getTime());

    // If script is null, default to null script.
    if (!script.isstring())
    {
        script = "null";
        addWarning(SOP_VEX_ERROR,
                   "No script specified. Using null.");
    }

    // Parse the script's parameters.
    char *argv[4096];
    int argc = script.parse(argv, 4096);

    UT_AutoInterrupt boss("Executing Volume VEX");

    // This op caller is what allows op: style paths inside
    // the CVEX context to properly set up dependencies to ourselves
    // Ie, if you refer to a point cloud on another SOP, this
    // SOP needs to recook when the referred to SOP is coooked.
    OP_Caller caller(this);

    // Actually process the vex function
    executeVex(argc, argv, t, caller);

    return error();
}

void
SOP_PrimVOP::executeVex(int argc, char **argv,
			fpreal t,
			OP_Caller &opcaller)
{
    CVEX_Context context;
    CVEX_RunData rundata;

    // Note that this is a reasonable level to multithread at.
    // Each thread will build its own VEX Context independently
    // and thus, provided the read/write code is properly threadsafe
    // be threadsafe.

    // Set the eval collection scope
    CH_AutoEvaluateTime scope(*CHgetManager(), SYSgetSTID(), t, getChannels());

    // The vex processing is block based.  We first marshall a block
    // of parameters from our primitive information.  We then bind
    // those parameters to vex.  Then we process vex, and read out
    // the new values.
    const int chunksize = 1024;
    UT_Array<int> primid(chunksize, chunksize);
    UT_Array<exint> procid(chunksize, chunksize);

    // Set the callback.
    rundata.setOpCaller(&opcaller);

    // If multithreading, each thread should allocate its own
    // geocmd queue.
    VEX_GeoCommandQueue geocmd;

    // In order to sort the resulting queue edits, we have to have
    // a global order for all vex processors.
    rundata.setProcId(procid.array());

    // These numbers are to seed the queue so it knows where to put
    // newly created primitive/point numbers.
    geocmd.myNumPrim = gdp->getNumPrimitives();
    geocmd.myNumVertex = gdp->getNumVertices();
    geocmd.myNumPoint = gdp->getNumPoints();

    rundata.setGeoCommandQueue(&geocmd);

    int n = 0;
    GEO_Primitive *prim;
    GA_FOR_ALL_PRIMITIVES(gdp, prim)
    {
	primid(n) = (int)prim->getMapIndex();
	procid(n) = (exint)prim->getMapIndex();

	n++;

	if (n >= chunksize)
	{
	    processVexBlock(context, rundata, argc, argv, primid.array(), n, t);
	    n = 0;
	}
    }

    // Handle any trailing values.
    if (n)
	processVexBlock(context, rundata, argc, argv, primid.array(), n, t);

    // If multithreaded, the following is done *after*
    // all threads have joined.
    // One does a fast merge like this:
/*
    GVEX_GeoCommand	allcmd;
    for (int i = 0; i < numthreads; i++)
    {
	allcmd.appendQueue( threadcmds[i] );
    }
    allcmd.apply(gdp);
*/
    // Note that merging will steal data from the various thread queues,
    // so the thread queues must be kept around until the application
    // is complete.

    // In this example we are just doing single threading so we
    // have but a single queue:
    GVEX_GeoCommand	allcmd;
    allcmd.appendQueue(geocmd);

    // NOTE: This manages data IDs for any modifications it does.
    allcmd.apply(gdp);

    if (context.getVexErrors().isstring())
	addError(SOP_VEX_ERROR, (const char *)context.getVexErrors());
    if (context.getVexWarnings().isstring())
	addWarning(SOP_VEX_ERROR, (const char *)context.getVexWarnings());
}

namespace HDK_Sample {
class sop_bindparms
{
public:
    const static int NUM_BUFFERS = 2;
    const static int INPUT_BUFFER = 0;
    const static int OUTPUT_BUFFER = 1;

    sop_bindparms()
    {
	clear();
    }
    sop_bindparms(const char *name, CVEX_Type type)
    {
	clear();
	myName.harden(name);
	myType = type;
    }
    sop_bindparms(const sop_bindparms &src)
    {
	clear();
	*this = src;
    }

    void clear()
    {
	myType = CVEX_TYPE_INVALID;
	for (int i = 0; i < NUM_BUFFERS; i++)
	{
	    myBuffer[i] = 0;
	    myBufLen[i] = 0;
	}
    }

    sop_bindparms &operator=(const sop_bindparms &src)
    {
	myName.harden(src.name());
	myType = src.type();

	for (int i = 0; i < NUM_BUFFERS; i++)
	{
	    delete [] myBuffer[i];
	    myBufLen[i] = src.myBufLen[i];
	    myBuffer[i] = 0;
	    if (src.buffer(i) && src.myBufLen[i])
	    {
		myBuffer[i] = new char[myBufLen[i]];
		memcpy(myBuffer[i], src.buffer(i), src.myBufLen[i]);
	    }
	}

	return *this;
    }

    ~sop_bindparms()
    {
	for (int i = 0; i < NUM_BUFFERS; i++)
	{
	    delete [] myBuffer[i];
	}
    }

    void allocateBuffer(int bufnum, int n)
    {
	delete [] myBuffer[bufnum];
	switch (myType)
	{
	    case CVEX_TYPE_INTEGER:
		myBuffer[bufnum] = new char [sizeof(int) * n];
		break;
	    case CVEX_TYPE_FLOAT:
		myBuffer[bufnum] = new char [sizeof(float) * n];
		break;
	    case CVEX_TYPE_VECTOR3:
		myBuffer[bufnum] = new char [3*sizeof(float) * n];
		break;
	    case CVEX_TYPE_VECTOR4:
		myBuffer[bufnum] = new char [4*sizeof(float) * n];
		break;
	    default:
		UT_ASSERT(0);
		break;
	}
	myBufLen[bufnum] = n;
    }

    void marshallIntoBuffer(int bufnum, GU_Detail *gdp, int *primid, int n)
    {
        allocateBuffer(bufnum, n);

        const GA_Attribute *attrib = gdp->findPrimitiveAttribute(name());
        if (attrib)
        {
            switch (myType)
            {
                case CVEX_TYPE_INTEGER:
                {
                    GA_ROHandleI handle(attrib);
                    for (int i = 0; i < n; i++)
                    {
                        GA_Offset primoff = gdp->primitiveOffset(GA_Index(primid[i]));
                        ((int *)myBuffer[bufnum])[i] = handle.get(primoff);
                    }
                    break;
                }
                case CVEX_TYPE_FLOAT:
                {
                    GA_ROHandleF handle(attrib);
                    for (int i = 0; i < n; i++)
                    {
                        GA_Offset primoff = gdp->primitiveOffset(GA_Index(primid[i]));
                        ((float *)myBuffer[bufnum])[i] = handle.get(primoff);
                    }
                    break;
                }
                case CVEX_TYPE_VECTOR3:
                {
                    GA_ROHandleV3 handle(attrib);
                    for (int i = 0; i < n; i++)
                    {
                        GA_Offset primoff = gdp->primitiveOffset(GA_Index(primid[i]));
                        ((UT_Vector3 *)myBuffer[bufnum])[i] = handle.get(primoff);
                    }
                    break;
                }
                case CVEX_TYPE_VECTOR4:
                {
                    GA_ROHandleV4 handle(attrib);
                    for (int i = 0; i < n; i++)
                    {
                        GA_Offset primoff = gdp->primitiveOffset(GA_Index(primid[i]));
                        ((UT_Vector4 *)myBuffer[bufnum])[i] = handle.get(primoff);
                    }
                    break;
                }
                default:
                {
                    UT_ASSERT(0);
                    break;
                }
            }
        }
    }

    void marshallDataToGdp(int bufnum, GU_Detail *gdp, int *primid, int n, int inc)
    {
        GA_Attribute *attrib = gdp->findPrimitiveAttribute(name());
        if (attrib)
        {
            switch (myType)
            {
                case CVEX_TYPE_INTEGER:
                {
                    GA_RWHandleI handle(attrib);
                    for (int i = 0, src = 0; i < n; i++, src += inc)
                    {
                        GA_Offset primoff = gdp->primitiveOffset(GA_Index(primid[i]));
                        handle.set(primoff, ((int *)myBuffer[bufnum])[src]);
                    }
                    break;
                }
                case CVEX_TYPE_FLOAT:
                {
                    GA_RWHandleF handle(attrib);
                    for (int i = 0, src = 0; i < n; i++, src += inc)
                    {
                        GA_Offset primoff = gdp->primitiveOffset(GA_Index(primid[i]));
                        handle.set(primoff, ((float *)myBuffer[bufnum])[src]);
                    }
                    break;
                }
                case CVEX_TYPE_VECTOR3:
                {
                    GA_RWHandleV3 handle(attrib);
                    for (int i = 0, src = 0; i < n; i++, src += inc)
                    {
                        GA_Offset primoff = gdp->primitiveOffset(GA_Index(primid[i]));
                        handle.set(primoff, ((UT_Vector3 *)myBuffer[bufnum])[src]);
                    }
                    break;
                }
                case CVEX_TYPE_VECTOR4:
                {
                    GA_RWHandleV4 handle(attrib);
                    for (int i = 0, src = 0; i < n; i++, src += inc)
                    {
                        GA_Offset primoff = gdp->primitiveOffset(GA_Index(primid[i]));
                        handle.set(primoff, ((UT_Vector4 *)myBuffer[bufnum])[src]);
                    }
                    break;
                }
                default:
                {
                    UT_ASSERT(0);
                    break;
                }
            }
            // We've modified the attribute, so its data ID should be bumped.
            attrib->bumpDataId();
        }
    }

    const char *name() const { return myName; };
    CVEX_Type   type() const { return myType; }
    const char	*buffer(int bufnum) const { return myBuffer[bufnum]; }
    char	*buffer(int bufnum) { return myBuffer[bufnum]; }

private:
    UT_String		myName;
    CVEX_Type		myType;
    char		*myBuffer[NUM_BUFFERS];
    int			myBufLen[NUM_BUFFERS];
};
}

void
SOP_PrimVOP::processVexBlock(CVEX_Context &context,
			    CVEX_RunData &rundata,
			    int argc, char **argv, 
			    int *primid, int n,
			    fpreal t)
{
    // We always export our primitive ids, so bind as integer.
    context.addInput("primid", CVEX_TYPE_INTEGER, primid, n);

    // These are lazy evaluated since we don't want to pay
    // the cost if not needed.
    context.addInput("P", 			// Name of parameter
		     CVEX_TYPE_VECTOR3, 	// VEX Type
		     true);			// Is varying?

    // We lazy add our time dependent inputs as we only want to
    // flag time dependent if they are used.
    context.addInput("Time", CVEX_TYPE_FLOAT, false);
    context.addInput("TimeInc", CVEX_TYPE_FLOAT, false);
    context.addInput("Frame", CVEX_TYPE_FLOAT, false);

    // Lazily bind all of our primitive attributes.
    UT_Array<sop_bindparms> bindlist;
    for (GA_AttributeDict::iterator it = gdp->primitiveAttribs().begin();
	 !it.atEnd();
	 ++it)
    {
	CVEX_Type		type = CVEX_TYPE_INVALID;

	GA_Attribute *attrib = it.attrib();
	if( attrib->getStorageClass() == GA_STORECLASS_FLOAT &&
	    attrib->getTupleSize() < 3 )
	{
	    type = CVEX_TYPE_FLOAT;
	}
	else if( attrib->getStorageClass() == GA_STORECLASS_FLOAT &&
		 attrib->getTupleSize() < 4 )
	{
	    type = CVEX_TYPE_VECTOR3;
	}
	else if( attrib->getStorageClass() == GA_STORECLASS_FLOAT )
	{
	    type = CVEX_TYPE_VECTOR4;
	}
	else if( attrib->getStorageClass() == GA_STORECLASS_INT )
	{
	    type = CVEX_TYPE_INTEGER;
	}
	else
	{
		type = CVEX_TYPE_INVALID;
	}

	if (type == CVEX_TYPE_INVALID)
	    continue;

	context.addInput(attrib->getName(), type, true);

	bindlist.append( sop_bindparms(attrib->getName(),
					    type) );
    }

    // We want to evaluate at the context time, not at what
    // the global frame time happens to be.
    rundata.setTime(t);

    // Load our array.
    if (!context.load(argc, argv))
	return;

    // Check for lazily bound inputs
    UT_Array<UT_Vector3> P;
    CVEX_Value *var;
    var = context.findInput("P", CVEX_TYPE_VECTOR3);
    if (var)
    {
	P.setSize(n);
	for (int i = 0; i < n; i++)
	{
            GA_Offset primoff = gdp->primitiveOffset(GA_Index(primid[i]));
	    P(i) = gdp->getGEOPrimitive(primoff)->baryCenter();
	}
	var->setTypedData(P.array(), n);
    }

    // Check if any of our parameters exist as either inputs or outputs.
    for (exint j = 0; j < bindlist.entries(); j++)
    {
	var = context.findInput(bindlist(j).name(), bindlist(j).type());
	if (var)
	{
	    // This exists as an input, we have to marshall it.
	    bindlist(j).marshallIntoBuffer(sop_bindparms::INPUT_BUFFER, 
					    gdp, primid, n);
	    void *buf = (void *)bindlist(j).buffer(sop_bindparms::INPUT_BUFFER);
	    var->setRawData(bindlist(j).type(), buf, n);
	}

	// The same attribute may be both an input and an output
	// This results in different CVEX_Values so requires two
	// buffers in the bindings.
	if (var = context.findOutput(bindlist(j).name(), bindlist(j).type()))
	{
	    bindlist(j).allocateBuffer(sop_bindparms::OUTPUT_BUFFER, n);
	    void *buf = (void *)bindlist(j).buffer(sop_bindparms::OUTPUT_BUFFER);
	    if (var->isVarying())
		var->setRawData(bindlist(j).type(), buf, n);
	    else
		var->setRawData(bindlist(j).type(), buf, 1);
	}

    }

    // Compute the time dependent inputs.
    fpreal32 curtime, curtimeinc, curframe;

    var = context.findInput("Time", CVEX_TYPE_FLOAT);
    if (var)
    {
	OP_Node::flags().timeDep = true;

	curtime = t;

	var->setTypedData(&curtime, 1);
    }
    var = context.findInput("TimeInc", CVEX_TYPE_FLOAT);
    if (var)
    {
	OP_Node::flags().timeDep = true;

	curtimeinc = 1.0f/OPgetDirector()->getChannelManager()->getSamplesPerSec();

	var->setTypedData(&curtimeinc, 1);
    }
    var = context.findInput("Frame", CVEX_TYPE_FLOAT);
    if (var)
    {
	OP_Node::flags().timeDep = true;

	curframe = OPgetDirector()->getChannelManager()->getSample(t);

	var->setTypedData(&curframe, 1);
    }

    // clear flag to detect time dependence of ch expressions.
    rundata.setTimeDependent(false);

    // Actually execute the vex code!
    // Allow interrupts.
    context.run(n, true, &rundata);

    // Update our timedependency based on the flag
    if (rundata.isTimeDependent())
	OP_Node::flags().timeDep = 1;

    // Write out all bound parameters.
    for (exint j = 0; j < bindlist.entries(); j++)
    {
	var = context.findOutput(bindlist(j).name(), bindlist(j).type());
	if (var)
	{
	    bindlist(j).marshallDataToGdp(sop_bindparms::OUTPUT_BUFFER, gdp, primid, n, (var->isVarying() ? 1 : 0));
	}
    }
}

void
SOP_PrimVOP::buildScript(UT_String &script, fpreal t)
{
    UT_String		shoppath;
    int			vexsrc = VEXSRC(t);

    script = "";

    switch (vexsrc)
    {
	case 0:
	{
	    getFullPath(shoppath);
	    script = "op:";
	    script += shoppath;
	    // buildVexCommand appends to our script variable.
	    buildVexCommand(script, getSpareParmTemplates(), t);
	    break;
	}

	case 2:
	    // Straightforward, use the explicit script
	    SCRIPT(script, t);
	    break;

	case 1:
	{
	    SHOPPATH(shoppath, t);

	    // Use the referenced shop network.
	    SHOP_Node *shop;
	    shop = findSHOPNode(shoppath);
	    if (shop)
	    {
		shop->buildVexCommand(script, shop->getSpareParmTemplates(), t);
		addExtraInput(shop, OP_INTEREST_DATA);

		// It is possible that we are dealing with a multi-context
		// shader (ie, shop). In that case, we need to specify the
		// shader context which we want to interpret it as. This is done
		// by passing it as an argument. Since this operator invokes
		// CVEX shader, we specify that type. Single-context shaders
		// will ignore it.
		shop->buildShaderString(script, t, 0, 0, 0, SHOP_CVEX);
	    }
	    break;
	}
    }
}

bool
SOP_PrimVOP::evalVariableValue(UT_String &value, int index, int thread)
{
    if (myCodeGenerator.getVariableString(index, value))
	return true;
    // else delegate to base class
    return SOP_Node::evalVariableValue(value, index, thread);
}

OP_OperatorFilter *
SOP_PrimVOP::getOperatorFilter()
{
    return myCodeGenerator.getOperatorFilter();
}

VOP_CodeGenerator *
SOP_PrimVOP::getVopCodeGenerator()
{
    return &myCodeGenerator;
}

bool
SOP_PrimVOP::hasVexShaderParameter(const char *parm_name)
{
    return myCodeGenerator.hasShaderParameter(parm_name);
}

const char *
SOP_PrimVOP::getChildType() const
{
    return VOP_OPTYPE_NAME;
}

OP_OpTypeId
SOP_PrimVOP::getChildTypeID() const
{
    return VOP_OPTYPE_ID;
}

void
SOP_PrimVOP::opChanged(OP_EventType reason, void *data)
{
    int update_id = myCodeGenerator.beginUpdate();
    SOP_Node::opChanged(reason, data);
    myCodeGenerator.ownerChanged(reason, data);
    myCodeGenerator.endUpdate(update_id);
}

void
SOP_PrimVOP::finishedLoadingNetwork(bool is_child_call)
{
    myCodeGenerator.ownerFinishedLoadingNetwork();
    SOP_Node::finishedLoadingNetwork(is_child_call);
}

void
SOP_PrimVOP::addNode(OP_Node *node, int notify, int explicitly)
{
    myCodeGenerator.beforeAddNode(node);
    SOP_Node::addNode(node, notify, explicitly);
    myCodeGenerator.afterAddNode(node);
}

void
SOP_PrimVOP::getNodeSpecificInfoText(OP_Context &context,
				    OP_NodeInfoParms &iparms)
{
    SOP_Node::getNodeSpecificInfoText(context, iparms);

#if 0
    // Compile errors should be already automatically reported in the 
    // node specific info. Still, here is an example of how that can be done
    // explicitly.
    UT_String	    errors;
    if( myCodeGenerator.getCompilerErrors( errors ))
    {
	iparms.appendSeparator();
	iparms.append(errors);
    }
#endif
}

