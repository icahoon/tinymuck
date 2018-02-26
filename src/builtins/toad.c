/* Copyright (c) 1992 by David Moore.  All rights reserved. */
#include "copyright.h"
/* toad.c,v 2.10 1994/05/15 18:30:52 dmoore Exp */

#include "config.h"

#include "db.h"
#include "match.h"
#include "externs.h"
#include "buffer.h"

void do_toad(const dbref player, const char *name, const char *recip, const char *ignore)
{
    dbref victim;
    dbref recipient;
    dbref stuff;
    Buffer buf;
    
    if (!Wizard(player)) {
	notify(player, "Only a Wizard can turn a person into a toad.");
	return;
    }
    
    victim = lookup_player(name);
    if (victim == NOTHING) {
	notify(player, "That player does not exist.");
	return;
    }
    
    if ((recip == NULL) || (*recip == '\0')) {
	recipient = head_player;
    } else {
	recipient = lookup_player(recip);
	if (recipient == NOTHING) {
	    notify(player, "That recipient does not exist.");
	    return;
	}
    }
    
    if (Typeof(victim) != TYPE_PLAYER) {
	notify(player, "You can only turn players into toads!");
    } else if (TrueWizard(victim)) {
	notify(player, "You can't turn a Wizard into a toad.");
    } else if (recipient == victim) {
	notify(player, "Toads can't own themselves.");
    } else {
	notify(victim, "You have been turned into a toad.");
	notify(player, "You turned %n into a toad!", victim);
	log_status("TOADED: %u by %u.", victim, victim, player, player);

	edit_quit_external(victim);
	interp_quit_external(victim);

	/* chown things to recipient, checking for a sane home location */
	/* for object. XXX -- if HOME/inventory handling changes, */
	/* please check this code.*/
	chown_macros(victim, recipient);
	
	for (stuff = 0; stuff < db_top; stuff++) {
	    if (Garbage(stuff)) continue;

	    if (((Typeof(stuff) == TYPE_THING)
		 || (Typeof(stuff) == TYPE_PROGRAM))
		&& (GetLink(stuff) == victim)) {
		SetLink(stuff, player_start);
	    }
	    if (GetOwner(stuff) == victim) {
		chown_object(stuff, recipient);
	    }
	}

	/* Take them home; things should no longer be homed here, programs */
	/* should not be owned by player */
	send_contents(victim, HOME);

	boot_off(victim, -1);
	delete_player(victim);

	/* Semi strange way to turn a player into an object. */
	SetFlags(victim, TYPE_THING);
	object_counts[TYPE_PLAYER]--;
	object_counts[TYPE_THING]++;

	chown_object(victim, player);
	SetPennies(victim, 1);
	FreePass(victim);
	
	/* reset name */
	Bufsprint(&buf, "a slimy toad named %n", victim);
	SetName(victim, Buftext(&buf));
    }
}


