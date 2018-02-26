/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* disassem.c,v 2.8 1997/08/30 06:44:04 dmoore Exp */
#include "config.h"

#include "db.h"
#include "buffer.h"
#include "code.h"
#include "externs.h"

void disassemble(const dbref player, const dbref program)
{
    struct code *code;
    struct func_addr *func;
    struct inst *bytecode;
    int    i, num_funcs;
    int    j, num_inst;
    static Buffer buf;
    static Buffer buf2;
    
    code = get_code(program);
    if (!code) {
	notify(player, "Program not compiled.");
	return;
    }

    num_funcs = code->num_funcs;
    notify(player, "Program %d.  Number of functions: %i",
	   program, num_funcs);

    for (i = 0; i < num_funcs; i++) {
	func = code->funcs[i];
	num_inst = func->num_inst;
	notify(player, "Function %i: %s.  Number of instructions: %i",
	       i, DoAnonymous(func->name), num_inst);

	bytecode = func->bytecode;
	for (j = 0; j < num_inst; j++, bytecode++) {
	    Bufsprint(&buf, "    %i", j);
	    switch(bytecode->type) {
	    case INST_STRING:	Bufcat(&buf, ": STRING      : "); break;
	    case INST_INTEGER:	Bufcat(&buf, ": INTEGER     : "); break;
	    case INST_OBJECT:	Bufcat(&buf, ": OBJECT      : "); break;
	    case INST_ADDRESS:	Bufcat(&buf, ": ADDRESS     : "); break;
	    case INST_CONNECTION:
		Bufcat(&buf, ": CONNECTION  : ");
		break;
	    case INST_VARIABLE:	Bufcat(&buf, ": VARIABLE    : "); break;
	    case INST_OFFSET:	Bufcat(&buf, ": JUMP OFFSET : "); break;
	    case INST_PRIMITIVE:
		Bufsprint(&buf2, ": PRIM (S%s T%s): ",
			  (bytecode->stack_safe ? "y" : "n"),
			  (bytecode->type_safe ? "y" : "n"));
		Bufcat(&buf, Buftext(&buf2));
		break;
	    case INST_IF:	Bufcat(&buf, ": IF          : "); break;
	    case INST_NOT_IF:	Bufcat(&buf, ": NOT IF      : "); break;
	    case INST_EXECUTE:	Bufcat(&buf, ": EXECUTE     : "); break;
	    case INST_LINENO:	Bufcat(&buf, ": LINE NUMBER : "); break;
	    case INST_BADVAR:	Bufcat(&buf, ": BAD VAR     : "); break;
	    case INST_WIZCHECK: Bufcat(&buf, ": WIZ CHECK   : "); break;
	    case INST_STKCHECK:	Bufcat(&buf, ": STK CHECK   : "); break;
	    case INST_TYPECHECK:
	        Bufcat(&buf, ": TYPE CHECK  : ");
		break;
	    case INST_PICK:     Bufcat(&buf, ": N PICK      : "); break;
	    case INST_EXIT:     Bufcat(&buf, ": EXIT        : "); break;
	    default:		Bufcat(&buf, ": ILLEGAL     : "); break;
	    }
	    inst_bufcat(&buf, bytecode);
	    notify(player, Buftext(&buf));
	}
    }
    notify(player, "Done.");
}

