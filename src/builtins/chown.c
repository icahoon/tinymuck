/* Copyright (c) 1992 by David Moore.  All rights reserved. */
#include "copyright.h"
/* chown.c,v 2.5 1997/08/10 03:12:19 dmoore Exp */

#include "config.h"

#include "db.h"
#include "match.h"
#include "externs.h"


void do_chown(const dbref player, const char *name, const char *newowner, const char *ignore)
{
    dbref thing;
    dbref owner;
    struct match_data md;
    
    if (!*name) {
	notify(player, "You must specify what you want to take ownership of.");
	return;
    }
    
    init_match(player, name, NOTYPE, &md);
    match_everything(&md, Wizard(player));
    if ((thing = noisy_match_result(&md)) == NOTHING) return;
    
    if (*newowner && muck_stricmp(newowner, "me")) {
	if ((owner = lookup_player(newowner)) == NOTHING) {
	    notify(player, "I couldn't find that player.");
	    return;
	}
    } else {
	owner = player;
    }
    
    if (!Wizard(player) && player != owner) {
	notify(player, "Only wizards can transfer ownership to others.");
	return;
    }
    
    if (!Wizard(player) && (!HasFlag(thing, CHOWN_OK)
			    || Typeof(thing) == TYPE_PROGRAM)) {
	notify(player, "You can't take possession of that.");
	return;
    }
    
    switch (Typeof(thing)) {
    case TYPE_ROOM:
	if (!Wizard(player) && GetLoc(player) != thing) {
	    notify(player, "You can only chown \"here\".");
	    return;
	}
	break;
    case TYPE_THING:
	if (!Wizard(player) && GetLoc(thing) != player) {
	    notify(player, "You aren't carrying that.");
	    return;
	}
	break;
    case TYPE_PLAYER:
	notify(player, "Players always own themselves.");
	return;
    case TYPE_EXIT:
    case TYPE_PROGRAM:
	break;
    case TYPE_GARBAGE:
	notify(player, "No one wants to own garbage.");
	return;
    }

    chown_object(thing, owner);
    if (owner == player)
	notify(player, "Owner changed to you.");
    else {
	notify(player, "Owner changed to %u.", player, owner);
    }
}
