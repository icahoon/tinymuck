/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* dig.c,v 2.4 1993/04/10 05:09:11 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "params.h"
#include "externs.h"


void do_dig(const dbref player, const char *name, const char *pname, const char *ignore)
{
    dbref room;
    dbref parent;
    struct match_data md;
  
    if (!Builder(player)) {
	notify(player, "That command is restricted to authorized builders.");
	return;
    }
  
    if (!*name) {
	notify(player, "You must specify a name for the room.");
	return;
    } else if (!ok_name(name)) {
	notify(player, "That's a silly name for a room!");
	return;
    }

    if (!payfor(player, ROOM_COST)) {
	notify(player, "Sorry, you don't have enough pennies to dig a room.");
	return;
    }

    room = make_object(TYPE_ROOM, name, player, global_environment, NOTHING);
    PushContents(global_environment, room);
  
    /* Amazingly silly item dating back to 2.2. */
    SetOnFlag(room, GetFlags(player) & JUMP_OK);

    notify(player, "%s created with room number %d.", name, room);

    /* If no parent room to set, then just return. */
    if (!*pname) return;

    notify(player, "Trying to set parent...");

    init_match(player, pname, TYPE_ROOM, &md);
    match_absolute(&md);
    match_here(&md);
    parent = noisy_match_result(&md);

    if (parent == NOTHING) {
	notify(player, "Parent set to default.");
	return;
    }

    if (!can_link_to(player, Typeof(room), parent)
	|| (room == parent)) {
	notify(player, "Permission denied.  Parent set to default.");
	return;
    }

    move_object(room, parent, NOTHING);

    notify(player, "Parent set to %u.", player, parent);
}
