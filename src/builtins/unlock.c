/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* unlock.c,v 2.3 1993/04/08 20:24:37 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"


void do_unlock(const dbref player, const char *name, const char *ignore1, const char *ignore2)
{
    dbref thing;
    
    thing = match_controlled(player, name);
    if (thing != NOTHING) {
	FreeKey(thing);
	notify(player, "Unlocked.");
    }
}
