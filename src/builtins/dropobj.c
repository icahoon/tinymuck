/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* dropobj.c,v 2.4 1993/12/16 07:26:23 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "externs.h"


void do_drop(const dbref player, const char *name, const char *ignore1, const char *ignore2)
{
    dbref loc;
    dbref thing;
    struct match_data md;
    
    loc = GetLoc(player);

    if (loc == NOTHING) return;
    
    init_match(player, name, NOTYPE, &md);
    match_possession(&md);
    thing = noisy_match_result(&md);

    if ((thing == NOTHING) || (thing == AMBIGUOUS)) return;
    
    switch (Typeof(thing)) {
    case TYPE_THING:
    case TYPE_PROGRAM:
	if (GetLoc(thing) != player) {
	    /* Shouldn't ever happen because of match_possession. */
	    notify(player, "You can't drop that.");
	    break;
	}

	if (HasFlag(thing, STICKY) && Typeof(thing) == TYPE_THING) {
	    send_home(thing);
	} else {
            if ((GetLink(loc) != NOTHING) && !HasFlag(loc, STICKY)) {
                /* Dropto now, not delayed. */
                move_object(thing, GetLink(loc), NOTHING);
	    } else {
		move_object(thing, loc, NOTHING);
	    }
	}

	if (GetDrop(thing))
	    exec_or_notify(player, thing, GetDrop(thing), 0);
	else
	    notify(player, "Dropped.");
	
	if (GetDrop(loc))
	    exec_or_notify(player, loc, GetDrop(loc), 0);
	
	if (GetODrop(thing)) {
	    notify_except(loc, player, "%n %p",
			  player, player, GetODrop(thing));
	} else {
	    notify_except(loc, player, "%n drops %n.", player, thing);
	}
	
	if (GetODrop(loc)) {
	    notify_except(loc, player, "%n %p", thing, player, GetODrop(loc));
	}
	break;
    default:
	notify(player, "You can't drop that.");
	break;
    }
}
