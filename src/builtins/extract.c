/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* extract.c,v 2.4 1993/12/09 08:35:59 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "externs.h"

/* Based on code written by Ben Jackson. */

/* You'll probably want to crank up MAX_OUTPUT in params.h if you've
   got players w/ lots of large objects.  I'd recommend against leaving
   this in (and the associated dangerously large MAX_OUTPUT), except when
   you are doing extractions. */


void do_extract(const dbref player, const char *name, const char *ignore1, const char *ignore2)
{
    dbref owner;
    dbref thing;
    
    if (name && *name) {
        if (!Wizard(player)) {
	    notify(player, "Only wizards can extract other peoples' objects.");
	    return;
	}

        owner = lookup_player(name);
	if (owner == NOTHING) {
            notify(player, "That player does not exist.");
            return;
	}
    } else {
        owner = player;
    }
    
    for (thing = owner; thing != NOTHING; thing = GetOwnList(thing)) {
	show_examine(player, thing, 1); /* 1 means list muf code. */
	notify(player, "Done.");
	notify(player, "");
    }
}
