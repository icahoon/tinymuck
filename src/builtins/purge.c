/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* purge.c,v 2.4 1993/12/10 04:35:33 dmoore Exp */
#include "config.h"
 
#include "db.h"
#include "match.h"
#include "externs.h"
 
 
void do_purge(const dbref player, const char *name, const char *ignore1, const char *ignore2)
{
    dbref victim;
 
    if (!Wizard(player)) {
        notify(player, "Permission denied.");
        return;
    }
 
    victim = lookup_player(name);
    if (victim == NOTHING) {
        notify(player, "That player does not exist.");
        return;
    } else if (TrueWizard(victim) && !God(player)) {
        notify(player, "You can't purge a wizard!");
    } else {
        log_status("PURGED: %u by %u.", victim, victim, player, player);
        purge(player, victim);
    }
}
 
