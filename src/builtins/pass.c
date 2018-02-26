/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* pass.c,v 2.5 1994/02/13 11:05:51 dmoore Exp */
#include "config.h"

#include <string.h>

#include "db.h"
#include "externs.h"

void do_password(const dbref player, const char *old, const char *new, const char *ignore)
{
    const char *pass;

    pass = GetPass(player);

    if (!pass || strcmp(old, pass)) {
	notify(player, "Sorry");
    } else if(!ok_password(new)) {
	notify(player, "Bad new password.");
    } else {
	SetPass(player, new);
	notify(player, "Password changed.");
    }
}

