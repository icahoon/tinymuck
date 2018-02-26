/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* interp.c,v 2.16 1997/08/30 06:44:06 dmoore Exp */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "db.h"
#include "params.h"
#include "buffer.h"
#include "code.h"
#include "prim_offsets.h"
#include "externs.h"

#ifndef INTERP_PASS_FRAME
frame *curr_frame;
data_stack *curr_data_st;
#endif

/* Useful things to know.  The top element on the address stack is the
   current function we are in, at the current location in it.  This stack
   starts off with the first main function to be executed, and things go
   from there. */

/* Use safe_push_data_st when you want error checking. If you know
   that the operation will not overflow, then use push_data_st.  */

void safe_push_data_st(frame *fr, data_stack *st, inst *what, const int prim)
{
    if (full_stack(st, DATA_STACK_SIZE)) {
	interp_error(fr, prim, "Data stack overflow");
	/* they expected it to get on the stack, so it's been dup'd, etc */
	clear_inst_interp(what);
	return;
    }
    
    push_stack(st, what);
}


void clear_data_stack_(data_stack *st)
{
    while (!empty_stack(st, DATA_STACK_SIZE)) {
        /* note that clear_inst_interp evals it's arg more than once */
        clear_inst_interp(st->top);
	st->top++;
    }
}

#define clear_data_stack(st) \
    if (!empty_stack(st, DATA_STACK_SIZE)) clear_data_stack_(st)

void clear_frame(frame *fr)
{
    addr_stack *addr_st = &(fr->addr_stack);
    int i;
    continue_func func;
    void *data;
    jump_addr *top;

    clear_data_stack(&(fr->data_stack));

    for (i = 0; i < MAX_VAR; i++)
	clear_inst_interp(&(fr->variables[i]));

    while (!empty_stack(addr_st, ADDR_STACK_SIZE)) {
        top = addr_st->top++;
	if (JumpType(top) == JUMP_CONTINUATION) {
	    func = JumpContinuation(top);
	    data = JumpContData(top);
	    func(NULL, NULL, data);
	} else if (JumpType(top) == JUMP_MUF_FUNC) {
	    clear_code(JumpFunc(top)->code);
	}
    }

    delete_frame(fr->player);
    FREE(fr);
}


void up_inst_limit(frame *fr)
{
    fr->max_instructions = fr->instructions_run + MAX_INSTRUCTIONS;
}


/* This is just for use by the context routines. */
static void push_addr_st(frame *fr, jump_addr *where)
{
    addr_stack *st = &(fr->addr_stack);

    if (full_stack(st, ADDR_STACK_SIZE)) {
	interp_error(fr, PRIM_PUSH, "Address stack overflow");
	return;
    }

    push_stack_nocheck(st, where);
    fr->cache_dirty = 1;
}


void pop_context(frame *fr)
{
    addr_stack *st = &(fr->addr_stack);
    jump_addr old;

    if (empty_stack(st, ADDR_STACK_SIZE)) {
	log_status("INTERP: pop_context called when address stack empty.");
	interp_error(fr, PRIM_NONE, "Fatal internal error.");
	return;
    }

    /* If the item we are going to pop is a muf function, lower the
       refcount. */
    if (JumpType(st->top) == JUMP_MUF_FUNC) {
	clear_code(JumpFunc(st->top)->code);
    }

    pop_stack_nocheck(st, &old);
    fr->cache_dirty = 1;
}


void push_context_muf(frame *fr, func_addr *func)
{
    jump_addr temp;

    JumpType(&temp) = JUMP_MUF_FUNC;
    JumpFunc(&temp) = func;
    JumpCurrInst(&temp) = func->bytecode;
    JumpLineno(&temp) = -1;

    dup_code(func->code);

    push_addr_st(fr, &temp);
}


void push_context_dbref(frame *fr, const dbref prog)
{
    jump_addr temp;

    JumpType(&temp) = JUMP_OBJECT;
    JumpObject(&temp) = prog;

    push_addr_st(fr, &temp);
}


void push_context_cont(frame *fr, continue_func func, void *data)
{
    jump_addr temp;

    JumpType(&temp) = JUMP_CONTINUATION;
    JumpContinuation(&temp) = func;
    JumpContData(&temp) = data;

    push_addr_st(fr, &temp);

    if (fr->error) {
	/* Install failed.  We call the function to have it cleanup. */
	func(NULL, NULL, data);
    }
}


