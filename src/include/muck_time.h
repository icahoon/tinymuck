/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* muck_time.h,v 2.3 1993/04/08 20:24:48 dmoore Exp */
#ifndef MUCK_MUCK_TIME_H
#define MUCK_MUCK_TIME_H

#include <time.h>

/* We make all time look like the unix notion, namely counting seconds
   since Jan 1, 1970.  On non unix (posix?) systems, an additional
   conversion may sometimes be needed. */

#define muck_time()		systime2mucktime(time(NULL))

#ifdef UNIX_TIME

#define systime2mucktime(x)	((long) (x))
#define localmucktime(x)	localtime((time_t *) (x))
#define init_mucktime()

#else

extern time_t jan119070;
#define systime2mucktime(x)	((long) difftime(x, jan119070))

#endif /* UNIX_TIME */

#endif /* MUCK_MUCK_TIME_H */
