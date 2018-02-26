/* Copyright (c) 1992 by David Moore.  All rights reserved. */
#include "copyright.h"
/* newpass.c,v 2.4 1993/12/10 04:35:32 dmoore Exp */

#include "config.h"

#include "db.h"
#include "externs.h"


void do_newpassword(const dbref player, const char *name, const char *password, const char *ignore)
{
    dbref victim;
    
    if (!Wizard(player)) {
        notify(player, "Your delusions of grandeur have been duly noted.");
        return;
    }

    victim = lookup_player(name);
    if (victim == NOTHING) {
        notify(player, "No such player.");
        return;
    }

    if (*password && !ok_password(password)) {
        /* Wiz can set null passwords, but not bad passwords */
        notify(player, "Bad password");
        return;
    }

    if (TrueGod(victim) && !God(player)) {
        notify(player, "You can't change God's password!");
        return;
    }

    /* it's ok, do it */
    SetPass(victim, password);
    notify(player, "Password changed.");
    notify(victim, "Your password has been changed by %n.", player);
    log_status("NEWPASS: %u by %u", victim, victim, player, player);
}

