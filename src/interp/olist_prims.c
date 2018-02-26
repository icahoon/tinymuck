/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* olist_prims.c,v 2.4 1996/01/26 01:04:17 dmoore Exp */
#if defined(PRIM_map_n_lists) || defined(PRIM_apply_n_lists) || defined(PRIM_sort_n_lists)

#include "config.h"

#include "db.h"
#include "code.h"
#include "prim_offsets.h"
#include "externs.h"

/* This structure is used by all of the *-n-lists routines.  It's sufficient
   for map and apply, and has an extra field which only sort uses. */
struct nl_prim_data {
    int prim;			/* Primitive being used. */
    struct func_addr *func;	/* User function. */
    int num_lists;		/* Number of lists of data. */
    int elements;		/* Number of elements in each list. */
    int width;			/* Total width of all data on stack. */
    int depth;			/* Expected depth of the stack. */
    int position;		/* Current location while going down array. */
    int cmp_pos;		/* What we are comparing against. (sort) */
};

#define SETUP_CONT(func) \
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


static void nl_prim_cleanup(frame *fr, data_stack *st, void *vdata)
{
    struct nl_prim_data *data = vdata;

    clear_code(data->func->code);
    FREE(data);
}


#define ONE_CONT_FUNC	1
#define ONE_SET_DATA	1
#define TWO_SET_DATA	2
#define ADDR_NO_RETURN	0
#define ADDR_ONE_RETURN	1

/* All the primitives share common initialization code. */
static struct nl_prim_data *nl_prim_init(frame *fr, data_stack *st, const int cont_overhead, const int data_overhead_mul, const inst result_overhead, const int prim)
{
    inst func, count;
    int num_lists, elements, width;
    int data_overhead;
    struct nl_prim_data *data;

    pop_data_st(st, &func);
    pop_data_st(st, &count);

    num_lists = count.un.integer;
    elements = top_data_st(st)->un.integer; /* Size of sub lists. */
    width = num_lists * (elements + 1);	/* Total # of items in lists. */
    data_overhead = (num_lists * data_overhead_mul) + result_overhead;

    if (num_lists == 0) {
	/* If it's empty we don't really have to do much. */
	return NULL;
    }

    if (!same_depth_lists(st, num_lists, elements)) {
	clear_inst_interp(&func);
	interp_error(fr, prim, "All lists must have the same depth");
	return NULL;
    }

    /* Check that there will be enough room to push all the data over
       the entire run of the program. */
    if ((st->depth + data_overhead) > DATA_STACK_SIZE) {
	clear_inst_interp(&func);
	interp_error(fr, prim, "Data stack too full for operation");
	return NULL;
    }

    /* 2+ is for the cleanup address we push here, and the muf addr. */
    if ((fr->addr_stack->depth + addr_overhead + 2) > ADDR_STACK_SIZE) {
	clear_inst_interp(&func);
	interp_error(fr, prim, "Address stack too full for operation");
	return NULL;
    }

    MALLOC(data, struct nl_prim_data, 1);

    data->prim = prim;
    data->func = func.un.address;
    data->num_lists = num_lists;
    data->elements = elements;
    data->width = width;
    data->depth = st->depth + data_overhead;
    data->position = 0;
    data->cmp_pos = 0;

    SETUP_CONT(nl_prim_cleanup);

    return data;
}


static void nl_push_data(frame *fr, data_stack *st, struct nl_prim_data *data, const int pos)
{
    int i;
    inst *top = top_data_st(st);
    inst *start;

    for (i = 0; i < data->num_lists; i++) {
	/* Yes, there is a nicer (and more optimal way to do this.
	   Unfortunately it's not ansi C cause it will go off the beginning
	   of the array on the last iteration. */
	start = top - (i * (data->elements + 1));
	push_data_st(st, start[-pos]);
    }
}


#ifdef PRIM_map_n_lists
static void mnl_loop(frame *fr, data_stack *st, void *vdata)
{
    struct nl_prim_data *data = vdata;

    /* Some sort of interp error, let the cleanup function handle it. */
    if (!fr) return;

    /* We have processed all of the data. */
    if (data->position >= data->num_lists) return;

    /* Make sure that the address has done the proper things. */
    CHK_ADDR_DEPTH();

    /* Push on the desired data. */
    nl_push_data(fr, st, data, data->position);

    /* Slide down the remaining portions.  This alters data->depth. */
    map_list_shift(fr, st, data, data->position);

    /* Setup next iteration, and then setup for the address call. */
    SETUP_CONT(mnl_loop);
    push_context_muf(fr, data->func);
}


void prim_map_n_lists(frame *fr, data_stack *st)
{
    struct nl_prim_data *data;

    data = nl_prim_init(fr, st,
			ONE_CONT_FUNC, ONE_SET_DATA, ADDR_NO_RETURN,
			PRIM_map_n_lists);
    if (!data) return;

    SETUP_CONT(mnl_loop);
}
#endif /* map-n-lists */

#ifdef PRIM_apply_n_lists
#endif /* apply-n-lists */


#if 0 
{
    /* Normal way to insertion sort:  This is how I do it. :) */
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
    
    SNL_SETUP(snl_cleanup);
    SNL_SETUP(snl_outer_loop);
}
#endif /* sort-n-lists */

#endif /* map-n-lists, apply-n-lists, or sort-n-lists */
