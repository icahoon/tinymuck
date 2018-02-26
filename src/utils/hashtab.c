/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* hashtab.c,v 2.7 1997/08/10 04:32:42 dmoore Exp */
#include "config.h"

#include <stdlib.h>		/* NULL */

#include "db.h"
#include "hashtab.h"
#include "externs.h"


struct hash_tab {
    const char *id;		/* ''Name'' of hashtable, for stat display. */

    compare_function cmpfn;	/* Function to compare two entries. */
    key_function     keyfn;	/* Function to generate a key for an entry. */
    delete_function  delfn;	/* Function called when freeing an entry. */

    unsigned long size;		/* Size of the hashtable. */
    struct hash_entry **table;	/* Actual hashtable array. */

    unsigned long walk_key;	/* Current bucket when walking hash table. */
    struct hash_entry *walk_loc; /* Current entry when walking hash table. */

    unsigned long entries;	/* Total number of entries. */
    unsigned long max_entries;	/* Max # of entries ever stored. */

    unsigned long hits;		/* Total hits. */
    unsigned long hits_visited;	/* Total number of entries visited on hits. */
    
    unsigned long misses;	/* total misses. */
    unsigned long misses_visited; /* Total number of entried visited misses. */

    unsigned long total_insert;	/* Total number of entries ever inserted. */

    struct hash_tab *next;	/* Keep all hashtables in linked list. */
    struct hash_tab **prev;
};


struct hash_entry {
    struct hash_entry *next;	/* Linked list buckets for collisions. */
    const void	      *data;	/* Actual entry we are storing. */
};


/* Linked list of all hash tables we are managing. */
static struct hash_tab *hash_table_list = NULL;

/* Linked list of reusable hash_entry structures. */
static struct hash_entry *available_hash_entries = NULL;
static long total_entries = 0;


/* These are the generic hashtable routines.  They _require_ that the first
   member of the entry be a string, and that string is taken as the key.
   The generic delete function simply free's it's argument. */
int compare_generic_hash(const void *entry1, const void *entry2)
{
    return muck_stricmp(*(const char **) entry1, *(const char **) entry2);
}


unsigned long make_key_generic_hash(const void *entry)
{
    return default_hash(*(const char **) entry);
}


void delete_generic_hash(void *entry)
{
    FREE(entry);
}


static unsigned long compute_hash(hash_tab *table, const void *data)
{
    return (table->keyfn(data) % table->size);
}


static void generate_hash_entries(long count)
{
    struct hash_entry *hunk;
    int num_4k;
    int i, j;
 
    /* Try allocating them in 4K sets.  The -4 is for some mallocs
       which want to stick some overhead in the space.  This keeps
       things on a single page (hopefully). */
    num_4k = (4096 - 4) / sizeof(struct hash_entry);
 
    count = (count / num_4k) + 1;
 
    for (i = 0; i < count; i++) {
        MALLOC(hunk, struct hash_entry, num_4k);
        for (j = 0; j < num_4k; j++) {
            hunk->next = available_hash_entries;
            available_hash_entries = hunk;
            hunk++;
        }
    }
}


static struct hash_entry *make_new_hash_entry(void)
{
    struct hash_entry *result;

    if (!available_hash_entries) {
	generate_hash_entries(1);
    }
    result = available_hash_entries;
    available_hash_entries = result->next;

    total_entries++;

    return result;
}


static void free_hash_entry(struct hash_entry *entry)
{
    entry->next = available_hash_entries;
    available_hash_entries = entry;

    total_entries--;
}



static struct hash_entry *find_hash_internal(hash_tab *table, const unsigned long hashval, const void *match)
{
    struct hash_entry *curr;
    struct hash_entry **prev;
    compare_function cmpfn;
    int visits = 0;
    
    if (!table) return NULL;

    cmpfn = table->cmpfn;

    prev = &(table->table[hashval]);
    for (curr = *prev; curr; prev = &(curr->next), curr = curr->next) {
	visits++;
	if (cmpfn(match, curr->data) == 0) {
	    /* Move this entry to the head of the bucket. */
	    *prev = curr->next;
	    curr->next = table->table[hashval];
	    table->table[hashval] = curr;

	    /* Mark this hit for statistics. */
	    table->hits_visited += visits;
	    table->hits++;

	    return curr;
	}
    }

    table->misses_visited += visits + 1;
    table->misses++;

    return NULL;                  /* not found */
}


const void *find_hash_entry(hash_tab *table, const void *match)
{
    struct hash_entry *entry;

    entry = find_hash_internal(table, compute_hash(table, match), match);

    return (entry) ? (entry->data) : NULL;
}


void add_hash_entry(hash_tab *table, const void *data)
{
    struct hash_entry *entry;
    unsigned long hashval;

    if (!table) return;
    
    hashval = compute_hash(table, data);
    
    entry = find_hash_internal(table, hashval, data);

    if (entry) {
	/* Free old entry. */
	table->delfn((void *) entry->data);
    } else {
	entry = make_new_hash_entry();
	entry->next = table->table[hashval];
	table->table[hashval] = entry;

	table->entries++;
	if (table->entries > table->max_entries)
	    table->max_entries = table->entries;
	table->total_insert++;
    }
    
    /* Either we just created it, or we already found a valid one. */
    entry->data = data;
}


