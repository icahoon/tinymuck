/* Modified on May 19, 1992 by dmoore.  Originally from the BSD NET-2
   release file strftime.c.  */
/* muck_strftime.c,v 2.10 1997/06/23 20:17:55 dmoore Exp */

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strftime.c	5.11 (Berkeley) 2/24/91";
#endif /* LIBC_SCCS and not lint */

#include "config.h"

#include <string.h>

#include "muck_time.h"
#include "externs.h"

#define TM_YEAR_BASE 1900


#ifdef NEED_TIME_ZONE
static const char *Zfmt[] = NEED_TIME_ZONE;
#endif

#ifdef HAVE_TZNAME
extern char *tzname[2];
static char **Zfmt;
#endif

static const char *afmt[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
};
static const char *Afmt[] = {
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
	"Saturday",
};
static const char *bfmt[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
	"Oct", "Nov", "Dec",
};
static const char *Bfmt[] = {
	"January", "February", "March", "April", "May", "June", "July",
	"August", "September", "October", "November", "December",
};

static size_t gsize;
static char *pt;


static size_t fmt(register const char *format, long tim, const struct tm *t);
static stuff_secs(register long s);
static conv(int n, int digits, char pad);
static add(register const char *str);

size_t muck_strftime(char *s, size_t maxsize, const char *format, long tim, const int use_local_time)
{
	const struct tm *t;
#ifdef HAVE_TZSET
	static int tzset_called = 0;
	if (!tzset_called) {
	    tzset();
	    tzset_called = 1;
	}
#endif
#ifdef HAVE_TZNAME
	Zfmt = tzname;
#endif

	if (use_local_time)
	    t = localmucktime(&tim);
	else
	    t = gmtime(&tim);

	pt = s;
	if ((gsize = maxsize) < 1)
		return(0);
	if (fmt(format, tim, t)) {
		*pt = '\0';
		return(maxsize - gsize);
	}
	return(0);
}


