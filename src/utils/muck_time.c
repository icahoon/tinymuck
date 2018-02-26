/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* muck_time.c,v 2.3 1993/04/08 20:24:40 dmoore Exp */
#include "config.h"

#include "muck_time.h"
#include "externs.h"

/* The MUF interpreter wants to treat all times as unix time, makes life
   easier for the player, since it's a nicely encoded integer.  So on
   non-unix systems some conversion may be needed at times. */

#ifndef UNIX_TIME

time_t jan119070;
static struct tm unix_tm;

void init_mucktime(void)
{
    struct tm temp;

    /* Setup for Jan 1, 1970. */
    unix_tm.tm_sec = 0;
    unix_tm.tm_min = 0;
    unix_tm.tm_hour = 0;
    unix_tm.tm_mday = 1;
    unix_tm.tm_mon = 0;
    unix_tm.tm_year = 70;
    unix_tm.tm_wday = 4;
    unix_tm.tm_yday = 0;
    unix_tm.tm_isdst = 0;

    temp = unix_tm;
    jan119070 = mktime(&temp);
}


struct tm *localmucktime(long *tp)
{
    struct tm temp;
    long t = *tp;

    temp = unix_tm;

    temp.tm_sec += t;

    t = mktime(&temp);

    return localtime(&t);
}

#endif /* UNIX_TIME */
