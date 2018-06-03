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
 */

#include <string.h>
#include <malloc.h>
#include <math.h>
#include <UT/UT_DSOVersion.h>
#include <EXPR/EXPR.h>
#include <EXPR/EX_Vector.h>
#include <OP/OP_Director.h>
#include <OP/OP_Operator.h>
#include <OP/OP_Channels.h>
#include <PRM/PRM_RefId.h>
#include <CH/CH_Support.h>
#include <CMD/CMD_Manager.h>


//
// Helpers for handling operator path dependencies
//

// Add name dependency for first argument
static void
fn_addOpNameDepend1(EV_FUNCTION *func, EV_SYMBOL **argv, void *ref_id)
{
    OP_InterestType type;

    if( argv[0] == NULL )
	return; // The argument is an lvalue (non-const)

    if (func->getUserFlags() & CH_EXPRDATA)
	type = OP_INTEREST_NAMEDATA;
    else
	type = OP_INTEREST_NAME;

    OP_Node::addExprOpDependency(argv[0]->value.sval, *((PRM_RefId *)ref_id),
	    type);
}

static void
fn_changeOpRef1(EV_FUNCTION *func, EV_SYMBOL **argv, char **new_args,
	const char *new_fullpath, const char *old_fullpath,
	const char *old_cwd, const char * /*chan_name*/, 
	const char * /*old_chan_name*/)
{
    if( argv[0] == NULL )
	return; // The argument is an lvalue (non-const)

    OP_Node::changeExprOpRef(argv[0]->value.sval, new_args[0], new_fullpath,
	    old_fullpath, old_cwd);
}

//
// This is a function which will find a node from our current node's
//	location.
//
static OP_Node *
findOp(int thread,
       const char *object, OP_InterestType interest_type = OP_INTEREST_DATA)
{
    OP_InterestRef   eval_ref(OP_InterestRef::EvalChannel, thread);
    OP_Node	    *cwd;		// Where to search from
    OP_Node	    *here;		// Where I currently am
    OP_Node	    *src;

    // Find our current evaluation node
    here = eval_ref.node();
    if (!here) return 0;

    // If there's a full path specification, search from the top
    cwd = (*object == '/') ? OPgetDirector() : here;

    if (*object == '\0' || !strcmp(object, "."))
	 src = (OP_Node *)cwd;
    else src = (OP_Node *)cwd->findNode(object);

    // Check to make sure we found the node.
    if (!src) return 0;

    // We need to be told when certain things happen to the node we just found.
    // For example, if the node gets re-named, we need to be told so that the
    // expression can fix itself.  As well, in many cases, we'll also want to
    // be notified when the data changes.  These types of interests can be
    // found in OP/OP_DataTypes.h
    OP_Node::addExtraInput(eval_ref, OP_InterestRef(*src, interest_type));

    return src;
}

#ifndef EV_START_FN
#define EV_START_FN(name)	 \
	    static void name(EV_FUNCTION *, EV_SYMBOL *result,\
			     EV_SYMBOL **argv, int thread)
#endif // EV_START_FN

// Callback function to evaluate the max of two numbers
EV_START_FN(fn_max)
{
    if (argv[0]->value.fval > argv[1]->value.fval)
	 result->value.fval = argv[0]->value.fval;
    else result->value.fval = argv[1]->value.fval;
}

// Callback function to evaluate the minimum of two numbers
EV_START_FN(fn_min)
{
    if (argv[0]->value.fval > argv[1]->value.fval)
	 result->value.fval = argv[1]->value.fval;
    else result->value.fval = argv[0]->value.fval;
}

// This callback is a little more tricky, it evaluates the type of the
// operator specified. Note that we call it optype_proto() because there's
// already a optype() expression function in Houdini proper.
EV_START_FN(fn_optype_proto)
{
    OP_Node		*node;		// Node specified
    const OP_Operator	*opdef;		// The operator definition

    if (node = findOp(thread, argv[0]->value.sval))
    {
	// Get the operator table definition which stores the type of operator
	opdef = node->getOperator();

	// Set the result to be a duplication of the operator table.  Make
	//	to allocate memory for a string result.
	result->value.sval = strdup(opdef->getName());
    }
    else result->value.sval = 0;	// An empty string
}

EV_START_FN(fn_vectorsum)
{
    ev_Vector	*v0 = (ev_Vector *)argv[0]->value.data;
    ev_Vector	*v1 = (ev_Vector *)argv[1]->value.data;
    ev_Vector	*sum  = (ev_Vector *)result->value.data;

    sum->copy(*v0);
    sum->add(*v1);
}

// A couple of defines to make life a lot easier for us
#define EVF	EV_TYPEFLOAT
#define EVS	EV_TYPESTRING
#define EVV	EV_TYPEVECTOR

static int	floatArgs[] = { EVF, EVF };
static int	stringArgs[] = { EVS };
static int	vectorArgs[] = { EVV, EVV };

static EV_FUNCTION funcTable[] = {
    EV_FUNCTION(0, "max",	    2, EVF,	floatArgs,  fn_max),
    EV_FUNCTION(0, "min",	    2, EVF,	floatArgs,  fn_min),
    EV_FUNCTION(0, "optype_proto",  1, EVS,	stringArgs, fn_optype_proto),
    EV_FUNCTION(0, "vectorsum",	    2, EVV,	vectorArgs, fn_vectorsum),
    EV_FUNCTION(),
};

void
CMDextendLibrary(CMD_Manager *)
{
    int		i;

    for (i = 0; funcTable[i].getName(); i++)
	ev_AddFunction(&funcTable[i]);

    // optype_proto() needs special hooks in order to handle operator path name
    // dependencies
    ev_SetFunctionDependencyCallbacks("optype_proto", fn_addOpNameDepend1,
				      fn_changeOpRef1);
}
