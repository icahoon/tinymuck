/* Copyright (c) 1997 by David Moore.  All rights reserved. */
/* optimize.c,v 2.3 1997/08/30 06:43:46 dmoore Exp */
#include "config.h"

#include <string.h>
#include <stdarg.h>

#include "db.h"
#include "code.h"
#include "prim_offsets.h"
#include "text.h"
#include "buffer.h"
#include "hashtab.h"
#include "externs.h"

struct optim_inst {
    int type;
    union inst_un un;
    int lineno;
    int blockno;		/* basic block # */
    int new_offset;		/* offset after optimization */

    unsigned int reset_pt : 1;	/* optim state should reset on this inst */
    unsigned int lineno_pt : 1; /* new line numbers needed */
    unsigned int reachable : 1;	/* used to find dead code */
    unsigned int dead : 1;	/* dead code */
    unsigned int wizzed : 1;	/* requires a wizard check */
    unsigned int type_safe : 1;
    unsigned int stack_safe : 1;

    int reaching_cnt;		/* number of blocks entering here */
    int *reaching_offsets;	/* list of offsets coming into this inst */

    int known_depth;
    int known_space;
};


#define constant_type(t) \
    ((t) == INST_STRING || (t) == INST_INTEGER || (t) == INST_OBJECT || \
     (t) == INST_ADDRESS || (t) == INST_VARIABLE)

#define jump_type(t) \
    ((t) == INST_OFFSET || (t) == INST_IF || (t) == INST_NOT_IF)

#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#define MIN(a, b) ((a) < (b)) ? (a) : (b)


/* note if you change this, be sure that it doesn't depend on all of the
   reaching_cnt and reaching_offsets being available, since we use this
   function to help track down some that the real compiler hides. */
static struct optim_inst *find_prev_inst(struct optim_inst *instructions, struct optim_inst *curr)
{
    struct optim_inst *prev;

    if (!curr) return NULL;

    prev = curr - 1;
    while ((prev >= instructions) && prev->dead) {
        if (prev->blockno != curr->blockno) break;
        prev--;
    }

    if (prev->blockno != curr->blockno) return NULL;

    if (prev < instructions) return NULL;

    return prev;
}


