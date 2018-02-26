/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* flow_prims.c,v 2.4 1997/08/29 21:02:01 dmoore Exp */
#include "config.h"

#include "db.h"
#include "code.h"
#include "prim_offsets.h"
#include "typechk.h"
#include "externs.h"

/* This file holds some flow control/context primitives.
   Namely: execute, call, read */


MUF_PRIM(prim_execute)
{
    inst temp;

    pop_stack(curr_data_st, &temp);

    push_context_muf(curr_frame, temp.un.address);
}
 

MUF_PRIM(prim_call)
{
    inst temp;
    dbref prog;

    pop_stack(curr_data_st, &temp);
    prog = temp.un.object;

    if (Typeof(prog) != TYPE_PROGRAM) {
	interp_error(curr_frame, PRIM_call, "Argument not program");
	return;
    }

    if (!Linkable(prog) && !permissions(curr_frame, prog)) {
	interp_error(curr_frame, PRIM_call,
		     "Permission denied to call #%d", prog);
    }

    push_context_dbref(curr_frame, prog);
}


MUF_PRIM(prim_read)
{
    if (curr_frame->writeonly) {
	interp_error(curr_frame, PRIM_read, "Program is write only");
	return;
    }

    up_inst_limit(curr_frame);

    curr_frame->swap = 1;
    SetOnFlag(curr_frame->player, IN_PROGRAM);
}

