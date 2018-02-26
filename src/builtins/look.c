/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* look.c,v 2.5 1994/02/20 23:03:53 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "externs.h"


static void look_contents(const dbref player, const dbref loc, const char *contents_name)
{
    dbref thing;
    int can_see_loc;
    int gave_message = 0;
    
    /* check to see if he can see the location */
    /* Change the 2 occurances of 'Wizard(player)' to '0' to make
       dark work for wizards. */
    can_see_loc = (!Dark(loc) || controls(player, loc, Wizard(player)));
    
    /* check to see if there is anything there */
    DOLIST(thing, GetContents(loc)) {
#ifdef DRUID_MUCK
	/* Don't show disconnected players in room contents. */
	if (Typeof(thing) != TYPE_PLAYER || check_awake(thing))
#endif
	if (can_see(player, thing, Wizard(player), can_see_loc)) {
	    if (!gave_message) {
		notify(player, contents_name);
		gave_message = 1;
	    }
	    notify(player, "%u", player, thing);
	}
    }
}


static void look_simple(const dbref player, const dbref thing)
{
    if (GetDesc(thing)) {
	exec_or_notify(player, thing, GetDesc(thing), 0);
    } else {
	notify(player, "You see nothing special.");
    }
}


static void look_room(const dbref player, const dbref loc)
{
    const char *tmp;

    /* tell him the name, and the number if he can link to it */
    notify(player, "%u", player, loc);
    
    /* tell him the description */
    tmp = GetDesc(loc);
    if (tmp)
        exec_or_notify(player, loc, tmp, 0);
    
    /* tell him the appropriate messages if he has the key */
    can_doit(player, loc, 0);
    
    /* tell him the contents if the room isn't MURKY */
    if(!HasFlag(loc, MURKY))
        look_contents(player, loc, "Contents:");
}


void do_look(const dbref player, const char *name, const char *ignore1, const char *ignore2)
{
    dbref thing;
    struct match_data md;
    
    if (!*name) {
	thing = GetLoc(player);

	if (thing != NOTHING) {
	    look_room(player, thing);
	}
    } else {
	/* look at a thing here */
	init_match(player, name, NOTYPE, &md);
	match_all_exits(&md);
	match_neighbor(&md);
	match_possession(&md);
	if (Wizard(player)) {
	    match_absolute(&md);
	    match_player(&md);
	}
	match_here(&md);
	match_me(&md);
	thing = noisy_match_result(&md);

	if (thing != NOTHING) {
	    switch (Typeof(thing)) {
	    case TYPE_ROOM:
		if (GetLoc(player) != thing
		    && !can_link_to(player, TYPE_ROOM, thing)) {
		    notify(player, "Permission denied.");
		} else {
		    look_room(player, thing);
		}
		break;
	    case TYPE_PLAYER:
		look_simple(player, thing);
		look_contents(player, thing, "Carrying:");
		break;
	    default:
		look_simple(player, thing);
		break;
	    }
	}
    }
}
