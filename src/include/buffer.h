/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* buffer.h,v 2.8 1997/08/29 21:00:11 dmoore Exp */
#ifndef MUCK_BUFFER_H
#define MUCK_BUFFER_H

/* Written 2/2/92 by dmoore to provide a simple package for buffer
   overflow protection for tinymuck.  Instead of occurances of
   char buf[BUFFER_LEN] all over the place, the routines which need
   buffers should use Buffer buf, and the routines below to manipulate
   the buffers. */

#include <stdarg.h>
#include <time.h>

/* Traditionally BUFFER_LEN has been ((MAX_COMMAND_LEN)*8), where
   M_C_L is defined in interface.h, and is 512, making BUFFER_LEN 4096. */
#define BUFFER_LEN 4096

typedef struct Buffer {
    int  size;
    char data[BUFFER_LEN];
} Buffer;

extern void Bufcat(Buffer *, const char *);
extern void Bufcatlist(Buffer *, ...);
extern void Bufcpy(Buffer *, const char *);
extern void Bufcat_int(Buffer *, const long);
extern void Bufcat_boolexp(Buffer *, const dbref, struct boolexp *, const int);
extern void Bufcat_mucktime(Buffer *, const char *, long, const int);
extern void Bufncat(Buffer *, const char *, unsigned int);
extern void Bufncpy(Buffer *, const char *, unsigned int);
extern void Bufsprint(Buffer *, const char *, ...);
extern void Bufvsprint(Buffer *, const char *, va_list);
extern void Bufsetchar(Buffer *, unsigned int, const char);
extern void Bufstw(Buffer *);
extern void Bufaddnl(Buffer *);
extern void Bufcat_dbref(Buffer *, const dbref);
extern void Bufcat_name(Buffer *, const dbref);
extern void Bufcat_unparse(Buffer *, const dbref, const dbref);
extern void Bufdel_char(Buffer *);

/* We declare it void not FILE, so that not everyone needs to include
   stdio.h. */
extern char *Bufgets(Buffer *, void *, int *);
extern void Bufputs(Buffer *, void *, int *);

#define Bufcat_char(buf, c) do { \
    Buffer *_b = (buf); \
    if (_b->size < BUFFER_LEN - 1) { \
        char *_p = _b->data + _b->size; \
        *_p++ = (c); \
        *_p = '\0'; \
        _b->size++; \
    } else { \
        ((void) (c)); /* Dummy to insure side-effects. */ \
    } \
} while (0)

#define Buftext(buf) FORCE_RHS((buf)->data)
#define Buflen(buf)  FORCE_RHS((buf)->size)

#endif /* MUCK_BUFFER_H */
