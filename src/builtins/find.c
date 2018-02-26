/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* find.c,v 2.4 1993/12/09 08:36:01 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"


void do_find(const dbref player, const char *name, const char *ignore1, const char *ignore2)
{
    dbref i;
  
    if (Wizard(player)) {
	for (i = 0; i < db_top; i++) {
	    if (!Garbage(i)
		&& (Typeof(i) != TYPE_EXIT)
	        && (!*name || muck_strmatch(GetName(i), name))) {
		notify(player, "%u", player, i);
	    }
	}
    } else {
	for (i = player; i != NOTHING; i = GetOwnList(i)) {
	    if ((Typeof(i) != TYPE_EXIT)
	        && (!*name || muck_strmatch(GetName(i), name))) {
		notify(player, "%u", player, i);
	    }
	}
    }

    notify(player, "***End of List***");
}

