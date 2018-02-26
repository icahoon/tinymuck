/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* inst.c,v 2.8 1997/08/30 06:44:05 dmoore Exp */
#include "config.h"

#include <string.h>

#include "db.h"
#include "code.h"
#include "buffer.h"
#include "typechk.h"
#include "prim_offsets.h"
#include "externs.h"

/* #### support for primitives taking a list input is not working. */

/* The size on types below is needed because you can't initialize array's
   in structs, unless the array size is preknown in ansi c. */
struct primitive_info {
    const char *name;
    prim_func func;
    unsigned int wiz_only : 1;
    unsigned int num_args : 8;
    unsigned int num_rtn : 8;
    const int arg_types[5];
    const int rtn_types[5];
};

static struct primitive_info prim_table[] = {
#include "primtab.h"		/* Generated by mkprimtab.h */
};

#define NUM_PRIM_ENTRIES (sizeof(prim_table)/sizeof(*prim_table))


const char *prim2name(const int primitive)
{
    static Buffer buf;

    if (primitive == PRIM_PUSH) return "PUSH";

    if ((primitive < 0) || (primitive >= NUM_PRIM_ENTRIES)) {
	Bufsprint(&buf, "Illegal primitive (%i).", primitive);
	return Buftext(&buf);
    }

    return prim_table[primitive].name;
}


/* Check for permissions to the given object. */
void check_perms(frame *fr, const dbref object, const int prim)
{
    if (!permissions(fr, object))
	interp_error(fr, prim, "Permission denied to #%d", object);
}


/* Check to see if this primitive can be run based only on stack space. */
void check_prim_space(const int primitive, frame *fr, data_stack *st)
{
    int depth, num_args, num_rtn;

    if (primitive == PRIM_PUSH) {
        num_args = 0;
	num_rtn = 1;
    } else {
	num_args = prim_table[primitive].num_args;
	num_rtn = prim_table[primitive].num_rtn;
    }

    depth = depth_stack(st, DATA_STACK_SIZE);

    if (num_args > depth) {
	interp_error(fr, primitive, "Data stack underflow");
	return;
    }

    /* Check that there will be enough room for the result. */
    if (num_rtn + depth - num_args > DATA_STACK_SIZE) {
	interp_error(fr, primitive, "Data stack overflow");
    }
}


void check_prim_types(const int primitive, frame *fr, data_stack *st)
{
    typechk_stack(fr, st, primitive,
		  prim_table[primitive].num_args,
		  prim_table[primitive].arg_types);
}


/* Check the arguments to a primitive, and assign func to the proper C
   routine to handle this primitive.  If the type checking fails then
   return an error string, otherwise NULL. */
prim_func prim_check(const int primitive, frame *fr, data_stack *st, data_stack *pop_stack)
{
    inst *curr;
    int pop_cnt;

    pop_cnt = prim_table[primitive].num_args;

    /* Put everything that needs to be cleared onto the pop_stack, so
       after the primitive is done we can clear all the args. */
    for (curr = st->top; pop_cnt; pop_cnt--, curr++) {
        if (dup_inst_p(curr)) push_stack(pop_stack, curr);
    }

    return prim_table[primitive].func;
}


/* This should use a hash table (probably) or a binary search once the
   shell script works. */
int lookup_primitive(const char *name)
{
    int primitive;

    for (primitive = 0; primitive < NUM_PRIM_ENTRIES; primitive++) {
	if (!muck_stricmp(prim_table[primitive].name, name)) {
	    /* The names match. */
	    return primitive;
	}
    }

    return -1;
}


int num_args_primitive(const int primitive)
{
    return prim_table[primitive].num_args;
}


int num_rtn_primitive(const int primitive)
{
    return prim_table[primitive].num_rtn;
}

/* is the return type of this primitive a list? */
int list_rtn_primitive(const int primitive)
{
    int i, num_rtn;

    num_rtn = prim_table[primitive].num_rtn;
    for (i = 0; i < num_rtn; i++) {
        /* #### this isn't right if more than 1 list is returned */
        if (prim_table[primitive].rtn_types[i] == INST_T_LIST)
	    return 1;
    }
    
    return 0;
}


