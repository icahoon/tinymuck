/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* inv.c,v 2.3 1993/04/08 20:24:35 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"

void do_inventory(const dbref player, const char *ignore1, const char *ignore2, const char *ignore3)
{
    dbref thing = GetContents(player);
  

    if (thing == NOTHING) {
	notify(player, "You aren't carrying anything.");
    } else {
	notify(player, "You are carrying:");
	DOLIST(thing, thing) {
	    notify(player, "%u", player, thing);
	}
    }
  
    do_score(player, "", "", "");
}

