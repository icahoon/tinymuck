/* ansify.h,v 2.12 1997/07/03 00:16:29 dmoore Exp */
#ifndef MUCK_ANSIFY_H
#define MUCK_ANSIFY_H

/* If your <time.h> include file does not include difftime, like it
   should, you will probably find that the below define works on most
   computers.  You might want to make whoever is in charge of such things
   at your site add this define to the proper include files. */

#ifndef HAVE_DIFFTIME
#define difftime(a, b) ((double)(a) - (double)(b))
#endif


/* Various machines (like sysvr4 don't have this, except in ucbcc) don't
   define this.  Posix 1003.1 doesn't provide a way of figuring it out.
   We use the value we hope to be.  Solaris uses 44. */
#ifndef NSIG
#define NSIG 44
#endif


/* If tolower on your machine is broken (ie messes up when called on
   something that's already lower case), then uncomment this. */
#ifdef BROKEN_TOLOWER
#undef tolower
#define tolower(x) (isupper(x) ? (x) - 'A' + 'a' : (x))
#endif

/* If this is posix get the max fd's that way, other wise try getdtablesize. */
/* If you don't have either sysconf or getdtablesize, try 64. */
#if defined(HAVE_UNISTD_H) && defined(_SC_OPEN_MAX)
#define muck_max_fds() sysconf(_SC_OPEN_MAX)
#else
#define muck_max_fds() getdtablesize()
#endif



/* If your <string.h> include file does not define the various mem routines,
   like it should, you will probably find that the below include works on
   most unix machines.  You might want to make whoever is in charge of
   such things at your site fix this for everyone. */

/*
#include <memory.h>
*/


/*
#define remove(x)	unlink(x)
*/


/*
extern int errno;
*/


#ifndef HAVE_STRERROR
extern int sys_nerr;
extern char *sys_errlist[];
#define strerror(x)	(((x) < 0 || (x) >= sys_nerr) \
			 ? "Unknown errno" \
			 : sys_errlist[x])
#endif


/*
typedef int pid_t;
*/


/*
typedef long time_t;
*/


/*
typedef unsigned long size_t;
*/

#ifndef HAVE_RANDOM
/* rand generally only returns 15 bits.  we can get 31 of it it this way.
   also the low bits of rand are often bad, so we toss them.  random when
   available is a much better choice. */
#define muck_rnd_11	((rand() & 0x7FFF) >> 4)
#define muck_rnd_10	((rand() & 0x7FFF) >> 5)
#define random()	((muck_rnd_11 << 20) | (muck_rnd_10 << 10) | muck_rnd_10)
#define srandom(x)	srand(x)
#endif

/* Specifically deal with HPUX annoyances. */
#ifdef _INCLUDE_HPUX_SOURCE
#define getdtablesize()	NOFILE
#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* MUCK_ANSIFY_H */
