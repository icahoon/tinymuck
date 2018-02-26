/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* connect.c,v 2.4 1993/12/16 07:26:41 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"

void do_connect(const dbref player)
{
    dbref loc = GetLoc(player);
    
    if (loc == NOTHING) return;
    if (!Dark(player) && !Dark(loc)) {
	notify_except(loc, player, "%n has connected.", player);
    }

    do_look(player, "", "", "");

#ifndef MUF_CONNECT_HOOKS
    if (HasFlag(player, IN_PROGRAM)) {
	notify(player, "***  You are currently using a program.  Use \"@Q\" to return to a more reasonable state of control.  ***");
    }
    if (HasFlag(player, IN_EDITOR)) {
	if (HasFlag(player, INSERT_MODE)) {
	    notify(player, "***  You are currently inserting MUF program text.  Use \".\" to return to the editor, then \"quit\" if you wish to return to your regularly scheduled Muck universe.  ***");
	} else {
	    notify(player, "***  You are currently using the MUF program editor.  ***");
	}
    }
#else /* MUF_CONNECT_HOOKS */
    /* Only do this on the first connection of the player. */
    if (check_awake(player) == 1) {
	const char *to_run;

	/* Minor sanity checking, shouldn't ever be needed. */
	interp_quit_external(player);
	edit_quit_external(player);

	/* Run _connect program located on #0. */
	to_run = get_string_prop(global_environment, "_connect", NORMAL_PROP);
	if (to_run) exec_or_notify(player, global_environment, to_run, 1);

	/* Run _connect program located on player. */
	to_run = get_string_prop(player, "_connect", NORMAL_PROP);
	if (to_run) exec_or_notify(player, player, to_run, 1);
    }
#endif /* MUF_CONNECT_HOOKS */
}


