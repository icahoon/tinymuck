/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* get.c,v 2.3 1993/04/08 20:24:36 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "externs.h"


void do_get(const dbref player, const char *what, const char *ignore1, const char *ignore2)
{
    dbref thing;
    struct match_data md;
    
    init_match_check_keys(player, what, TYPE_THING, &md);
    match_neighbor(&md);
    if (Wizard(player))
	match_absolute(&md); /* the wizard has long fingers */

    thing = noisy_match_result(&md);
    if (thing == NOTHING) return;

    if (GetLoc(thing) == player) {
        notify(player, "You already have that!");
        return;
    }

    switch (Typeof(thing)) {
    case TYPE_THING:
    case TYPE_PROGRAM:
        if (can_doit(player, thing, "You can't pick that up.")) {
            move_object(thing, player, NOTHING);
            notify(player, "Taken.");
        }
        break;
    default:
        notify(player, "You can't take that!");
        break;
    }
}