static size_t fmt(register const char *format, long tim, const struct tm *t)
{
	for (; *format; ++format) {
		if (*format == '%')
			switch(*++format) {
			case '\0':
				--format;
				break;
			case 'A':
				if (t->tm_wday < 0 || t->tm_wday > 6)
					return(0);
				if (!add(Afmt[t->tm_wday]))
					return(0);
				continue;
			case 'a':
				if (t->tm_wday < 0 || t->tm_wday > 6)
					return(0);
				if (!add(afmt[t->tm_wday]))
					return(0);
				continue;
			case 'B':
				if (t->tm_mon < 0 || t->tm_mon > 11)
					return(0);
				if (!add(Bfmt[t->tm_mon]))
					return(0);
				continue;
			case 'b':
			case 'h':
				if (t->tm_mon < 0 || t->tm_mon > 11)
					return(0);
				if (!add(bfmt[t->tm_mon]))
					return(0);
				continue;
			case 'C':
				if (!fmt("%a %b %e %H:%M:%S %Y", tim, t))
					return(0);
				continue;
			case 'c':
				if (!fmt("%m/%d/%y %H:%M:%S", tim, t))
					return(0);
				continue;
			case 'D':
				if (!fmt("%m/%d/%y", tim, t))
					return(0);
				continue;
			case 'd':
				if (!conv(t->tm_mday, 2, '0'))
					return(0);
				continue;
			case 'e':
				if (!conv(t->tm_mday, 2, ' '))
					return(0);
				continue;
			case 'H':
				if (!conv(t->tm_hour, 2, '0'))
					return(0);
				continue;
			case 'I':
				if (!conv(t->tm_hour % 12 ?
				    t->tm_hour % 12 : 12, 2, '0'))
					return(0);
				continue;
			case 'j':
				if (!conv(t->tm_yday + 1, 3, '0'))
					return(0);
				continue;
			case 'k':
				if (!conv(t->tm_hour, 2, ' '))
					return(0);
				continue;
			case 'l':
				if (!conv(t->tm_hour % 12 ?
				    t->tm_hour % 12 : 12, 2, ' '))
					return(0);
				continue;
			case 'M':
				if (!conv(t->tm_min, 2, '0'))
					return(0);
				continue;
			case 'm':
				if (!conv(t->tm_mon + 1, 2, '0'))
					return(0);
				continue;
			case 'p':
				if (!add(t->tm_hour >= 12 ? "PM" : "AM"))
					return(0);
				continue;
			case 'R':
				if (!fmt("%H:%M", tim, t))
					return(0);
				continue;
			case 'r':
				if (!fmt("%I:%M:%S %p", tim, t))
					return(0);
				continue;
			case 'S':
				if (!conv(t->tm_sec, 2, '0'))
					return(0);
				continue;
			case 's':
				if (!stuff_secs(tim))
					return(0);
				continue;
			case 'T':
			case 'X':
				if (!fmt("%H:%M:%S", tim, t))
					return(0);
				continue;
			case 'U':
				if (!conv((t->tm_yday + 7 - t->tm_wday) / 7,
				    2, '0'))
					return(0);
				continue;
			case 'W':
				if (!conv((t->tm_yday + 7 -
				    (t->tm_wday ? (t->tm_wday - 1) : 6))
				    / 7, 2, '0'))
					return(0);
				continue;
			case 'w':
				if (!conv(t->tm_wday, 1, '0'))
					return(0);
				continue;
			case 'x':
				if (!fmt("%m/%d/%y", tim, t))
					return(0);
				continue;
			case 'y':
				if (!conv((t->tm_year + TM_YEAR_BASE)
				    % 100, 2, '0'))
					return(0);
				continue;
			case 'Y':
				if (!conv(t->tm_year + TM_YEAR_BASE, 4, '0'))
					return(0);
				continue;
			case 'Z':
#if defined(NEED_TIME_ZONE) || defined(HAVE_TZNAME)
				/* guesses std if not known. */
				if (!add(Zfmt[!!t->tm_isdst]))
				    return(0);
#else
				if (!t->tm_zone || !add(t->tm_zone))
					return(0);
#endif
				continue;
			case 'J':
				/* WHO list connect time */
				/* like j, but uses leading spaces, not 0. */
				if (!conv(t->tm_yday, 3, ' '))
					return(0);
				continue;
			case 'i':
				/* WHO list idle time */
				if (t->tm_year > 70) {
					if (!conv(t->tm_year - 70, 3, ' '))
						return(0);
					if (!add("y"))
						return(0);
				} else if (t->tm_yday > 0) {
					if (!conv(t->tm_yday, 3, ' '))
						return(0);
					if (!add("d"))
						return(0);
				} else if (t->tm_hour > 0) {
					if (!conv(t->tm_hour, 3, ' '))
						return(0);
					if (!add("h"))
						return(0);
				} else  if (t->tm_min > 0) {
					if (!conv(t->tm_min, 3, ' '))
						return(0);
					if (!add("m"))
						return(0);
				} else if (t->tm_sec > 0) {
					if (!conv(t->tm_sec, 3, ' '))
						return(0);
					if (!add("s"))
						return(0);
				} else {
					if (!add("0s"))
						return(0);
				}
				continue;
			case '%':
			/*
			 * X311J/88-090 (4.12.3.5): if conversion char is
			 * undefined, behavior is undefined.  Print out the
			 * character itself as printf(3) does.
			 */
			default:
				break;
		}
		if (!gsize--)
			return(0);
		*pt++ = *format;
	}
	return(gsize);
}

static int stuff_secs(register long s)
{
	static char buf[15];
	register char *p;

	for (p = buf + sizeof(buf) - 2; s > 0 && p > buf; s /= 10)
		*p-- = s % 10 + '0';
	return(add(++p));
}

static int conv(int n, int digits, char pad)
{
	static char buf[10];
	register char *p;

	for (p = buf + sizeof(buf) - 2; n > 0 && p > buf; n /= 10, --digits)
		*p-- = n % 10 + '0';
	while (p > buf && digits-- > 0)
		*p-- = pad;
	return(add(++p));
}

static int add(register const char *str)
{
	for (;; ++pt, --gsize) {
		if (!gsize)
			return(0);
		if (!(*pt = *str++))
			return(1);
	}
}