/* Starts up interpreting the given program.  Uses the arg_text as the
   first item on the data stack.  If nothread is true then any calls
   to 'read' or 'sleep' or similar are illegal (causing fatal runtime
   error).  This is used to ensure that locks/desc/succ/etc run to
   completion.  An alternative to this would be to use continuations,
   however it greatly complicates the server for little gain.
   Return value is only meaningful if used with nothread.  In that case
   it's 1 if the muf program left a true value on top of the data
   stack at point of completion, otherwise 0.  It always returns 0
   if nothread was 0. */
int interp(const dbref player, const dbref program, const dbref trigger, const int writeonly, const char *arg_text)
{
    frame *fr;
    inst first_arg;
    int i;

    if (Garbage(program) || (Typeof(program) != TYPE_PROGRAM)) {
	notify(player, "Program call: Garbage or illegal type.");
	log_status("INTERP: interp called with bad program: %d", program);
	return 0;
    }
  
    /* This shouldn't happen, I don't think.  Well, might happen currently
       if someone gets moveto'd while they are in a program, to a room
       with a lock/desc program. */
    fr = get_frame(player);
    if (fr) {
	notify(player, "You are already running a program.");
	return 0;
    }

    if (!can_link_to(GetOwner(trigger), TYPE_EXIT, program)) {
	notify(player, "Program call: Permission denied.");
	return 0;
    }

    MALLOC(fr, frame, 1);

    fr->next = NULL;

    for (i = 0; i < MAX_VAR; i++) fr->variables[i].type = INST_BADVAR;
  
    /* Set default variables. */
    fr->variables[0].type = INST_OBJECT;
    fr->variables[0].un.object = player;
    fr->variables[1].type = INST_OBJECT;
    fr->variables[1].un.object = GetLoc(player);
    fr->variables[2].type = INST_OBJECT;
    fr->variables[2].un.object = trigger;

    fr->writeonly = writeonly;
    fr->swap = 0;
    fr->error = 0;

    fr->max_create = MAX_INTERP_CREATE;

    fr->orig_program = program;
    fr->player = player;

    fr->no_inst_limit = (Wizard(program) && TrueWizard(GetOwner(program)));
    fr->instructions_run = 0;
    up_inst_limit(fr);

    /* Setup the stacks. */
    fr->addr_stack.top = fr->addr_stack.data + ADDR_STACK_SIZE;
    fr->data_stack.top = fr->data_stack.data + DATA_STACK_SIZE;

    /* Push on the first arg. */
    first_arg.type = INST_STRING;
    first_arg.un.string = make_shared_string(arg_text, strlen(arg_text));
    push_stack(&(fr->data_stack), &first_arg);

    /* Push on the program to run. */
    push_context_dbref(fr, program);

    add_frame(player, fr);

    return interp_loop(fr);
}


/* Returns 1 if the program ran to completion, nothread was true, and the
   top of the stack had a true data element.  Otherwise returns 0. */
