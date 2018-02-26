/* hashtab.h,v 2.3 1993/04/08 20:24:47 dmoore Exp */
#ifndef MUCK_HASHTAB_H
#define MUCK_HASHTAB_H

/* Comparison function used to check if two hash table entries are the same.
   Should return 0 if the same, non-zero otherwise. */
typedef int (*compare_function)(const void *, const void *);

/* Key generating function.  Return the key for a hash table entry. */
typedef unsigned long (*key_function)(const void *);

/* Function to be called when a hash entry is to be deleted. */
typedef void (*delete_function)(void *);

#endif /* MUCK_HASHTAB_H */
