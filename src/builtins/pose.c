/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* pose.c,v 2.3 1993/04/08 20:24:35 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"

void do_pose(const dbref player, const char *ignore1, const char *ignore2, const char *message)
{
    dbref loc = GetLoc(player);

    if (loc == NOTHING) return;
    
    notify_except(loc, NOTHING, "%n %s", player, message);
}