int interp_loop(frame *fr)
{
    data_stack *data_st;
    addr_stack *addr_st;
    jump_addr *addr_top;
    int retval;
    prim_func prim_funcptr;
    data_stack pop_stack;	/* Holds args to curr prim, to clear after. */
    inst *curr_inst;
    unsigned long max_instructions = fr->max_instructions;
    unsigned long instructions_run = fr->instructions_run;
    int trace;			/* is current program set debug? */

    data_st = &(fr->data_stack);
    addr_st = &(fr->addr_stack);

    pop_stack.top = pop_stack.data + DATA_STACK_SIZE;

#ifndef INTERP_PASS_FRAME
    curr_frame = fr;
    curr_data_st = data_st;
#endif

    /* Keep going while we are still executing code (some function/offset is
       on the address stack), and nothing caused any sort of error, and
       the player isn't being swapped out for a read or other condition. */
    addr_top = NULL;
    trace = 0;
    fr->cache_dirty = 1;
    while (!empty_stack(addr_st, ADDR_STACK_SIZE)
	   && !fr->error && !fr->swap) {

	/* Increment and check the instruction count limit. */
	instructions_run++;
	if ((instructions_run > max_instructions)
	    && !fr->no_inst_limit) {
	    interp_error(fr, PRIM_NONE, "Instruction count limit exceeded.");
	    break;
	}

	if (fr->cache_dirty) {
	    addr_top = addr_st->top;

	    if (JumpType(addr_top) == JUMP_CONTINUATION) {
	        continue_func func;
		void *data;

		/* Store current values, since we are going to pop. */
		func = JumpContinuation(addr_top);
		data = JumpContData(addr_top);

		pop_context(fr);
		func(fr, data_st, data);
		continue;		/* Top of loop. */
	    }

	    if (JumpType(addr_top) == JUMP_OBJECT) {
	        dbref program;
		struct code *code;

		program = JumpObject(addr_top);

		pop_context(fr);

		if (!get_code(program)) {
		    /* Not compiled, try to compile it. */
		    compile_text(NOTHING, program, NULL);
		}

		code = get_code(program);
		if (!code) {
		    interp_error(fr, PRIM_NONE,
				 "Program not compiled.  Cannot run");
		    break;
		}

		push_context_muf(fr, code->main_func);
		/* fall through -- addr_top ok, since we pop & push */
	    }

	    if (JumpType(addr_top) != JUMP_MUF_FUNC) {
	        log_status("INTERP: bad type (%i) for jump_addr (prog %d).",
			   JumpType(addr_top), fr->orig_program);
		interp_error(fr, PRIM_NONE, "Fatal internal error.");
		break;
	    }

	    /* top of stack is new muf function, get info */
	    {
		dbref new_program = JumpFunc(addr_top)->code->object;
		if (new_program == NOTHING) {
		    interp_error(fr, PRIM_NONE,
				 "Program recycled while running");
		    break;
		}
		fr->program = new_program;
		trace = HasFlag(new_program, DEBUG);
	    }

	    fr->cache_dirty = 0;
	} /* fr->cache_dirty */


	/* Collect the current instruction, then increment the offset
	   by 1.  This way the next time around the loop we'll be at the
	   right spot in the bytecode.  Grab linenumbers here, rather
	   than doing the whole outer loop again for them. */
	curr_inst = JumpCurrInst(addr_top)++;
	while (curr_inst->type == INST_LINENO) {
	    JumpLineno(addr_top) = curr_inst->un.lineno;
	    curr_inst = JumpCurrInst(addr_top)++;
	}



	/* Print debug info (if not type check info.) */
	if (trace && ((curr_inst->type != INST_STKCHECK) &&
		      (curr_inst->type != INST_WIZCHECK))) {
	    debug_inst(fr, data_st, addr_st, curr_inst);
	}


	/* check stack space and argument types - only for INST_PRIMITIVE */
	if (!curr_inst->stack_safe) {
	    check_prim_space(curr_inst->un.primitive, fr, curr_data_st);
	    if (fr->error) break;
	}
	if (!curr_inst->type_safe) {
	    check_prim_types(curr_inst->un.primitive, fr, curr_data_st);
	    if (fr->error) break;
	}


	/* dispatch the instruction */
	switch (curr_inst->type) {
	case INST_STRING:
	    dup_shared_string(curr_inst->un.string);
	    push_stack(curr_data_st, curr_inst);
	    break;
	case INST_ADDRESS:
	    dup_code(curr_inst->un.address->code);
	    push_stack(data_st, curr_inst);
	    break;
	case INST_INTEGER:
	case INST_OBJECT:
	case INST_VARIABLE:
	case INST_CONNECTION:
	    push_stack(data_st, curr_inst);
	    break;
	case INST_OFFSET:
	    /* Do an immediate jump to specifed position. */
	    JumpCurrInst(addr_top) = (JumpFunc(addr_top)->bytecode +
				      curr_inst->un.offset);
	    break;
	case INST_IF:
	    if (!true_inst(data_st->top))
	        JumpCurrInst(addr_top) = (JumpFunc(addr_top)->bytecode +
					  curr_inst->un.offset);
	    clear_inst_interp(curr_data_st->top);
	    data_st->top++;
	    break;
	case INST_NOT_IF:
	    if (true_inst(data_st->top))
	        JumpCurrInst(addr_top) = (JumpFunc(addr_top)->bytecode +
					  curr_inst->un.offset);
	    clear_inst_interp(data_st->top);
	    data_st->top++;
	    break;
	case INST_EXECUTE:
	    /* Start executing a new function. */
	    push_context_muf(fr, curr_inst->un.address);
	    break;
	case INST_EXIT:
	    pop_context(fr);
	    break;
	case INST_PRIMITIVE:
	    prim_funcptr = prim_check(curr_inst->un.primitive,
				      fr, data_st, &pop_stack);
	    if (fr->error) break;
#ifdef INTERP_PASS_FRAME
	    prim_funcptr(fr, data_st);
#else
	    prim_funcptr();
#endif
	    clear_data_stack(&pop_stack);
	    break;
	case INST_WIZCHECK:
	    if (!frame_wizzed(fr)) {
	        interp_error(fr, curr_inst->un.primitive,
			     "Wizard only primitive");
	    }
	    break;
	case INST_STKCHECK:
	    check_prim_space(curr_inst->un.primitive, fr, data_st);
	    break;
	case INST_TYPECHECK:
	    check_prim_types(curr_inst->un.primitive, fr, data_st);
	    break;
	case INST_PICK:
	    {
	        /* Note that 'INST_PICK(x)' is like 'x+1 PICK'. */
	        inst *what = data_st->top + curr_inst->un.integer;
		dup_inst_interp(what);
		push_stack(data_st, what);
	    }
	    break;
	case INST_LINENO:
	    /* already handled */
	    break;
	default:
	    interp_error(fr, PRIM_NONE, "Program internal error.  Aborted");
	    break;
	}
    }

    /* Put some locally cached values back on the frame. */
    fr->instructions_run = instructions_run;


    if (!fr->error && fr->swap) {
	/* Not done yet.  In read or sleep, or similar. */
	return 0;
    }
    
    /* Otherwise, we are really finished.  So use the final value on the
       data stack as the return value (false if empty stack). */
    if (!empty_stack(data_st, DATA_STACK_SIZE)) {
	inst result;

	pop_stack(data_st, &result);
	retval = true_inst(&result);
	clear_inst_interp(&result);	/* Might have been string. */
    } else {
	retval = 0;
    }

    if (fr->error) retval = 0;

    /* The pop_stack only has items in it if prim_check failed, since
       they are still on the real stack, don't pop them. */
    /* clear_data_stack(&pop_stack); */

    /* FIX FIX FIX: is this temporary, or make really permenant. */
    if (!Garbage(fr->orig_program)) {
	IncRunCnt(fr->orig_program, 1);
	IncInstCnt(fr->orig_program, fr->instructions_run);
    }
    
    /* Clean out this frame pointer. */
    clear_frame (fr);

    return retval;
}