/* Peephole optimize certain instructions and sequences. */
static void peephole_optim(struct optim_inst *instructions, const int num_inst)
{
    int pos;
    struct optim_inst *curr, *prev;

    pos = 0;
    while (pos < num_inst) {
        if (instructions[pos].dead) {
	    pos++;
	    continue;
	}

	curr = &instructions[pos];
	prev = find_prev_inst(instructions, curr);

	/* Let the optimizations begin. */
	switch (curr->type) {
	case INST_IF:
	    if (prev &&
		(prev->type == INST_PRIMITIVE) &&
		(prev->un.primitive == PRIM_not)) {
	        prev->dead = 1;
		curr->type = INST_NOT_IF;
		continue;	/* don't move up pos */
	    }
	    if (prev && constant_type(prev->type)) {
	        struct inst temp;
		temp.type = prev->type;
		temp.un = prev->un;
	        if (true_inst(&temp)) {
		    curr->dead = 1;
		} else {
		    curr->type = INST_OFFSET;
		}
		prev->dead = 1;
	    }
	    break;

	case INST_NOT_IF:
	    if (prev &&
		(prev->type == INST_PRIMITIVE) &&
		(prev->un.primitive == PRIM_not)) {
	        prev->dead = 1;
		curr->type = INST_NOT_IF;
		continue;	/* don't move up pos */
	    }

	    if (prev && constant_type(prev->type)) {
	        struct inst temp;
		temp.type = prev->type;
		temp.un = prev->un;
	        if (!true_inst(&temp)) {
		    curr->dead = 1;
		} else {
		    curr->type = INST_OFFSET;
		}
		prev->dead = 1;
	    }
	    break;

	case INST_PICK:
	    {
		struct optim_inst *data = curr;
	        int i, all_const = 1;
		for(i = 0; i <= curr->un.integer; i++) {
		    data = find_prev_inst(instructions, data);
		    if (!data || !constant_type(data->type)) {
		        all_const = 0;
			break;
		    }
		}
		if (all_const) {
		    curr->type = data->type;
		    curr->un = data->un;
		    if (curr->type == INST_STRING)
		        dup_shared_string(curr->un.string);
		}
	    }
	    break;


	case INST_PRIMITIVE:
	    switch (curr->un.primitive) {
	    case PRIM_swap:
	        if (prev && constant_type(prev->type)) {
		    struct optim_inst *prevprev;
		    prevprev = find_prev_inst(instructions, prev);
		    if (prevprev && constant_type(prevprev->type)) {
		        struct inst temp;
			temp.type = prev->type;
			temp.un = prev->un;
			prev->type = prevprev->type;
			prev->un = prevprev->un;
			prevprev->type = temp.type;
			prevprev->un = temp.un;
			curr->dead = 1;
		    }
		}
		break;

	    case PRIM_dup:
	        curr->type = INST_PICK;
		curr->un.integer = 0;
		break;

	    case PRIM_over:
	        curr->type = INST_PICK;
		curr->un.integer = 1;
		break;

	    case PRIM_pick:
	        if (prev && (prev->type == INST_INTEGER)) {
		    prev->dead = 1;
		    curr->type = INST_PICK;
		    curr->un.integer = prev->un.integer - 1;
		}
		break;

	    case PRIM_pop:
	        if (prev && (constant_type(prev->type) ||
			     (prev->type == INST_PICK))) {
		    prev->dead = 1;
		    curr->dead = 1;
		}
		break;

	    case PRIM_execute:
	        if (prev && (prev->type == INST_ADDRESS)) {
		    prev->dead = 1;
		    curr->type = INST_EXECUTE;
		    curr->un.address = prev->un.address;
		}
		break;


	    /* a very large number of calls to property access routines
	       have the string known at compile time.  it might be very
	       smart to provide specialized versions of these functions
	       where the string is converted to an istring at compile
	       time, and the property code allows for looking up by
	       istring.  The main cost in property lookup is the istring
	       cost. */
	    case PRIM_getpropv:
	        if (prev && (prev->type == INST_STRING)) {
		    curr->un.primitive = PRIM_getpropv_i;
		}
		break;
	      
	    case PRIM_getprops:
	        if (prev && (prev->type == INST_STRING)) {
		    curr->un.primitive = PRIM_getprops_i;
		}
		break;
	      
	    case PRIM_getpropd:
	        if (prev && (prev->type == INST_STRING)) {
		    curr->un.primitive = PRIM_getpropd_i;
		}
		break;
	      
	    case PRIM_rmvprop:
	        if (prev && (prev->type == INST_STRING)) {
		    curr->un.primitive = PRIM_rmvprop_i;
		}
		break;

	    case PRIM_addprop:
	        /* not worth the trouble, and not much can gain */
	        break;

	    case PRIM_setprop:
	        /* probably not much gain, but more likely to happen */
	        if (prev && constant_type(prev->type)) {
		    struct optim_inst *prev2;
		    prev2 = find_prev_inst(instructions, prev);
		    if (prev2 && (prev2->type == INST_STRING)) {
		        curr->un.primitive = PRIM_setprop_i;
		    }
		}
		break;

	    /* Maybe we should have a version of flagp with the string->
	       int lookup done at compile time.  On the other hand, it's
	       still probably smarter to just give something like
	       unparseobj.  we can also only use this optimization if
	       the string name given is the same flag across all object
	       types. */
	    case PRIM_flagp:
	        if (prev && (prev->type == INST_STRING)) {
		    curr->un.primitive = PRIM_flagp_i;
		}
		break;

	    default:
		break;
	    }
	    break;

	default:
	    break;
	}

	/* move to the next spot */
	pos++;
    }
}


