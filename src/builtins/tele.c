/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* tele.c,v 2.3 1993/04/08 20:24:34 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "externs.h"


void do_teleport(const dbref player, const char *what, const char *whereto, const char *ignore)
{
    dbref victim;
    dbref destination;
    struct match_data md;
    
    /* get victim, destination */
    if (!whereto || !*whereto) {
	/* Shorthand: @tel #21   ->   @tel me = #21 */
	victim = player;
	whereto = what;
    } else {
	init_match(player, what, NOTYPE, &md);
	match_neighbor(&md);
	match_possession(&md);
	match_me(&md);
	match_here(&md);
	match_absolute(&md);
	match_player(&md);

	victim = noisy_match_result(&md);
	if (victim == NOTHING) return;
    }
    
    /* get destination */
    init_match(player, whereto, TYPE_PLAYER, &md);
    match_here(&md);
    match_home(&md);
    match_absolute(&md);
    match_me(&md);
    if (Wizard(player)) {
	match_neighbor(&md);
	match_player(&md);
    }

    destination = match_result(&md);
    switch (destination = match_result(&md)) {
    case NOTHING:
	notify(player, "Send it where?");
	return;
	break;
    case AMBIGUOUS:
	notify(player, "I don't know which destination you mean!");
	return;
	break;
    case HOME:
	destination = GetHome(victim);
	break;
    default:
	break;
    }

    switch (Typeof(victim)) {
    case TYPE_EXIT:
	notify(player, "You can't teleport that.");
	return;
	break;
    case TYPE_GARBAGE:
	notify(player,
	       "That object is in a place where magic cannot reach it.");
	return;
	break;
    default:
	break;
    }

    if (!moveto_type_ok(victim, destination)) {
	notify(player, "Bad destination.");
	return;
    }

    if (!(moveto_source_ok(victim, destination, player, Wizard(player))
	  && moveto_thing_ok(victim, destination, player, Wizard(player))
	  && moveto_dest_ok(victim, destination, player, Wizard(player)))) {
	notify(player, "Permission denied.");
	return;
    }

    if (!moveto_loop_ok(victim, destination)) {
	notify(player, "Parent would create a loop.");
	return;
    }

    switch (Typeof(victim)) {
    case TYPE_PLAYER:
	if (player != victim) {
	    notify(victim, "You feel a wrenching sensation...");
	    notify(player, "Teleported.");
	}
	break;
    case TYPE_ROOM:
	notify(player, "Parent set.");
	break;
    default:
	/* Aieee, need to have some sort of check for droptos and crap. */
	notify(player, "Teleported.");
	break;
    }

    move_object(victim, destination, NOTHING);
}