void interactive(const dbref player, const char *command)
{
    frame *fr;
    inst string;
  
    SetOffFlag(player, IN_PROGRAM);

    fr = get_frame(player);
    if (!fr) {
	log_status("INTERP: interactive called but player (%u) had no frame.",
		   player, player);
	notify(player,
	       "Whoops, thought you were in a program.  That line was lost.");
	return;
    }

    if (!muck_stricmp(command, BREAK_COMMAND)) {
	clear_frame(fr);
	return;
    }

    fr->swap = 0;

    string.type = INST_STRING;
    string.un.string = make_shared_string(command, strlen(command));
    
    safe_push_data_st(fr, &(fr->data_stack), &string, PRIM_read);

    interp_loop(fr);
}


void interp_quit_external(const dbref player)
{
    frame *fr;

    if (!HasFlag(player, IN_PROGRAM)) return;
  
    SetOffFlag(player, IN_PROGRAM);

    fr = get_frame(player);
    if (!fr) {
	log_status("INTERP: interp_quit_external called but player (%u) had no frame.",
		   player, player);
	return;
    }

    clear_frame(fr);
}


void interp_error(frame *fr, int primitive, const char *format, ...)
{
    addr_stack *st = &(fr->addr_stack);
    const char *prim_name;
    Buffer buf;
    va_list ap;

    if (fr->error) {
	log_status("INTERP: primitive %i called interp_error when fr->error was 1.",
		   primitive);
	return;
    }

    fr->error = 1;

    va_start(ap, format);
    Bufvsprint(&buf, format, ap);
    va_end(ap);

    if (!Garbage(fr->orig_program) && !empty_stack(st, ADDR_STACK_SIZE)
	&& (JumpType(st->top) == JUMP_MUF_FUNC)) {
	notify(fr->player,
	       "Please tell %n the following information for program #%d:",
	       GetOwner(fr->orig_program), fr->orig_program);

	if (primitive == PRIM_NONE) prim_name = "";
        else prim_name = prim2name(primitive);


	notify(fr->player, "#%d (line %i, func %s) %s: %S.",
	       fr->program, JumpLineno(st->top),
	       DoAnonymous(JumpFunc(st->top)->name),
	       prim_name, &buf);
    } else {
	notify(fr->player, "%S.", &buf);
    }
}


void init_interp(void)
{
    init_code_table();
    init_frame_table();
}


void clear_interp(void)
{
    clear_code_table();
    clear_frame_table();
}