void clear_hash_entry(hash_tab *table, const void *data)
{
    struct hash_entry *entry;
    unsigned long hashval;

    if (!table) return;

    hashval = compute_hash(table, data);
    
    entry = find_hash_internal(table, hashval, data);

    if (!entry) return;

    /* Found one.  It's now at front of bucket. */
    table->table[hashval] = entry->next;

    table->entries--;

    table->delfn((void *) entry->data);
    free_hash_entry(entry);
}


void clear_hash_table(hash_tab *table)
{
    struct hash_entry **temp;	/* In the table array. */
    struct hash_entry *entry, *next;
    unsigned long i, size;
    
    if (!table) return;

    size = table->size;
    temp = table->table;
    for (i = 0; i < size; i++, temp++) {
	for (entry = *temp; entry; entry = next) {
	    next = entry->next;	/* Save it, since we free next. */
	    table->delfn((void *) entry->data);
	    free_hash_entry(entry);
	}
    }

    /* Remove from our linked list of tables. */
    *(table->prev) = table->next;
    if (table->next)
	table->next->prev = table->prev;

    FREE(table->table);
    FREE(table);
}


hash_tab *init_hash_table(const char *id, compare_function cmpfn, key_function keyfn, delete_function delfn, unsigned long size)
{
    hash_tab *table;
    struct hash_entry **temp;
    unsigned long i;

    /* Malloc everything. */
    MALLOC(table, hash_tab, 1);
    MALLOC(table->table, struct hash_entry *, size);

    /* Clear out the actual table. */
    temp = table->table;
    for (i = 0; i < size; i++, temp++)
	*temp = NULL;

    /* Fill in everything. */
    table->id = id;
    table->cmpfn = cmpfn;
    table->keyfn = keyfn;
    table->delfn = delfn;
    table->size = size;
    table->entries = 0;
    table->max_entries = 0;
    table->hits = 0;
    table->hits_visited = 0;
    table->misses = 0;
    table->misses_visited = 0;
    table->total_insert = 0;

    /* Stick in our linked list of hash tables. */
    if (hash_table_list)
	hash_table_list->prev = &(table->next);
    table->next = hash_table_list;
    table->prev = &hash_table_list;
    hash_table_list = table;
    
    return table;
}


/* Don't do _anything_ else to this hash table when walking it.  This
   includes looking for other things, etc. */
void init_hash_walk(hash_tab *table)
{
    table->walk_key = 0;
    table->walk_loc = NULL;
}


const void *next_hash_walk(hash_tab *table)
{
    struct hash_entry **temp;
    const void *result;
    unsigned long i, size;

    if (table->walk_loc) {
	result = table->walk_loc->data;
	table->walk_loc = table->walk_loc->next;
	return result;
    }

    size = table->size;
    temp = table->table + table->walk_key;

    for (i = table->walk_key; i < size; i++, temp++) {
	if (*temp) break;
    }

    if (i == table->size) return NULL;

    result = (*temp)->data;
    table->walk_loc = (*temp)->next;
    table->walk_key = i + 1;

    return result;
}


void dump_hashtab_stats(hash_tab *table)
{
    int doing_all = 0;
    long total_ratio, hit_ratio, miss_ratio;
    double t;

    if (table == NULL) {
	/* Dump information on all of the hashtables. */
	doing_all = 1;
	table = hash_table_list;
    }

    for (; table; table = table->next) {
	if (table->hits_visited + table->misses_visited) {
	    t = 1000 * (((double) table->hits_visited + (double) table->misses_visited) / ((double) table->hits + (double) table->misses));
	    total_ratio = t;
	} else total_ratio = 0;
	if (table->hits_visited) {
	    t = 1000 * ((double) table->hits_visited / (double) table->hits);
	    hit_ratio = t;
	} else hit_ratio = 0;
	if (table->misses_visited) {
	    t = 1000 * ((double) table->misses_visited / (double) table->misses);
	    miss_ratio = t;
	} else miss_ratio = 0;

	log_info("");
	log_info("--- Statistics for: %s",
		 table->id);
	log_info("Table size: %i.  Entries: %i.",
		 table->size, table->entries);
	log_info("Total accesses: %i.",
		 table->hits + table->misses);
	log_info("Hit accesses: %i.  Miss accesses: %i.",
		 table->hits, table->misses);
	log_info("Bucket probes requiring comparison: %i",
		 table->hits_visited + table->misses_visited);
	log_info("Hit compares: %i.  Miss compares: %i.",
		 table->hits_visited, table->misses_visited);
	log_info("Total access ratio: %i (/1000)",
		 total_ratio);
	log_info("Hit ratio: %i.  Miss ratio: %i. (/1000)",
		 hit_ratio, miss_ratio); 
	log_info("Max entries stored: %i.",
		 table->max_entries);

	if (!doing_all) break;
    }
}


long total_hash_entries(void)
{
    return total_entries;
}


void init_hash_entries(const long count)
{
    generate_hash_entries(count);
}
