/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* muck_malloc.h,v 2.6 1994/01/03 04:09:40 dmoore Exp */
#ifndef MUCK_MALLOC_H
#define MUCK_MALLOC_H

extern void *muck_realloc(void *, size_t, const char *, const int);
extern void *muck_malloc(size_t, const char *, const int);
extern void muck_free(void *, const char *, const int);
extern const char *muck_alloc_string(const char *, const char *, const int);
extern const char *muck_alloc_cnt_string(const char *, const unsigned int, const char*, const int);
extern const char *muck_alloc_cnt2_string(const char *, const unsigned int, const char*, const unsigned int, const char *, const int);
extern void muck_free_string(const char *, const char *, const int);
extern void muck_free_cnt_string(const char *, unsigned int, const char *, const int);

/* This works with some compilers and will save some space in the object
   code. On other compilers __FILE__ will have 'muck_malloc.h' rather
   than the .c file's name. */

/*
static const char Muck_Malloc_FILE[] = __FILE__;
*/
#define Muck_Malloc_FILE __FILE__

#define REALLOC(r, t, c) \
    ((r) = (t *) muck_realloc(r, sizeof(t)*(c), Muck_Malloc_FILE, __LINE__))

#define MALLOCEXACT(r, t, s) \
    ((r) = (t *) muck_malloc(s, Muck_Malloc_FILE, __LINE__))

#define MALLOC(r, t, c) \
    ((r) = (t *) muck_malloc(sizeof(t)*(c), Muck_Malloc_FILE, __LINE__))

#define FREE(r) \
    do { muck_free(r, Muck_Malloc_FILE, __LINE__); (r) = 0; } while (0)

/* Allocate a new copy of the string. */
#define ALLOC_STRING(x) \
    muck_alloc_string(x, Muck_Malloc_FILE, __LINE__)

/* Allocate a copy of the string of given size. */
#define ALLOC_CNT_STRING(x, y) \
    muck_alloc_cnt_string(x, y, Muck_Malloc_FILE, __LINE__)

#define ALLOC_CNT2_STRING(x1, y1, x2, y2) \
    muck_alloc_cnt2_string(x1, y1, x2, y2, Muck_Malloc_FILE, __LINE__)

#define FREE_STRING(x) \
    do { muck_free_string(x, Muck_Malloc_FILE, __LINE__); (x) = 0; } while (0)

#define FREE_CNT_STRING(x, y) \
    do { \
	muck_free_cnt_string(x, y, Muck_Malloc_FILE, __LINE__); \
	(x) = 0; \
    } while (0)

#endif /* MUCK_MALLOC_H */
