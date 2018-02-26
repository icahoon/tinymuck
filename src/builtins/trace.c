/* Copyright (c) 1992 by David Moore.  All rights reserved. */
#include "copyright.h"
/* trace.c,v 2.3 1993/04/08 20:24:35 dmoore Exp */

#include "config.h"

#include <stdio.h>		/* atoi */

#include "db.h"
#include "match.h"
#include "externs.h"


void do_trace(const dbref player, const char *name, const char *sdepth, const char *ignore)
{
    dbref thing;
    int i;
    int depth;
    struct match_data md;

    depth = atoi(sdepth);

    if (!name || !*name) { 
	thing = GetLoc(player);
    } else {
	init_match(player, name, NOTYPE, &md);
	match_absolute(&md);
	match_here(&md);
	match_me(&md);
	match_neighbor(&md);
	match_possession(&md);
	if ((thing = noisy_match_result(&md)) == NOTHING
	    || thing == AMBIGUOUS) return;
    }

    for (i = 0; (!depth || i < depth) && thing != NOTHING; i++) {
	if (can_link_to(player, NOTYPE, thing))
	    notify(player, "%u", player, thing);
	else
	    notify(player, "**Missing**");
	thing = GetLoc(thing);
    }

    notify(player, "***End of List***");
}