static void depth_optim(struct optim_inst *instructions, const int num_inst)
{
    struct optim_inst *curr;
    int i;
    int num_args, num_new, list_rtn;
    int cur_spot, known_depth, known_space;

    cur_spot = 0;
    known_depth = 0;
    known_space = 0;

    for (i = 0; i < num_inst; i++) {
        curr = &instructions[i];

	/* decide if we need to flush info, or maybe we can get it from
	   blocks jumping to here. */
	if (curr->reset_pt) {
	    cur_spot = 0;
	    known_depth = 0;
	    known_space = 0;
	} else if (curr->reaching_cnt) {
	    int j; 
	    int *offsets = curr->reaching_offsets;
	    struct optim_inst *reaching;

	    known_depth = 2048;
	    known_space = 2048;
	    for (j = 0; j < curr->reaching_cnt; j++) {
	        reaching = &instructions[offsets[j]];
	        known_depth = MIN(known_depth, reaching->known_depth);
	        known_space = MIN(known_space, reaching->known_space);
	    }
	    cur_spot = 0;
	} 

	/* if you allow killing of ifs and offsets, then this should
	   update the info like below, or better, you should have
	   already taken the info out of the destination. */
	if (curr->dead) continue;

	switch (curr->type) {
	case INST_STRING:
	case INST_INTEGER:
	case INST_OBJECT:
	case INST_ADDRESS:
	case INST_VARIABLE:
	case INST_CONNECTION:
	    num_args = 0;
	    num_new = 1;
	    list_rtn = 0;
	    break;

	case INST_IF:
	case INST_NOT_IF:
	    num_args = 1;
	    num_new = -1;
	    list_rtn = 0;
	    break;

	case INST_PICK:
	    num_args = curr->un.integer + 1;
	    num_new = 1;
	    list_rtn = 0;
	    break;

	case INST_OFFSET:
	case INST_EXECUTE:
	case INST_EXIT:
	    num_args = 0;
	    num_new = 0;
	    list_rtn = 0;
	    break;

	case INST_PRIMITIVE:
	    switch (curr->un.primitive) {
	    case PRIM_swap:
	        num_args = 2;
		num_new = 0;
		list_rtn = 0;
		break;
	    case PRIM_rot:
	        num_args = 3;
		num_new = 0;
		list_rtn = 0;
		break;
	    default:
	        num_args = num_args_primitive(curr->un.primitive);
		num_new = num_rtn_primitive(curr->un.primitive) - num_args;
		list_rtn = list_rtn_primitive(curr->un.primitive);
		break;
	    }
	    break;

	default:
	    log_status("OPTIMIZE: unexpected type %i from compiler",
		       curr->type);
	    panic("bad type in optimize");
	    num_args = num_new = list_rtn = 0; /* shut up the compiler */
	    break;
	}

	if ((num_args <= known_depth + cur_spot) &&
	    (num_new <= known_space - cur_spot)) {
	    curr->stack_safe = 1;
	}

	known_depth = MAX(known_depth, num_args - cur_spot);
	known_space = MAX(known_space, num_new + cur_spot);
	cur_spot += num_new;

	if (list_rtn) {
	    known_depth = known_depth + cur_spot;
	    known_space = 0;
	    cur_spot = 0;
	}

	if (jump_type(curr->type)) {
	    curr->known_depth = known_depth + cur_spot;
	    curr->known_space = known_space - cur_spot;
	}
    }
}


static void type_optim(struct optim_inst *instructions, const int num_inst)
{
    struct optim_inst *curr, *prev;
    int i;

    for (i = 0; i < num_inst; i++) {
        if (instructions[i].dead) continue;

        curr = &instructions[i];
	prev = find_prev_inst(instructions, curr);

	/* simple version follows */

	if (curr->type != INST_PRIMITIVE) {
	    curr->type_safe = 1;
	    continue;
	}

	if (num_args_primitive(curr->un.primitive) == 0) {
	    curr->type_safe = 1;
	    continue;
	}

	switch (curr->un.primitive) {
	case PRIM_pop:
	case PRIM_not:
	case PRIM_dup:
	case PRIM_over:
	case PRIM_swap:
	case PRIM_rot:
	case PRIM_okp:
	case PRIM_int:
	case PRIM_and:
	case PRIM_or:
	case PRIM_intostr:
	    curr->type_safe = 1;
	    break;

	case PRIM_pick:
	case PRIM_put:
	    if (prev && (prev->type == INST_INTEGER))
	        curr->type_safe = (prev->un.integer > 0);
	    break;

	case PRIM_rotate:
	case PRIM_dbref:
	case PRIM_variable:
	    if (prev && (prev->type == INST_INTEGER))
	        curr->type_safe = 1;
	    break;

	case PRIM_at:
	case PRIM_bang:
	    if (prev && (prev->type == INST_VARIABLE)) 
	        curr->type_safe = 1;
	    break;

	case PRIM_call:
	    /* can't check garbage at compile time. */
	    break;

	}
    }
}


static void dead_optim_1(struct optim_inst *instructions, int pos)
{
    struct optim_inst *curr;

    while (1) {
        curr = &instructions[pos];

	if (curr->reachable) return;
	curr->reachable = 1;

	if (curr->dead) {
	    pos++;
	    continue;
	}

	switch (curr->type) {
	case INST_OFFSET:
	    /* start checking from destination */
	    pos = curr->un.offset;
	    continue;
	    
	case INST_IF:
	case INST_NOT_IF:
	    /* recurse down jump path, we keep checking fallthru here. */
	    dead_optim_1(instructions, curr->un.offset);
	    break;

	case INST_EXIT:
	    /* nothing more to check here */
	    return;
	}
	
	pos++;
    }
}

