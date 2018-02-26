/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* shutdown.c,v 2.3 1993/04/08 20:24:36 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"

extern int shutdown_flag;

void do_shutdown(const dbref player, const char *ignore1, const char *ignore2, const char *ignore3)
{
    if (Wizard(player)) {
	log_status("SHUTDOWN: by %u", player, player);
	shutdown_flag = 1;
    } else {
	notify(player, "Your delusions of grandeur have been duly noted.");
	log_status("ILLEGAL SHUTDOWN: tried by %u", player, player);
    }
}
