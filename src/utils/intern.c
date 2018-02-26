/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* intern.c,v 2.7 1993/12/19 14:33:25 dmoore Exp */
#include "config.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "db.h"
#include "params.h"
#include "buffer.h"
#include "hashtab.h"
#include "intern.h"
#include "externs.h"


static long total_istrings;
static hash_tab *intern_table = NULL;
static struct interned_string *available_istrings = NULL;


static void generate_istrings(long count)
{
    struct interned_string *hunk;
    int num_4k;
    int i, j;
 
    /* Try allocating them in 4K sets.  The -4 is for some mallocs
       which want to stick some overhead in the space.  This keeps
       things on a single page (hopefully). */
    num_4k = (4096 - 4) / sizeof(struct interned_string);
 
    count = (count / num_4k) + 1;
 
    for (i = 0; i < count; i++) {
        MALLOC(hunk, struct interned_string, num_4k);
        for (j = 0; j < num_4k; j++) {
            hunk->un.next = available_istrings;
            available_istrings = hunk;
            hunk++;
        }
    }
}


static int compare_istrings(const void *entry1, const void *entry2)
{
    const struct interned_string *istring1 = entry1;
    const struct interned_string *istring2 = entry2;

    return muck_stricmp(istring1->un.valid.string, istring2->un.valid.string);
}


static unsigned long make_key_istring(const void *entry)
{
    const struct interned_string *istring = entry;

    return default_hash(istring->un.valid.string);
}


static void delete_istring(void *entry)
{
    struct interned_string *what = entry;

    if (!what) return;

    total_istrings--;

    FREE_STRING(what->un.valid.string);

    what->un.next = available_istrings;
    available_istrings = what;
}


static struct interned_string *new_istring(const char *what)
{
    struct interned_string *result;

    if (!available_istrings) generate_istrings(1);

    result = available_istrings;
    available_istrings = result->un.next;

    total_istrings++;

    result->un.valid.string = ALLOC_STRING(what);
    result->un.valid.refcount = 0;

    return result;
}


struct interned_string *find_interned_string(const char *what)
{
    struct interned_string match;

    match.un.valid.string = what;

    return (struct interned_string *) find_hash_entry(intern_table, &match);
}


struct interned_string *make_interned_string(const char *what)
{
    struct interned_string match;
    struct interned_string *result;

    match.un.valid.string = what;

    result = (struct interned_string *) find_hash_entry(intern_table, &match);

    if (!result) {
        result = new_istring(what);
        add_hash_entry(intern_table, result);
    }

    result->un.valid.refcount++;

    return result;
}


struct interned_string *dup_interned_string(struct interned_string *what)
{
    if (!what) return NULL;

    what->un.valid.refcount++;

    return what;
}


void clear_interned_string(struct interned_string *what)
{
    if (!what) return;

    what->un.valid.refcount--;

    if (what->un.valid.refcount > 0)  return;

    clear_hash_entry(intern_table, what);
}


long total_intern_strings(void)
{
    return total_istrings;
}


void init_intern_strings(const long count)
{
    if (intern_table)  return;

    total_istrings = 0;
    intern_table = init_hash_table("Intern table", compare_istrings,
				   make_key_istring, delete_istring,
				   INTERN_HASH_SIZE);

    generate_istrings(count);
}
