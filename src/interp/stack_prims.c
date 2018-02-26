/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* stack_prims.c,v 2.7 1997/08/29 21:02:07 dmoore Exp */
#include "config.h"

#include "db.h"
#include "code.h"
#include "prim_offsets.h"
#include "externs.h"

/* This file holds the various pure stack primitives, as well as those
   for manipulating variables.  Namely: pop, not, dup, over, swap, rot
   pick, put, rotate, @, !, variable, and, or */


MUF_PRIM(prim_depth)
{
    inst result;

    result.type = INST_INTEGER;
    result.un.integer = depth_stack(curr_data_st, DATA_STACK_SIZE);

    push_stack(curr_data_st, &result);
}

MUF_PRIM(prim_pop)
{
    inst temp;

    pop_stack(curr_data_st, &temp);
}


MUF_PRIM(prim_not)
{
    inst temp;
    inst result;
    
    pop_stack(curr_data_st, &temp);

    result.type = INST_INTEGER;
    result.un.integer = !true_inst(&temp);

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_dup)
{
    inst what;

    pop_stack(curr_data_st, &what);

    dup_inst_interp(&what);	/* In case of address or string. */
    push_stack(curr_data_st, &what);

    dup_inst_interp(&what);	/* In case of address or string. */
    push_stack(curr_data_st, &what);
}


MUF_PRIM(prim_over)
{
    inst arg1, arg2;

    pop_stack(curr_data_st, &arg2);
    pop_stack(curr_data_st, &arg1);

    dup_inst_interp(&arg1);
    push_stack(curr_data_st, &arg1);
    
    dup_inst_interp(&arg2);
    push_stack(curr_data_st, &arg2);

    dup_inst_interp(&arg1);
    push_stack(curr_data_st, &arg1);
}


MUF_PRIM(prim_swap)
{
    inst arg1, arg2;

    pop_stack(curr_data_st, &arg2);
    pop_stack(curr_data_st, &arg1);
    
    dup_inst_interp(&arg2);
    push_stack(curr_data_st, &arg2);

    dup_inst_interp(&arg1);
    push_stack(curr_data_st, &arg1);
}


/* Yes, 3 rotate is more efficient than rot. ;-) */
MUF_PRIM(prim_rot)
{
    inst arg1, arg2, arg3;

    pop_stack(curr_data_st, &arg3);
    pop_stack(curr_data_st, &arg2);
    pop_stack(curr_data_st, &arg1);
    
    dup_inst_interp(&arg2);
    push_stack(curr_data_st, &arg2);

    dup_inst_interp(&arg3);
    push_stack(curr_data_st, &arg3);

    dup_inst_interp(&arg1);
    push_stack(curr_data_st, &arg1);
}


MUF_PRIM(prim_pick)
{
    inst depth;
    inst *what;

    pop_stack(curr_data_st, &depth);

    if (depth.un.integer > depth_stack(curr_data_st, DATA_STACK_SIZE)) {
	interp_error(curr_frame, PRIM_pick,
		     "%i is larger than stack depth (%i)",
		     depth.un.integer,
		     depth_stack(curr_data_st, DATA_STACK_SIZE));
	return;
    }

    what = curr_data_st->top + depth.un.integer - 1;

    dup_inst_interp(what);
    push_stack(curr_data_st, what);
}
 

MUF_PRIM(prim_put)
{
    inst depth;
    inst what;
    inst *where;

    pop_stack(curr_data_st, &depth);

    if (depth.un.integer >= depth_stack(curr_data_st, DATA_STACK_SIZE)) {
	interp_error(curr_frame, PRIM_put,
		     "%i is larger than stack depth (%i)",
		     depth.un.integer,
		     depth_stack(curr_data_st, DATA_STACK_SIZE));
	return;
    }

    pop_stack(curr_data_st, &what);

    where = curr_data_st->top + depth.un.integer - 1;

    dup_inst_interp(&what);
    *where = what;
}
 

MUF_PRIM(prim_rotate)
{
    inst depth;
    inst temp;
    inst *pos;
    int i;

    pop_stack(curr_data_st, &depth);

    if (depth.un.integer > depth_stack(curr_data_st, DATA_STACK_SIZE)) {
	interp_error(curr_frame, PRIM_rotate,
		     "%i is larger than stack depth (%i)",
		     depth.un.integer,
		     depth_stack(curr_data_st, DATA_STACK_SIZE));
	return;
    }

    if (depth.un.integer == 0) return;

    if (depth.un.integer > 0) {
	depth.un.integer--;	/* Counter is 1 based, adjust. */
	pos = curr_data_st->top + depth.un.integer;
	temp = *pos;
	for (i = 0; i < depth.un.integer; i++, pos--)
	    *pos = *(pos - 1);
	*pos = temp;
    } else {
	depth.un.integer = -depth.un.integer;
	depth.un.integer--;	/* Counter is 1 based, adjust. */
	pos = curr_data_st->top;
	temp = *pos;
	for (i = 0; i < depth.un.integer; i++, pos++)
	    *pos = *(pos + 1);
	*pos = temp;
    }
}


MUF_PRIM(prim_at)
{
    inst which_var;
    inst *contents;
   
    pop_stack(curr_data_st, &which_var);

    contents = &(curr_frame->variables[which_var.un.variable]);
    dup_inst_interp(contents);		/* Might be string or address. */

    push_stack(curr_data_st, contents);
}


MUF_PRIM(prim_bang)
{
    inst which_var;
    inst value;
    inst *var;

    pop_stack(curr_data_st, &which_var);
    pop_stack(curr_data_st, &value);

    var = &(curr_frame->variables[which_var.un.variable]);
    clear_inst_interp(var);	/* Old might have been string/addr. */

    dup_inst_interp(&value);
    *var = value;
}


MUF_PRIM(prim_int)
{
    inst what;

    pop_stack(curr_data_st, &what);

    switch(what.type) {
    case INST_INTEGER:
	break;
    case INST_OBJECT:
	what.un.integer = dbref2long(what.un.object);
	break;
    case INST_VARIABLE:
	what.un.integer = what.un.variable;
	break;
    default:
	interp_error(curr_frame, PRIM_int, "Can't convert to int");
	return;
	break;
    }
    what.type = INST_INTEGER;

    push_stack(curr_data_st, &what);
}


MUF_PRIM(prim_dbref)
{
    inst what;

    pop_stack(curr_data_st, &what);

    what.type = INST_OBJECT;
    what.un.object = long2dbref(what.un.integer);

    push_stack(curr_data_st, &what);
}


MUF_PRIM(prim_variable)
{
    inst which;
    inst result;

    pop_stack(curr_data_st, &which);

    if ((which.un.integer < 0) || (which.un.integer >= MAX_VAR)) {
	interp_error(curr_frame, PRIM_variable, "Out of variable range");
	return;
    }

    result.type = INST_VARIABLE;
    result.un.variable = which.un.integer;

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_and)
{
    inst arg1, arg2;
    inst result;

    pop_stack(curr_data_st, &arg2);
    pop_stack(curr_data_st, &arg1);

    result.type = INST_INTEGER;
    result.un.integer = true_inst(&arg1) && true_inst(&arg2);

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_or)
{
    inst arg1, arg2;
    inst result;

    pop_stack(curr_data_st, &arg2);
    pop_stack(curr_data_st, &arg1);

    result.type = INST_INTEGER;
    result.un.integer = true_inst(&arg1) || true_inst(&arg2);

    push_stack(curr_data_st, &result);
}