/* find unreachable code -- you need to rebuild the basic block info
   after this */
static void dead_optim(struct optim_inst *instructions, const int num_inst)
{
    struct optim_inst *curr;
    int i, j, k, done;

    for (i = 0; i < num_inst; i++)
        instructions[i].reachable = 0;

    dead_optim_1(instructions, 0);

    for (i = 0; i < num_inst; i++) {
        curr = &instructions[i];
	if (!curr->reachable)
	    curr->dead = 1;
	curr->reachable = 0;
    }

    /* now we update offsets to not point to dead instructions.  and
       get any jump to a jump.  if we also notice that the destination
       of the jump is the same place we'd fall thru to, then we kill
       the jump. */
    for (i = 0; i < num_inst; i++) {
        curr = &instructions[i];
	if (curr->dead) continue;
	
	if (jump_type(curr->type)) {
	    /* Move j to where we'd get if we followed the jump */
	    j = curr->un.offset;
	    do {
	        done = 1;
	        while (instructions[j].dead) j++;
		if (instructions[j].type == INST_OFFSET) {
		    j = instructions[j].un.offset;
		    done = 0;
		}
	    } while (!done);

	    /* Move k to where we'd get if we fell-thru */
	    k = i + 1;
	    do {
	        done = 1;
	        while (instructions[k].dead) k++;
		if (instructions[k].type == INST_OFFSET) {
		    k = instructions[k].un.offset;
		    done = 0;
		}
	    } while (!done);
	    
	    if (j == k) {
	        curr->dead = 1;	/* jump destination == fall_thru spot */
	    } else {
	        curr->un.offset = j;
	    }
	}
    }
}


#define LINK_BLOCK(FROM, TO) \
    do { \
	   int from_offset = (FROM); \
           int to_offset = (TO); \
	   struct optim_inst *dest = &instructions[to_offset]; \
	   int cnt = dest->reaching_cnt; \
	   if (cnt) { \
	       REALLOC(dest->reaching_offsets, int, cnt+1); \
	   } else { \
	       MALLOC(dest->reaching_offsets, int, 1); \
	   } \
	   dest->reaching_offsets[cnt] = from_offset; \
	   dest->reaching_cnt = cnt + 1; \
    } while (0)


static void compute_blocks(struct optim_inst *instructions, const int num_inst)
{
    struct optim_inst *curr;
    int i, blockno;

    for (i = 0; i < num_inst; i++) {
        curr = &instructions[i];

        if (curr->reaching_offsets)
	    FREE(curr->reaching_offsets);
	curr->reaching_cnt = 0;
	curr->reset_pt = 0;
	curr->lineno_pt = 0;
    }


    for (i = 0; i < num_inst; i++) {
        curr = &instructions[i];

	if (curr->dead) continue;

	/* Collect special info */
        switch (curr->type) {
	case INST_OFFSET:
	    LINK_BLOCK(i, curr->un.offset);
	    if (i+1 < num_inst) instructions[i+1].reset_pt = 1;
	    break;

	case INST_IF:
	case INST_NOT_IF:
	    LINK_BLOCK(i, curr->un.offset);
	    if (i+1 < num_inst) LINK_BLOCK(i, i+1);
	    break;

	case INST_EXECUTE:
	    if (i+1 < num_inst) instructions[i+1].reset_pt = 1;
	    break;

	case INST_PRIMITIVE:
	    if (((curr->un.primitive == PRIM_call) ||
		 (curr->un.primitive == PRIM_execute) ||
		 (curr->un.primitive == PRIM_set)) &&
		(i+1 < num_inst)) {
	        /* we include PRIM_set since it might change WIZ or other. */
	        instructions[i+1].reset_pt = 1;
	    }
	    break;
	}
    }

    /* find the basic blocks & line number spots */
    blockno = 0;
    for (i = 0; i < num_inst; i++) {
        curr = &instructions[i];
        if (curr->reset_pt || curr->reaching_cnt) {
	    blockno++;
	    curr->lineno_pt = 1;
	}
	curr->blockno = blockno;
    }

    /* the real compiler optimizes out jumps at the end of if's & co.
       So if we're already a jump destination, see if the previous
       instruction could fall through to here also. */
    for (i = 1; i < num_inst; i++) {
        curr = &instructions[i];
        if (curr->reaching_cnt) {
	    struct optim_inst *prev;
	    prev = find_prev_inst(instructions, curr);
	    if (prev && !jump_type(prev->type)) {
	        LINK_BLOCK(prev-instructions, i);
	    }
	}
    }

}