int wiz_primitive(const int primitive)
{
    return prim_table[primitive].wiz_only;
}


#define DoNull(x)		((x)->un.string ? (x)->un.string->data : "")

/* converts an instruction into a printable string, which is appended
   to the given buffer. */
void inst_bufcat(Buffer *buf, inst *what)
{
    static Buffer buf2;
        
    switch (what->type) {
    case INST_STRING:
	Bufsprint(&buf2, "\"%s\"", DoNull(what));
	break;
    case INST_INTEGER:
	Bufsprint(&buf2, "%i", what->un.integer);
	break;
    case INST_OBJECT:
	Bufsprint(&buf2, "#%d", what->un.object);
	break;
    case INST_CONNECTION:
	Bufsprint(&buf2, "C%i", what->un.connection);
	break;
    case INST_ADDRESS:
	Bufsprint(&buf2, "'#%d'%s", what->un.address->code->object,
		  DoAnonymous(what->un.address->name));
	break;
    case INST_VARIABLE:
	Bufsprint(&buf2, "V%i", what->un.variable);
	break;
    case INST_OFFSET:
	Bufsprint(&buf2, "JMP(%i)", what->un.offset);
	break;
    case INST_PRIMITIVE:
	Bufcpy(&buf2, prim2name(what->un.primitive));
	break;
    case INST_IF:
	Bufsprint(&buf2, "IF(%i)", what->un.offset);
	break;
    case INST_NOT_IF:
	Bufsprint(&buf2, "NOT IF(%i)", what->un.offset);
	break;
    case INST_EXECUTE:
	Bufsprint(&buf2, "#%d'%s", what->un.address->code->object,
		  DoAnonymous(what->un.address->name));
	break;
    case INST_LINENO:
	Bufsprint(&buf2, "L%i", what->un.lineno);
	break;
    case INST_BADVAR:
	Bufsprint(&buf2, "XXX");
	break;
    case INST_WIZCHECK:
        Bufsprint(&buf2, "WIZCHK(%s)", prim2name(what->un.primitive));
	break;
    case INST_STKCHECK:
	Bufsprint(&buf2, "STKCHK(%s)", prim2name(what->un.primitive));
	break;
    case INST_TYPECHECK:
	Bufsprint(&buf2, "TYPECHK(%s)", prim2name(what->un.primitive));
	break;
    case INST_PICK:
        /* Note that 'x INST_PICK' is like 'x+1 PICK'. */
        switch (what->un.integer) {
	case 0: Bufcpy(&buf2, "dup"); break;
	case 1: Bufcpy(&buf2, "over"); break;
	default: Bufsprint(&buf2, "PICK %i", what->un.integer + 1); break;
	}
	break;
    case INST_EXIT:
        Bufcpy(&buf2, "exit");
	break;
    default:
	Bufsprint(&buf2, "???");
	break;
    }

    Bufcat(buf, Buftext(&buf2));
}


void debug_inst(frame *fr, data_stack *data_st, addr_stack *addr_st, inst *curr_inst)
{
    int	i, count;
    static Buffer buf;

    if (curr_inst) {
	Bufsprint(&buf, "D(#%d L%i) S(",
		  JumpFunc(addr_st->top)->code->object,
		  JumpLineno(addr_st->top));
    } 

    count = depth_stack(data_st, DATA_STACK_SIZE);

    if (count > 5) {
	Bufcat(&buf, "..., ");
	count = 5;
    } 

    for (i = 0; i < count; i++) {
	inst_bufcat(&buf, data_st->top + (count - 1 - i));
	if (i < count-1) Bufcat(&buf, ", ");
    }

    Bufcat(&buf, ") ");
    inst_bufcat(&buf, curr_inst);

    notify(fr->player, "%S", &buf);
}

