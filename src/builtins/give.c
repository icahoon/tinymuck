/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* give.c,v 2.3 1993/04/08 20:24:38 dmoore Exp */
#include "config.h"

#include <stdlib.h>		/* atoi */

#include "db.h"
#include "params.h"
#include "match.h"
#include "externs.h"


void do_give(const dbref player, const char *recipient, const char *samount, const char *ignore)
{
    dbref who;
    int amount;
    struct match_data md;
    
    amount = atoi(samount);
    /* do amount consistency check */
    if (amount < 0 && !Wizard(player)) {
	notify(player, "Try using the \"rob\" command.");
	return;
    } else if (amount == 0) {
	notify(player, "You must specify a positive number of pennies.");
	return;
    }
    
    /* check recipient */
    init_match(player, recipient, TYPE_PLAYER, &md);
    match_neighbor(&md);
    match_me(&md);
    if (Wizard(player)) {
	match_player(&md);
	match_absolute(&md);
    }
    who = match_result(&md);
    
    switch (who) {
    case NOTHING:
	notify(player, "Give to whom?");
	return;
    case AMBIGUOUS:
	notify(player, "I don't know who you mean!");
	return;
    default:
	if (!Wizard(player)) {
	    if (Typeof(who) != TYPE_PLAYER) {
		notify(player, "You can only give to other players.");
		return;
	    } else if (GetPennies(who) + amount > MAX_PENNIES) {
		notify(player, "That player doesn't need that many pennies!");
		return;
	    }
	}
	break;
    }
    
    /* try to do the give */
    if (!payfor(player, amount)) {
	notify(player, "You don't have that many pennies to give!");
    } else {
	/* she can do it */
	switch (Typeof(who)) {
	case TYPE_PLAYER:
	    IncPennies(who, amount);
	    notify(player, "You give %d penn%s to %n.", amount,
		   (amount == 1) ? "y" : "ies", who);
	    notify(who, "%n gives you %d penn%s.", player, amount,
		   (amount == 1) ? "y" : "ies");
	    break;
	case TYPE_THING:
	    SetPennies(who, amount);
	    notify(player, "You change the value of %n to %d penn%s.",
		   who, amount, (amount == 1) ? "y" : "ies");
	    break;
	default:
	    notify(player, "You can't give pennies to that!");
	    break;
	}
    }
}
