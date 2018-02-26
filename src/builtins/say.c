/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* say.c,v 2.3 1993/04/08 20:24:34 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"


void do_say(const dbref player, const char *ignore1, const char *ignore2, const char *message)
{
    dbref loc = GetLoc(player);
    
    if (loc == NOTHING) return;
    
    notify(player, "You say, \"%s\"", message);
    notify_except(loc, player, "%n says, \"%s\"", player, message);
}

