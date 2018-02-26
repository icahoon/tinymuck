/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* stats.c,v 2.7 1993/12/09 08:36:05 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"

void do_stats(const dbref player, const char *name, const char *ignore1, const char *ignore2)
{
    int rooms;
    int exits;
    int things;
    int players;
    int programs;
    int garbage;
    int total;
    dbref i;
    dbref owner;
    
/*
    if (!Wizard(player)) {
        notify(player, "The universe contains %d objects.", db_top);
        return;
    }
*/

    if (!name || !*name) {
	rooms = object_counts[TYPE_ROOM];
	exits = object_counts[TYPE_EXIT];
	things = object_counts[TYPE_THING];
	players = object_counts[TYPE_PLAYER];
	programs = object_counts[TYPE_PROGRAM];
	garbage = object_counts[TYPE_GARBAGE];
    } else {
	if (!muck_stricmp(name, "me")) {
	    owner = player;
	} else {
            owner = lookup_player(name);
	}

        if (owner == NOTHING) {
            notify(player, "I can't find that player.");
            return;
        }

	if (!controls(player, owner, Wizard(player))) {
	    notify(player, "Permission denied.");
	    return;
	}

	rooms = exits = things = players = programs = garbage = 0;

	for (i = owner; i != NOTHING; i = GetOwnList(i)) {
	    switch (Typeof(i)) {
            case TYPE_ROOM: rooms++; break;
            case TYPE_EXIT: exits++; break;
            case TYPE_THING: things++; break;
            case TYPE_PLAYER: players++; break;
            case TYPE_PROGRAM: programs++; break;
	    }
	}
    } 

    total = rooms + exits + things + programs + players + garbage;

    notify(player,
           "%d objects = %d rooms, %d exits, %d things, %d programs, %d players, %d garbage.",
           total, rooms, exits, things, programs, players, garbage);
}

