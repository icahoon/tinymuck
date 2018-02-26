/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* page.c,v 2.4 1993/12/09 08:36:04 dmoore Exp */
#include "config.h"

#include <ctype.h>

#include "db.h"
#include "params.h"			/* PAGE_COST */
#include "externs.h"


static int blank(const char *s)
{
    while (*s && isspace(*s)) s++;
    
    return !(*s);
}


void do_page(const dbref player, const char *who, const char *mesg, const char *ignore)
{
    int succeed;
    dbref target;
    
    if ((!payfor(player, PAGE_COST))
	|| (!Wizard(player) && (GetPennies(player) < 0))) {
	notify(player, "You don't have enough pennies.");
	return;
    }

    target = lookup_player(who);
    if (target == NOTHING) {
	notify(player, "I don't recognize that name.");
	return;
    }

    if (HasFlag(target, HAVEN)) {
	notify(player, "That player does not wish to be disturbed.");
	return;
    }

    if (blank(mesg)) {
	succeed = notify(target, "You sense that %n is looking for you in %n.",
			 player, GetLoc(player));
    } else {
	succeed = notify(target, "%n pages from %n: \"%s\"",
			 player, GetLoc(player), mesg);
    }

    if (succeed)
	notify(player, "Your message has been sent.");
    else {
	notify(player, "%n is not connected.", target);
    }
}

