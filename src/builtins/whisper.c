/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* whisper.c,v 2.5 1993/11/03 22:44:19 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "externs.h"


void do_whisper(const dbref player, const char *arg1, const char *arg2, const char *ignore)
{
    int succeed;
    dbref who;
    struct match_data md;
    
    init_match(player, arg1, TYPE_PLAYER, &md);
    match_neighbor(&md);
    match_me(&md);
    if (Wizard(player)) {
	match_absolute(&md);
	match_player(&md);
    }
    who = match_result(&md);

    switch (who) {
    case NOTHING:
	notify(player, "Whisper to whom?");
	break;
    case AMBIGUOUS:
	notify(player, "I don't know who you mean!");
	break;
    default:
	if (Typeof(who) == TYPE_PLAYER) {
	    succeed = notify(who, "%n whispers, \"%s\"", player, arg2);
	    if (!succeed) {
		notify(player, "%n is not connected.", who);
	    } else {
		notify(player, "You whisper, \"%s\" to %n.", arg2, who);
	    }
	} else {
	    notify(player, "%n is not a player.", who);
	}
	break;
    }
}
