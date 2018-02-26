/* Copyright (c) 1992 by David Moore.  All rights reserved. */
#include "copyright.h"
/* unlink.c,v 2.3 1993/04/08 20:24:37 dmoore Exp */

#include "config.h"

#include "db.h"
#include "params.h"		/* LINK_COST */
#include "match.h"
#include "externs.h"


void do_unlink(const dbref player, const char *name, const char *ignore1, const char *ignore2)
{
    dbref exit;
    struct match_data md;
    
    init_match(player, name, TYPE_EXIT, &md);
    match_all_exits(&md);
    match_here(&md);
    if (Wizard(player)) {
	match_absolute(&md);
    }
    
    exit = match_result(&md);
    switch (exit) {
    case NOTHING:
	notify(player, "Unlink what?");
	break;
    case AMBIGUOUS:
	notify(player, "I don't know which one you mean!");
	break;
    default:
	if (!controls(player, exit, Wizard(player))) {
	    notify(player, "Permission denied.");
	} else {
	    switch (Typeof(exit)) {
	    case TYPE_EXIT:
		if (GetNDest(exit) > 0) {
		    IncPennies(GetOwner(exit), LINK_COST);
		}
		FreeLink(exit);
		notify(player, "Unlinked.");
		break;
	    case TYPE_ROOM:
		FreeLink(exit);
		notify(player, "Dropto removed.");
		break;
	    default:
		notify(player, "You can't unlink that!");
		break;
	    }
	}
    }
}
