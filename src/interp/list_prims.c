/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* list_prims.c,v 2.4 1996/01/26 01:04:16 dmoore Exp */
#include "config.h"

#include "db.h"
#include "code.h"
#include "prim_offsets.h"
#include "externs.h"

/* sort-n-lists: basic explanation of how one could figure out how to
   code this, followed by actual verion. */

#if 0 
{
    /* Normal way to insertion sort:  This is how it's done below... :) */
    for (pos = 1; pos < elements; pos++) {
	for (cmp_pos = 0; cmp_pos < pos; cmp_pos++) {
	    if (!cmp_func(cmp_pos, pos)) break;
	}
	insert(pos, cmp_pos);
    }
    
    /* Now twist your mind a bit as the above is rewritten.  It does add an
       extra insert(0, 0) onto the very beginning.  No real loss. */

    pos = 0;
    cmp_pos = 0;
    while (pos < elements) {
	insert(pos, cmp_pos);
	pos++;
	cmp_pos = 0;
	result = cmp_func(cmp_pos, pos);
	while (cmp_pos < pos && result) {
	    cmp_pos++;
	    result = cmp_func(cmp_pos, pos);
	}
    }

    /* Now encode the above recursively: */
    func inner_loop {
	if (cmp_pos >= pos || !result) return;
	cmp_pos++;
	result = cmp_func(cmp_pos, pos);
	inner_loop();
    }

    func outer_loop {
	if (pos >= elements) return;
	insert(pos, cmp_pos);
	pos++;
	cmp_pos = 0;
	result = cmp_func(cmp_pos, pos);
	inner_loop();
	outer_loop();
    }

    func sort {
	pos = 0;
	cmp_pos = 0;
	outer_loop();
    }

    /* It is really programmed below like this: */

    func inner_loop {
	if (cmp_pos >= pos || !result) return;
	cmp_pos++;
	SETUP inner_loop;
	result = cmp_func(cmp_pos, pos);
    }

    func outer_loop {
	if (pos >= elements) return;
	insert(pos, cmp_pos);
	pos++;
	cmp_pos = 0;
	SETUP outer_loop;
	SETUP inner_loop;
	result = cmp_func(cmp_pos, pos);
    }

    func sort {
	pos = 0;
	cmp_pos = 0;
	SETUP outer_loop;
    }

}
#endif

#ifdef PRIM_sort_n_lists

struct snl_data {
    struct func_addr *func;	/* User function. */
    int num_lists;		/* Number of lists of data. */
    int elements;		/* Number of elements in each list. */
    int depth;			/* Expected depth of the stack. */
    int position;		/* Current location while going down array. */
    int cmp_pos;		/* What we are comparing against. (sort) */
};

#define SNL_SETUP_CONT(func) \
    do { push_context_cont(fr, func, data); if (fr->error) return; } while (0)

#define CHK_ADDR_DEPTH() \
    do { \
        if (st->depth > data->depth) { \
            interp_error(fr, data->prim, \
			 "Address left too much (%i) on stack", \
			 st->depth - data->depth); \
            return; \
        } else if (st->depth < data->depth) { \
            interp_error(fr, data->prim, \
			 "Address took too much (%i) off stack", \
			 data->depth - st->depth); \
	     return; \
	} \
    } while (0)

static void snl_insert(data_stack *st, struct snl_data *data)
{
    int i;
    inst *curr_pos, *curr_cmp_pos, *move;
    inst temp;

    curr_pos = data->start + data->position;
    curr_cmp_pos = data->start + data->cmp_pos;

    for (i = 0; i < data->num_lists; i++) {
	temp = *curr_pos;
	for (move = curr_pos; move > curr_cmp_pos; move--)
	    *move = *(move-1);
	*curr_cmp_pos = temp;

	curr_pos += data->elements + 1;
	curr_cmp_pos += data->elements + 1;
    }

}


static void snl_do_compare(frame *fr, data_stack *st, struct snl_data *data)
{
    SNL_PUSH_DATA(data->cmp_pos);
    SNL_PUSH_DATA(data->position);
    push_context_muf(fr, data->func);
}


static void snl_cleanup(frame *fr, data_stack *st, void *vdata)
{
    struct snl_data *data = vdata;
    inst temp;

    if (fr) {
	temp.type = INST_INTEGER;
	temp.un.integer = data->num_lists;

	safe_push_data_st(fr, st, &temp, PRIM_sort_n_lists);
    }

    clear_code(data->func->code);
    FREE(data);
}


static void snl_inner_loop(frame *fr, data_stack *st, void *vdata)
{
    struct snl_data *data = vdata;
    inst result;

    if (!fr) return;		/* Some error occured. */

    depth_delta = st->depth - (data->depth + 1);

    CHK_ADDR_DEPTH(data->depth + 1, PRIM_sort_n_lists);

    pop_data_st(st, &result);

    if (result.type != INST_INTEGER) {
	clear_inst_interp(&result);
	interp_error(fr, PRIM_sort_n_lists,
		     "Comparison address did not return an int.");
	return;
    }

    if (result.un.integer < 0) return;

    data->cmp_pos++;

    /* Check if we've found the correct location, return if so. */
    if (data->cmp_pos >= data->position)
	return;

    SNL_SETUP(snl_inner_loop);

    snl_do_compare(fr, st, data);
}


static void snl_outer_loop(frame *fr, data_stack *st, void *vdata)
{
    struct snl_data *data = vdata;

    if (!fr) return;		/* Some error occured. */

    snl_insert(st, data);

    data->position++;
    data->cmp_pos = 0;

    if (data->position >= data->elements) return; /* All sorted, whee. */

    SNL_SETUP(snl_outer_loop);
    SNL_SETUP(snl_inner_loop);

    snl_do_compare(fr, st, data);
}


void prim_sort_n_lists(frame *fr, data_stack *st)
{
    struct snl_data *data;
    inst func, 
    
    SNL_SETUP(snl_cleanup);
    SNL_SETUP(snl_outer_loop);
}
#endif /* sort-n-lists */
