/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* typechk.c,v 2.6 1997/08/29 21:02:09 dmoore Exp */
#include "config.h"

#include "db.h"
#include "code.h"
#include "typechk.h"
#include "externs.h"

/* #### support for primitives taking a list input is not working. */

#ifdef __GNUC__
#define MUCK_INLINE __inline__
#else
#define MUCK_INLINE
#endif

static const char *typecheck_list(frame *fr, data_stack *st, const int **types, int *pos, int depth);

/* pos and types are based by reference so we can move them along as needed. */
MUCK_INLINE
static const char *typechk_arg(frame *fr, data_stack *st, const int **types, int *pos)
{
    inst *arg;
    int type;

#if 0
    /* now in check_prim_space in inst.c */
    if (*pos >= depth_stack(st, DATA_STACK_SIZE))
	return "Data stack underflow";
#endif

    arg = st->top + *pos;
    (*pos)++;

    type = **types;
    (*types)++;

    switch (type) {
    case INST_T_ANY:
	break;
    case INST_T_a:
	if (arg->type != INST_ADDRESS) return "Address expected";
	break;
    case INST_T_c:
	if (arg->type != INST_CONNECTION) return "Connection expected";
	break;
    case INST_T_n:
	if (arg->type != INST_INTEGER) return "Integer expected";
	break;
    case INST_T_N:
	if ((arg->type != INST_INTEGER) || (arg->un.integer <= 0))
	    return "Non-zero integer expected";
	break;
    case INST_T_o: 
	if (arg->type != INST_OBJECT) return "Dbref expected";
	break;
    case INST_T_O:
	if ((arg->type != INST_OBJECT) || Garbage(arg->un.object))
	    return "Valid dbref expected";
	break;
    case INST_T_Oh:
	if ((arg->type != INST_OBJECT) || ((arg->un.object != HOME)
					   && Garbage(arg->un.object)))
	    return "Valid dbref or HOME expected";
	break;
    case INST_T_P:
	if ((arg->type != INST_OBJECT) || Garbage(arg->un.object))
	    return "Valid dbref expected";
	if (!controls(frame_euid(fr), arg->un.object, frame_wizzed(fr)))
	    return "Permission denied";
	break;
    case INST_T_s:
	if (arg->type != INST_STRING) return "String expected";
	break;
    case INST_T_S:
	if ((arg->type != INST_STRING) || (arg->un.string == NULL))
	    return "Non-empty string expected";
	break;
    case INST_T_v:
	if (arg->type != INST_VARIABLE) return "Variable expected";
	break;
    case INST_T_LIST:
        if ((arg->type != INST_INTEGER) || (arg->un.integer < 0))
	    return "List expected";
        return typecheck_list(fr, st, types, pos, arg->un.integer);
    default:
        log_status("INTERP: Illegal type to typecheck_arg: (%i)", type);
	return "Fatal internal error.";
    }
    
    return NULL;
}


static const char *typecheck_list(frame *fr, data_stack *st, const int **types, int *pos, int depth)
{
    int i;
    const char *error = NULL;
    static Buffer buf;
    static Buffer buf2;

    /* #### i think this is broken.  it'll keep incrementing types when
       it should be keeping types fixed. */
    for (i = 0; i < depth; i++) {
        error = typechk_arg(fr, st, types, pos);
	if (error) break;
    }
	    
    if (error) {
        /* Have it work right for lists w/in lists, etc. */
        Bufsprint(&buf2, "Bad arg (%i) in list: %s", i, error);
	Bufcpy(&buf, Buftext(&buf2));
	return Buftext(&buf);
    }

    return NULL;
}


/* Return the total number of items being popped. */
int typechk_stack(frame *fr, data_stack *st, const int prim, const int total_args, const int *types)
{
    int curr_arg;
    int pos;
    const char *error = NULL;

    pos = 0;
    for (curr_arg = 1; curr_arg <= total_args; curr_arg++) {
	error = typechk_arg(fr, st, &types, &pos);
	if (error) break;
    }

    if (error) {
	interp_error(fr, prim, "Bad arg (%i): %s", curr_arg, error);
	return -1;
    }

    return pos;
}
