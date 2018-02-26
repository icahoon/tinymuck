/* Copyright (c) 1992 by Ken Arromdee and David Moore.  All rights reserved. */
/* entrances.c,v 2.5 1994/02/17 23:49:53 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "params.h"		/* LOOKUP_COST */
#include "externs.h"

static void show_stuff_player(const dbref, const dbref);
static void show_stuff_other(const dbref, const dbref);
static void show_item(const dbref, const dbref, const dbref, const int);

void do_entrances(const dbref player, const char *arg, const char *ignore1, const char *ignore2)
{
    dbref thing;
    struct match_data md;
    
    if (!*arg) {
	thing = GetLoc(player);
    } else {
	init_match(player, arg, NOTYPE, &md);
	match_neighbor(&md);
	match_possession(&md);
	match_me(&md);
	match_here(&md);
	match_absolute(&md);
	match_player(&md);
	
	if ((thing = noisy_match_result(&md)) == NOTHING)
	    return;
    }

    if (thing == HOME) thing = GetLink(player);

    if (Garbage(thing)) {
	notify(player, "<garbage> has nothing linked to it.");
	return;
    }

    if (!controls(player, thing, Wizard(player))) {
	notify(player, "Permission denied.");
	return;
    }

    /* Same cost as @find, for the time being, until we get backlinks */
    if (!payfor(player, LOOKUP_COST)) {
	notify(player, "You don't have enough pennies.");
	return;
    }

    show_stuff_player(player, thing);
    show_stuff_other(player, thing);
}


static void show_stuff_player(const dbref player, const dbref thing)
{
    dbref thing_owner, i;

    thing_owner = GetOwner(thing);
    if (thing_owner == player) {
	notify(player, "Linked things owned by you:");
    } else {
	notify(player, "Linked things owned by %u:", player, thing_owner);
    }

    for (i = thing_owner; i != NOTHING; i = GetOwnList(i)) {
	if (Typeof(i) == TYPE_PROGRAM) continue;

	show_item(player, thing, i, 1);
    }
	
    notify(player, "***End of List***");
}


static void show_stuff_other(const dbref player, const dbref thing)
{
    dbref thing_owner, i;

    thing_owner = GetOwner(thing);
    if (thing_owner == player) {
	notify(player, "Linked things owned by others:");
    } else {
	notify(player, "Linked things not owned by %u:", player, thing_owner);
    }

    for (i = 0; i < db_top; i++) {
	if (Garbage(i)) continue;
	if (Typeof(i) == TYPE_PROGRAM) continue;
	if (GetOwner(i) == thing_owner) continue;

	show_item(player, thing, i, 0);
    }
	
    notify(player, "***End of List***");
}


static void show_item(const dbref player, const dbref thing, const dbref i, const int self)
{
    dbref *exit_list;
    int cnt, found;

    found = 0;
    exit_list = GetDest(i);
    for (cnt = GetNDest(i); cnt; cnt--) {
	if (*exit_list++ == thing) found++;
    }

    if (found == 1) {
	if (self || (Typeof(i) == TYPE_PLAYER)) {
	    notify(player, "%u", GetOwner(i), i);
	} else {
	    notify(player, "%u  Owner: %n", GetOwner(i), i, GetOwner(i));
	}
    } else if (found > 1) {
	if (self || (Typeof(i) == TYPE_PLAYER)) {
	    notify(player, "%u [multiplicity %i]", GetOwner(i), i, found);
	} else {
	    notify(player, "%u [multiplicity %i]  Owner: %n", GetOwner(i),
		   i, found, GetOwner(i));
	}
    }
}
