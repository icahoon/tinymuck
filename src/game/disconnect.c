/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* disconnect.c,v 2.5 1993/12/16 07:26:43 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"


void do_disconnect(const dbref player)
{
    dbref loc = GetLoc(player);

    if (!Dark(player) && !Dark(loc)) {
	notify_except(loc, player, "%n has disconnected.", player);
    }

#ifdef MUF_CONNECT_HOOKS
    /* Only do this on the first connection of the player. */
    if (check_awake(player) == 1) {
	const char *to_run;

	/* Kick them out on disconnect. */
	interp_quit_external(player);
	edit_quit_external(player);

	/* Run _disconnect program located on player. */
	to_run = get_string_prop(player, "_disconnect", NORMAL_PROP);
	if (to_run) exec_or_notify(player, player, to_run, 1);

	/* Run _disconnect program located on #0. */
	to_run = get_string_prop(global_environment, "_disconnect", NORMAL_PROP);
	if (to_run) exec_or_notify(player, global_environment, to_run, 1);
    }
#endif /* MUF_CONNECT_HOOKS */
}