static struct optim_inst *interp_to_optim(func_addr *func)
{
    struct optim_inst *instructions, *curr;
    int i, num_inst, lineno, type;
    union inst_un data;

    num_inst = func->num_inst;
    MALLOC(instructions, struct optim_inst, num_inst);
    for (i = 0; i < num_inst; i++) {
	curr = &instructions[i];
        curr->new_offset = -1;
        curr->reset_pt = 0;
        curr->lineno_pt = 0;
 	curr->wizzed = 0;
	curr->reachable = 0;
        curr->dead = 0;
	curr->type_safe = 0;
	curr->stack_safe = 0;

	curr->reaching_cnt = 0;
	curr->reaching_offsets = NULL;
	curr->known_depth = 0;
	curr->known_space = 0;
    }
    instructions[0].reset_pt = 1;

    lineno = -1;

    for (i = 0; i < num_inst; i++) {
        type = func->bytecode[i].type;
	data = func->bytecode[i].un;

	curr = &instructions[i];

	if (type == INST_LINENO) {
	    lineno = data.lineno;
	    curr->dead = 1;
	}

	curr->type = type;
	curr->un = data;
	curr->lineno = lineno;
	
	if ((type == INST_PRIMITIVE) && wiz_primitive(data.primitive))
	    curr->wizzed = 1;
    }


    return instructions;
}


#define EMIT(FUNC_ADDR, TYPE, DATA, STACK_SAFE, TYPE_SAFE) \
    do {\
        int _offset = (FUNC_ADDR)->num_inst++; \
        struct inst *_inst = &((FUNC_ADDR)->bytecode[_offset]); \
	_inst->type = (TYPE); \
	_inst->un = (DATA); \
	_inst->stack_safe = (STACK_SAFE); \
	_inst->type_safe = (TYPE_SAFE); \
    } while(0)

#define EMIT_STK(FUNC_ADDR, PRIM) \
    do {\
        union inst_un _temp; \
        _temp.primitive = PRIM; \
	EMIT(FUNC_ADDR, INST_STKCHECK, _temp, 1, 1); \
    } while (0)


