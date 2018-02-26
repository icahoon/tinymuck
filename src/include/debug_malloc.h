/* debug_malloc.h,v 2.3 1993/04/08 20:24:47 dmoore Exp */
#ifndef DEBUG_MALLOC_H
#define DEBUG_MALLOC_H

extern void *debug_realloc(void *, size_t, const char *, const int);
extern void *debug_malloc(size_t, const char *, const int);
extern void debug_free(void *, const char *, const int);
extern void *debug_calloc(size_t, const char *, const int);
extern void debug_mstats(const char *);

/* Possible defines to allow easy integration into existing code.  Install
   these _after_ including stdlib.h on ansi systems. */
/*
#define malloc(x)	debug_malloc(x, __FILE__, __LINE__)
#define realloc(x, y)	debug_realloc(x, y, __FILE__, __LINE__)
#define free(x)		debug_free(x, __FILE__, __LINE__)
#define calloc(x)	debug_calloc(x, __FILE__, __LINE__)
*/

#endif /* DEBUG_MALLOC_H */

