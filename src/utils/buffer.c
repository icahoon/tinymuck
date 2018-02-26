/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* buffer.c,v 2.9 1997/08/09 21:12:30 dmoore Exp */

/* Written 2/2/92 by dmoore to provide a simple package for buffer
   overflow protection for tinymuck.  Instead of occurances of
   char buf[BUFFER_LEN] all over the place, the routines which need
   buffers should use Buffer buf, and the routines below to manipulate
   the buffers. */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "db.h"			/* We need dbref defined. */
#include "buffer.h"
#include "muck_time.h"
#include "externs.h"

#define MIN(a, b) ((a) < (b)) ? (a) : (b)

void Bufcat(Buffer *buf, register const char *str)
{
    register char *ptr;
    register unsigned len;
    
    if (!str || !*str) return;

    len = (BUFFER_LEN - 1) - buf->size;	/* Space left. */
    ptr = buf->data + buf->size;

    while (*str && len) {
        *ptr++ = *str++;
        len--;
    }

    *ptr = '\0';
    buf->size = (BUFFER_LEN - 1) - len;
}


void Bufcatlist(Buffer *buf, ...)
{
    va_list ap;
    const char *ptr;
    
    va_start(ap, buf);
    while (ptr = va_arg(ap, const char *))
        Bufcat(buf, ptr);
    va_end(ap);
}


void Bufcpy(Buffer *buf, const char *str)
{
    buf->size = 0;
    *(buf->data) = '\0';
    if (str && *str) Bufcat(buf, str); /* Don't call it for "". */
}


void Bufcat_boolexp(Buffer *buf, const dbref player, struct boolexp *bool, const int format)
{
    Bufcat(buf, unparse_boolexp(player, bool, format));
}


void Bufcat_int(Buffer *buf, const long x)
{
    char num[1024];		/* Big enough to hold a long? */
    
    sprintf(num, "%ld", x);
    Bufcat(buf, num);
}


void Bufcat_mucktime(Buffer *buf, const char *fmt, long t, const int use_local_time)
{
    register char *ptr;
    register unsigned len;
    
    if (!fmt) return;

    len = (BUFFER_LEN - 1) - buf->size;	/* Space left. */
    ptr = buf->data + buf->size;

    buf->size += muck_strftime(ptr, len, fmt, t, use_local_time);
}


void Bufncat(Buffer *buf, register const char *str, register unsigned len)
{
    register char *ptr;
    register int cnt;
 
    if (!str || !*str) return;
 
    cnt = buf->size;
    len = MIN((BUFFER_LEN - 1 - cnt), len);
    ptr = buf->data + cnt;
 
    while (*str && len) {
        *ptr++ = *str++;
        len--;
    }
 
    *ptr = '\0';
    buf->size = ptr - buf->data;
}


void Bufncpy(Buffer *buf, const char *str, unsigned len)
{
    buf->size = 0;
    *(buf->data) = '\0';
    Bufncat(buf, str, len);
}


void Bufsprint(Buffer *buf, const char *fmt, ...)
{
    va_list ap;
    
    va_start(ap, fmt);
    Bufvsprint(buf, fmt, ap);
    va_end(ap);
}

void Bufvsprint(Buffer *buf, const char *fmt, va_list ap)
{
    register const char *ptr;
    dbref temp;
    const char *timefmt;
    
    Bufcpy(buf, "");
    for (ptr = fmt; *ptr; ptr++) {
        if (*ptr != '%') {
            Bufcat_char(buf, *ptr);
            continue;
        }
        switch (*++ptr) {
        case 's':
            Bufcat(buf, va_arg(ap, const char *));
            break;
	case 'S':
	    Bufcat(buf, Buftext(va_arg(ap, Buffer *)));
	    break;
        case 'i':
            Bufcat_int(buf, va_arg(ap, int));
            break;
        case 'd':
            Bufcat_dbref(buf, va_arg(ap, dbref));
            break;
        case 'n':
            Bufcat_name(buf, va_arg(ap, dbref));
            break;
        case 'u':
            temp = va_arg(ap, dbref);
            Bufcat_unparse(buf, temp, va_arg(ap, dbref));
            break;
        case 'p':
            temp = va_arg(ap, dbref);
            Bufcat(buf, pronoun_substitute(temp, va_arg(ap, char *)));
            break;
        case 'b':
            /* Get a normal unparse of the boolexp (0). */
            temp = va_arg(ap, dbref);
	    Bufcat_boolexp(buf, temp, va_arg(ap, struct boolexp *), 0);
            break;
	case 't':
	    /* Use the local time of the mud. */
	    timefmt = va_arg(ap, const char *);
	    Bufcat_mucktime(buf, timefmt, va_arg(ap, long), 1);
	    break;
	case 'T':
	    /* Use gmtime, which is also good for deltas (WHO list). */
	    timefmt = va_arg(ap, const char *);
	    Bufcat_mucktime(buf, timefmt, va_arg(ap, long), 0);
	    break;
        case '%':
            Bufcat_char(buf, '%');
            break;
        default:
            Bufcat_char(buf, '%');
            Bufcat_char(buf, *ptr);
            break;
        }
    }
}


