/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* code.c,v 2.7 1997/08/30 06:44:01 dmoore Exp */
#include "config.h"

#include "db.h"
#include "code.h"
#include "params.h"
#include "externs.h"


struct code_entry {
    dbref program;
    struct code *code;
};


/* Hash table of programs currently being edited. */
static hash_tab *code_table = NULL;


/* Hash table routines.  Maps a program # -> codes. */
static unsigned long hash_code_entry(const void *entry)
{
    const struct code_entry *info = entry;

    return dbref2long(info->program);
}


static int compare_code_entries(const void *entry1, const void *entry2)
{
    const struct code_entry *info1 = entry1;
    const struct code_entry *info2 = entry2;

    return (info1->program != info2->program);
}


static void delete_code_entry(void *entry)
{
    struct code_entry *info = entry;

    /* #### i think this should call clear_code? */
    clear_code(info->code);
    FREE(info);
}


void init_code_table(void)
{
    if (code_table) return;

    code_table = init_hash_table("Prog -> Code Segment",
				 compare_code_entries,
				 hash_code_entry, delete_code_entry,
				 CODE_HASH_SIZE);
}


void clear_code_table(void)
{
    clear_hash_table(code_table);
    code_table = NULL;
}


struct code *get_code(const dbref program)
{
    struct code_entry match;
    const struct code_entry *result;

    match.program = program;
    result = find_hash_entry(code_table, &match);
    
    if (!result) return NULL;

    return result->code;
}


void add_code(const dbref program, struct code *code)
{
    struct code_entry match;
    struct code_entry *info;

    match.program = program;
    info = (struct code_entry *) find_hash_entry(code_table, &match);
    
    if (!info) {
	MALLOC(info, struct code_entry, 1);
	info->program = program;
	info->code = NULL;
	add_hash_entry(code_table, info);
    }
    /* First up the ref count on the new code, and down the previous. */
    code = dup_code(code);
    if (info->code) clear_code(info->code);

    code->next = info->code;
    code->prev = &info->code; 
    if (info->code)
	info->code->prev = &code->next;
    info->code = code;
}


/* This is for recycling a program. */
void delete_code(const dbref program)
{
    struct code_entry match;
    struct code *first;
    struct code *temp;

    first = get_code(program);
    if (!first) return;

    clear_code(first);

    first = get_code(program);

    if (first) {
	/* Something still here.  Detach the code chain, and mark
	   each item so that the interpreter kicks people out of them. */
	first->prev = NULL;
 
        for (temp = first; temp; temp = temp->next)
            temp->object = NOTHING;
    }

    match.program = program;
    clear_hash_entry(code_table, &match);
}


/*  make_code:  creates an empty code block
 */
struct code *make_code(dbref obj, int num_funcs)
{
    struct code *result;

    MALLOC(result, struct code, 1);
    result->refcount = 1;
    result->object = obj;
    result->num_funcs = num_funcs;
    if (num_funcs)
        MALLOC(result->funcs, struct func_addr *, num_funcs);
    else
        result->funcs = NULL;
    result->next = NULL;
    result->prev = NULL;
    return result;
}


/*  dup_code: this ups the refcount on a code block.
 */
struct code *dup_code(struct code *code)
{
    if (code) code->refcount++;
    return code;
}


/*  clear_code:  tells the world you're done with whatever you were doing
 *  with this struct code.  may or may not cause it to be flushed from
 *  memory. this is called from outside the compiler, during recycles, etc.
 */
void clear_code(struct code *code)
{
    int x, y;

    if (!code) return;

    code->refcount--;
    if (code->refcount > 0) return;
    if (code->refcount < 0) {
	log_status("CODE: refcount on code struct below 0");
	return;
    }
	

    for (y = code->num_funcs; y; --y) {
	struct func_addr *victim = code->funcs[y-1];

	for (x = 0; x < victim->num_inst; x++)
	    clear_inst_code(&(victim->bytecode[x]));
    }

    for (y = code->num_funcs; y; --y) {
        struct func_addr *victim = code->funcs[y-1];

	FREE(victim->bytecode);

        if (victim->name)
            FREE_STRING(victim->name);
        FREE(victim);
    }

    if (code->funcs)
        FREE(code->funcs);

    /* unlink this code from it's list. */
    if (code->prev)
	*(code->prev) = code->next;
    if (code->next)
	code->next->prev = code->prev;

    FREE(code);
}


/*  get_code_size: finds out the # of functions and instructions for
 *  a code block.
 */
void get_code_size(struct code *code, int *num_func, int *num_inst)
{
    int funcs = 0;
    int insts = 0;
    int i;

    if (code) {
	funcs = code->num_funcs;
	for (i = 0; i < funcs; i++) {
	    insts += code->funcs[i]->num_inst;
	}
    }
    *num_func = funcs;
    *num_inst = insts;
}
	

