/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* force.c,v 2.5 1997/06/28 00:53:30 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"


void do_force(const dbref player, const char *name, const char *command, const char *ignore)
{
    dbref victim;
    
    if (!Wizard(player)) {
	notify(player, "Only Wizards may use this command.");
	return;
    }
    
    victim = lookup_player(name);
    if (victim == NOTHING) {
	notify(player, "That player does not exist.");
	return;
    }
    
    if (TrueGod(victim) && !God(player)) {
	notify(player, "You cannot force god to do anything.");
	return;
    }
    
    log_status("FORCED: %u by %u: %s", victim, victim,
	       player, player, command);
    notify(victim, "You are forced by %n to do their bidding: %s",
	   player, command);
    /* force victim to do command */
    process_command(victim, command, strlen(command));
}

