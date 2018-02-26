/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* wall.c,v 2.3 1993/04/08 20:24:34 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"


void do_wall(const dbref player, const char *ignore1, const char *ignore2, const char *message)
{
    if (Wizard(player)) {
	log_status("WALL: %u: %s", player, player, message);
	notify_all("%n shouts, \"%s\"", player, message);
    } else {
	notify(player, "But what do you want to do with the wall?");
    }
}

