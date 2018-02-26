/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* muck_malloc.c,v 2.8 1996/01/25 20:37:44 dmoore Exp */
#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "externs.h"

#define MAX_HUNK_SIZE 64 /* Must be a multiple of hunk_node size. */

#ifdef DEBUG_MALLOCS
#include "debug_malloc.h"
#define malloc(x)	debug_malloc(x, file, line)
#define free(x)		debug_free(x, file, line)
#define realloc(x, y)	debug_realloc(x, y, file, line)
#endif


/* For strings, we'll allocate them in larger hunks of memory and then
   cut those up and hand them out.  This saves on malloc overhead. */
/* We make the assumption here that &(node.data) == &(node.next). */
/* Luckily ansi seems to say this. :) */
union hunk_node {
    union hunk_node *next;
    char align_force[ALIGN_BYTES];
    char data[1];
};

static int initialized = 0;
static union hunk_node *free_list[MAX_HUNK_SIZE + 1];


void *muck_realloc(void *cp, size_t size, const char *file, const int line)
{
    void *result;

    result = realloc(cp, size);

    if (!result) panic("Out of memory.");

    return result;
}


void *muck_malloc(size_t size, const char *file, const int line)
{
    void *result;

    result = malloc(size);

    if (!result) panic("Out of memory.");

    return result;
}
    

void muck_free(void *cp, const char *file, const int line)
{
    free(cp);
}


/* Make lenth a multiple of the needed alignment. */
#define AlignLen(l) \
   (((l) % sizeof(union hunk_node)) ? \
	(l) + sizeof(union hunk_node) - ((l) % sizeof(union hunk_node)) : \
	(l))


static void init_alloc_strings(void)
{
    int i;

    for (i = 0; i < MAX_HUNK_SIZE+1; i++)
	free_list[i] = NULL;

    initialized = 1;
}


static void generate_strings(const int len, const char *file, const int line)
{
    union hunk_node *hunk;
    int num_4k;
    int i;

    /* Try allocating them in 4K sets.  The -4 is for some mallocs
       which want to stick some overhead in the space.  This keeps
       things on a single page (hopefully). */
    num_4k = (4096 - ALIGN_BYTES) / len;

    hunk = malloc(4906 - ALIGN_BYTES);
    for (i = 0; i < num_4k; i++) {
	hunk->next = free_list[len];
	free_list[len] = hunk;
	hunk = (union hunk_node *) (len + (char *) hunk);
    }

    /* If len wasn't of a size to fit perfectly, stick extra space
       onto a smaller list. */
    i = (4096 - ALIGN_BYTES) - (num_4k * len);
    if (AlignLen(i) != i)
	i = AlignLen(i) - sizeof(union hunk_node);
    if (i > 0) {
	hunk->next = free_list[i];
	free_list[i] = hunk;
    }
}


static char *alloc_string(unsigned int len, const char *file, const int line)
{
    union hunk_node *head;

#ifdef FIX_FIX_FIX_DEBUG_MALLOCS
    return (char *) debug_malloc(len + 1, file, line);
#else
    len = len + 1;			/* for null byte */
    /* Decide to allocate a hunk or just malloc directly based on size. */
    if (len <= MAX_HUNK_SIZE) {
	if (!initialized) init_alloc_strings();

	len = AlignLen(len);

	if (!free_list[len]) generate_strings(len, file, line);

	head = free_list[len];
	free_list[len] = head->next;

	return (char *) head;
    } else {
	return (char *) malloc(len);
    }
#endif /* DEBUG_MALLOCS */
}


void muck_free_cnt_string(const char *s, unsigned int len, const char *file, const int line)
{
    union hunk_node *head;

#ifdef FIX_FIX_FIX_DEBUG_MALLOCS
    debug_free((char *) s, file, line);
#else
    len = len + 1;			/* null byte */
    if (len <= MAX_HUNK_SIZE) {
	len = AlignLen(len);

	head = (union hunk_node *) ((char *) s);
	head->next = free_list[len];
	free_list[len] = head;
    } else {
	free((char *) s);
    }
#endif /* DEBUG_MALLOCS */
}


void muck_free_string(const char *s, const char *file, const int line)
{
    muck_free_cnt_string(s, strlen(s), file, line);
}


const char *muck_alloc_cnt_string(const char *s, const unsigned int len, const char *file, const int line)
{
    char *result;

    if (s == NULL || *s == '\0' || len == 0) return NULL;

    result = (char *) alloc_string(len, file, line);
    strncpy(result, s, len + 1);

    return (const char *) result;
}


const char *muck_alloc_cnt2_string(const char *s1, const unsigned int len1, const char *s2, const unsigned int len2, const char *file, const int line)
{
    char *result;

    if ((s1 == NULL || s1 == '\0') && (s2 == NULL || s2 == '\0'))
	return NULL;

    if ((len1 + len2) == 0) return NULL;

    result = (char *) alloc_string(len1 + len2, file, line);
    strncpy(result, s1, len1 + 1);
    strncpy(result+len1, s2, len2 + 1);

    return (const char *) result;
}


const char *muck_alloc_string(const char *s, const char *file, const int line)
{
    if (s == NULL || *s == '\0') return NULL;

    return muck_alloc_cnt_string(s, strlen(s), file, line);
}