void Bufsetchar(Buffer *buf, unsigned where, const char what)
{
    if (buf->size < where)
        return;
    
    buf->data[where] = what;
}


void Bufstw(Buffer *buf)
{
    register char *ptr;
    register int len;
    
    len = buf->size;
    ptr = buf->data + len - 1;

    /* isspace(*ptr) == 0 when *ptr == '\0' */
    while (len && isspace(*ptr)) {
        *ptr-- = '\0';
        len--;
    }

    buf->size = len;
}


void Bufaddnl(Buffer *buf)
{
    register char *ptr;
    register int len;
    
    len = buf->size;
    if (len == BUFFER_LEN - 1) len--;
    ptr = buf->data + len;
    *ptr++ = '\n';
    *ptr = '\0';
    buf->size = len + 1;
}


/* Bufgets returns a char *, rather than a Buffer *, in order to make loops
   to read in files easier to deal with.  You already have the buffer in
   variable if you need it, alos. */
char *Bufgets(Buffer *buf, void *vfp, int *eof)
{
    register char *result;
    register unsigned len;
    register char *ptr;
    FILE *fp = vfp;
    int chr;
    
    ptr = buf->data;

    result = fgets(ptr, BUFFER_LEN - 1, fp);
    if (result) {
        /* Strip the trailing newline. */
        len = strlen(buf->data);
        ptr += len - 1;
        if (*ptr == '\n') {
            *ptr = '\0';
            len--;
        } else {
            /* Eat characters to the newline. */
	    chr = '\0';
	    while (!feof(fp) && !ferror(fp) && (chr != '\n'))
        	chr = getc(fp);
        }
        buf->size = len;
    } else {
        buf->size = 0;
        *ptr = '\0';		/* Make sure it's empty. */
    }

    if (eof)
	*eof = (feof(fp) || ferror(fp));

    return result;
}


void Bufputs(Buffer *buf, void *vfp, int *eof)
{
    FILE *fp = vfp;

    if (buf->size) fputs(buf->data, fp);
    putc('\n', fp);

    if (eof)
	*eof = (feof(fp) || ferror(fp));
}


void Bufcat_dbref(Buffer *buf, const dbref what)
{
    Bufcat_int(buf, (long) what);
}


void Bufcat_name(Buffer *buf, const dbref what)
{
    if (what == NOTHING) Bufcat(buf, "*NOTHING*");
    else if (what == HOME) Bufcat(buf, "*HOME*");
    else if ((what < 0) || (what >= db_top)) Bufcat(buf, "*BAD OBJ*");
    else if (Typeof(what) == TYPE_GARBAGE) Bufcat(buf, "*GARBAGE*");
    else Bufcat(buf, GetName(what));
}


void Bufcat_unparse(Buffer *buf, const dbref player, const dbref what)
{
    Bufcat_name(buf, what);

    if (Garbage(what)) return;
    if (player == NOTHING) return;

    if (can_see_flags(player, what)) {
        /* name(#43265flags) */
        Bufcat(buf, "(#");
        Bufcat_dbref(buf, what);
        Bufcat(buf, unparse_flags(what));
        Bufcat(buf, ")");
    }
}


void Bufdel_char(Buffer *buf)
{
    char *ptr;

    if (buf->size > 0) {
	ptr = buf->data + buf->size - 1;
	*ptr = '\0';
	buf->size--;
    }
}
