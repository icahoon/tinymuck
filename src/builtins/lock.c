/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* lock.c,v 2.5 1997/08/15 22:53:54 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "externs.h"


void do_lock(const dbref player, const char *name, const char *keyname, const char *ignore)
{
    dbref thing;
    struct boolexp *key;
    struct match_data md;
    
    init_match(player, name, NOTYPE, &md);
    match_everything(&md, Wizard(player));
    thing = match_result(&md);

    switch (thing) {
    case NOTHING:
	notify(player, "I don't see what you want to lock!");
	return;
    case AMBIGUOUS:
	notify(player, "I don't know which one you want to lock!");
	return;
    default:
	if (!controls(player, thing, Wizard(player))) {
	    notify(player, "You can't lock that!");
	    return;
	}
	break;
    }
    
    /* Parse it in normal mud format (0). */
    key = parse_boolexp(player, keyname, 0);
    if (key == TRUE_BOOLEXP) {
	notify(player, "I don't understand that key.");
    } else {
	SetKey(thing, key);
	notify(player, "Locked.");
    }
}
