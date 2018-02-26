/* Copyright (c) 1992 by David Moore.  All rights reserved. */
#include "copyright.h"
/* pcreate.c,v 2.3 1993/04/08 20:24:36 dmoore Exp */

#include "config.h"

#include "db.h"
#include "externs.h"

void do_pcreate(const dbref player, const char *user, const char *password, const char *ignore)
{
    dbref newguy;
    
#ifdef GOD_PCREATE
    if (!God(player)) {
	notify(player, "Permission denied.");
	return;
    }
#else
    if (!Wizard(player)) {
	notify(player, "Permission denied.");
	return;
    }
#endif /* GOD_PCREATE */
    
    newguy = create_player(user, password);
    if (newguy == NOTHING) {
	notify(player, "Create failed.");
    } else {
	log_status("PCREATED: %u by %u", newguy, newguy, player, player);
	notify(player, "Player %s created as object %d.", user, newguy);
    }
}


