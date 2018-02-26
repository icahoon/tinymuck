/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* owned.c,v 2.4 1993/12/09 08:36:02 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"


void do_owned(const dbref player, const char *ignore1, const char *ignore2, const char *name)
{
    dbref victim, i;

    if (Wizard(player) && *name) {
        victim = lookup_player(name);
        if (victim == NOTHING) {
	    notify(player, "I couldn't find that player.");
	    return;
	}
    } else {
        victim = player;
    }

    for (i = victim; i != NOTHING; i = GetOwnList(i)) {
	notify(player, "%u", player, i);
    }

    notify(player, "***End of List***");
}