static void optim_to_interp(struct optim_inst *instructions, const int num_inst, func_addr *result)
{
    struct optim_inst *curr;
    int i, lineno, wizchecked, next_offset;
    int codesize;
    union inst_un temp;
    int old_num_inst;
    struct inst *oldbytecode;

    oldbytecode = result->bytecode;
    old_num_inst = result->num_inst;

    /* make a quick educated guess about the new size of the program,
       if it's off, we'll resize it anyways. */
    codesize = num_inst + (num_inst >> 6);
    for (i = 0; i <= num_inst; i++) {
        if (instructions[i].dead) codesize--;
        if (instructions[i].lineno_pt) codesize++;
	if (instructions[i].wizzed) codesize++;
	if (!instructions[i].stack_safe) codesize++;
    }
    codesize = codesize + (2048 - (codesize % 2048));

    MALLOC(result->bytecode, struct inst, codesize);
    result->num_inst = 0;

    lineno = -1;
    wizchecked = 0;
    next_offset = -1;

    for (i = 0; i < num_inst; i++) {
        /* see if we need to resize the bytecode arrays.  We may add up
	   to 6 things below, so make sure enough space for all of them. */
        if (result->num_inst + 5 >= codesize) {
            codesize += 2048;
	    REALLOC(result->bytecode, struct inst, codesize);
	}

        curr = &instructions[i];

	if (curr->reset_pt || curr->reaching_cnt) {
	    wizchecked = 0;
	}

	if (curr->lineno_pt) lineno = -1;

        if (curr->dead) continue;

	if (curr->lineno != lineno) {
	    lineno = curr->lineno;
	    temp.integer = lineno;
	    EMIT(result, INST_LINENO, temp, 1, 1);
	}

	if (curr->wizzed && !wizchecked) {
	    wizchecked = 1;
	    EMIT(result, INST_WIZCHECK, curr->un, 1, 1);
	}

	/* emit needed stack depth checks */
	if (!curr->stack_safe) {
	    curr->stack_safe = 1; /* we make most of them safe here */

	    switch (curr->type) {
	    case INST_STRING:     EMIT_STK(result, PRIM_PUSH); break;
	    case INST_INTEGER:    EMIT_STK(result, PRIM_PUSH); break;
	    case INST_OBJECT:     EMIT_STK(result, PRIM_PUSH); break;
	    case INST_ADDRESS:    EMIT_STK(result, PRIM_PUSH); break;
	    case INST_VARIABLE:   EMIT_STK(result, PRIM_PUSH); break;
	    case INST_CONNECTION: EMIT_STK(result, PRIM_PUSH); break;
	    case INST_IF:         EMIT_STK(result, PRIM_if); break;
	    case INST_NOT_IF:     EMIT_STK(result, PRIM_if); break;

	    case INST_PRIMITIVE:
	        curr->stack_safe = 0;
		break;

	    case INST_PICK:
	        switch (curr->un.integer) {
		case 0:  
		    curr->stack_safe = 0;
		    curr->type_safe = 1;
		    curr->type = INST_PRIMITIVE;
		    curr->un.primitive = PRIM_dup;
		    break;
		case 1:  
		    curr->stack_safe = 0;
		    curr->type_safe = 1;
		    curr->type = INST_PRIMITIVE;
		    curr->un.primitive = PRIM_over;
		    break;
		default:
		    EMIT_STK(result, PRIM_PUSH);
		    temp.integer = curr->un.integer + 1;
		    EMIT(result, INST_INTEGER, temp, 1, 1);
		    curr->stack_safe = 0;
		    curr->type_safe = 1;
		    curr->type = INST_PRIMITIVE;
		    curr->un.primitive = PRIM_pick;
		    break;
		}
		break;

	    case INST_OFFSET:    break;
	    case INST_EXECUTE:   break;
	    case INST_EXIT:	 break;

	    default:
	        log_status("OPTIMIZE: unexpected type %i from compiler",
			   curr->type);
		panic("bad type in optimize");
		break;
	    }
	}

	if (!curr->type_safe && (curr->type != INST_PRIMITIVE)) {
	    curr->type_safe = 1;
	}


	if (curr->type == INST_STRING) {
	    dup_shared_string(curr->un.string);
	}

	curr->new_offset = next_offset;
	EMIT(result, curr->type, curr->un, curr->stack_safe, curr->type_safe);
	next_offset = result->num_inst;
    }

    /* update the offsets */
    for (i = 0; i < result->num_inst; i++) {
        if (jump_type(result->bytecode[i].type)) {
	    int offset = result->bytecode[i].un.offset;
	    curr = &instructions[offset];
	    while ((curr->new_offset == -1) && (offset < num_inst)) {
	        /* scan forward looking for someplace to jump */
	        offset++;
		curr = &instructions[offset];
	    }
	    if (curr->new_offset == -1)
	        panic("optimize jumping to no longer existing code");
	    result->bytecode[i].un.offset = curr->new_offset;
	}
    }

    /* clean up the old bytecode/data now that we've dupped strings */
    for (i = 0; i < old_num_inst; i++) {
        if (oldbytecode[i].type == INST_STRING) {
	    clear_shared_string(oldbytecode[i].un.string);
	}
    }
    FREE(oldbytecode);

    /* shrink up any extra space we had. */
    REALLOC(result->bytecode, struct inst, result->num_inst);
}


static void optimize_func(const dbref who, func_addr *func)
{
    struct optim_inst *instructions;
    int i, num_inst;

    /* convert to format for optimization */
    num_inst = func->num_inst;
    instructions = interp_to_optim(func);
    compute_blocks(instructions, num_inst);
    

    /* optimize -- could loop til no changes, but 3 times is easier */
    for (i = 0; i < 3; i++) {
        peephole_optim(instructions, num_inst);
	dead_optim(instructions, num_inst);
	compute_blocks(instructions, num_inst);
    }

    depth_optim(instructions, num_inst);
    type_optim(instructions, num_inst);


    /* convert back */
    optim_to_interp(instructions, num_inst, func);

    /* clean up */
    for (i = 0; i < num_inst; i++) {
        if (instructions[i].reaching_offsets)
	    FREE(instructions[i].reaching_offsets);
    }
    FREE(instructions);
}


void optimize_code(const dbref who, struct code *code)
{
    int i;

    for (i = 0; i < code->num_funcs; i++) {
        optimize_func(who, code->funcs[i]);
    }
}
