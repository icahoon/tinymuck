/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* boot.c,v 2.5 1994/05/15 18:30:50 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "externs.h"


void do_boot(const dbref player, const char *name, const char *ignore1, const char *ignore2)
{
    dbref victim;
    
    if (!Wizard(player)) {
	notify(player, "Only a Wizard can boot someone off.");
	return;
    }
    
    victim = lookup_player(name);
    if (victim == NOTHING) {
	notify(player, "That player does not exist.");
	return;
    } else if (TrueGod(victim) && !God(player)) {
	notify(player, "You can't boot God!");
    } else {
	notify(victim, "You have been booted off the game.");
	if (boot_off(victim, 0)) {
	    log_status("BOOTED: %u by %u.", victim, victim, player, player);
	    notify(player, "You booted %n off!", victim);
	} else {
	    notify(player, "%n is not connected.", victim);
	}
    }
}


