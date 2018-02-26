/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* recycle.c,v 2.3 1993/04/08 20:24:37 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "externs.h"

void do_recycle(const dbref player, const char *name, const char *ignore1, const char *ignore2)
{
    dbref thing;
    struct match_data md;
    
    init_match(player, name, TYPE_THING, &md);
    match_all_exits(&md);
    match_neighbor(&md);
    match_possession(&md);
    match_here(&md);
    match_absolute(&md);
    thing = noisy_match_result(&md);
    
    if (thing == NOTHING) return;

    if (thing < 0 || thing >= db_top) return;

    if (Typeof(thing) == TYPE_GARBAGE) {
        notify(player, "That's already garbage!");
	return;
    }

    if (!controls(player, thing, 0)) {
        notify(player, "Permission denied.");
	return;
    }

    switch (Typeof(thing)) {
    case TYPE_ROOM:
        if (thing == player_start || thing == global_environment) {
            notify(player, "This room may not be recycled.");
            return;
        }
        break;
    case TYPE_PLAYER:
        notify(player, "You can't recycle a player!");
	return;
	break;
    }
    recycle(player, thing);
}

