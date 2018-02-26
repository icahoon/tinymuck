/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* intern.h,v 2.5 1997/08/29 21:00:18 dmoore Exp */
#ifndef MUCK_INTERN_H
#define MUCK_INTERN_H

struct interned_string {
    union {
        struct {
            int refcount;
            const char *string;
        } valid;
        struct interned_string *next;
    } un;
};

#define istring_text(x)		FORCE_RHS((x)->un.valid.string)

#endif /* MUCK_INTERN_H */
