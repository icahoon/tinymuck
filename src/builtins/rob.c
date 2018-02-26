/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* rob.c,v 2.3 1993/04/08 20:24:34 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "externs.h"

void do_rob(const dbref player, const char *what, const char *ignore1, const char *ignore2)
{
    dbref thing;
    struct match_data md;
    
    init_match(player, what, TYPE_PLAYER, &md);
    match_neighbor(&md);
    match_me(&md);
    if (Wizard(player)) {
        match_absolute(&md);
        match_player(&md);
    }
    thing = match_result(&md);
    
    switch (thing) {
    case NOTHING:
        notify(player, "Rob whom?");
        return;
        break;
    case AMBIGUOUS:
        notify(player, "I don't know who you mean!");
        return;
        break;
    }

    if (Typeof(thing) != TYPE_PLAYER) {
        notify(player, "Sorry, you can only rob other players.");
	return;
    }

    if (GetPennies(thing) < 1) {
        notify(player, "%n is penniless.", thing);
        notify(thing,
               "%n tried to rob you, but you have no pennies to take.",
               player);
        return;
    }

    if (can_doit(player, thing, "Your conscience tells you not to.")) {
        /* steal a penny */
        IncPennies(player, 1);
        IncPennies(thing, -1);
        notify(player, "You stole a penny.");
        notify(thing, "%n stole one of your pennies!", player);
    }
}

