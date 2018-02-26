/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* open.c,v 2.4 1993/04/10 05:09:13 dmoore Exp */
#include "config.h"

#include "db.h"
#include "params.h"
#include "externs.h"


/* Should really be in a header file.  Code is in builtins/link.c. */
extern int link_exit(const dbref player, const dbref exit, const char *dest_name, dbref *dest_list);


void do_open(const dbref player, const char *direction, const char *linkto, const char *ignore)
{
    dbref loc;
    dbref exit;
    dbref dest[MAX_LINKS];
    dbref *temp;
    int  i, ndest;
  
    if (!Builder(player)) {
	notify(player, "That command is restricted to authorized builders.");
	return;
    }
  
    if (!*direction) {
	notify(player, "You must specify a direction or action name to open.");
	return;
    } else if (!ok_name(direction)) {
	notify(player, "That's a strange name for an exit!");
	return;
    }

    loc = GetLoc(player);

    if (!controls(player, loc, Wizard(player))) {
	notify(player, "Permission denied.");
	return;
    }

    if (!payfor(player, EXIT_COST)) {
	notify(player,
	       "Sorry, you don't have enough pennies to open an exit.");
	return;
    }

    /* Make it. */
    exit = make_object(TYPE_EXIT, direction, player, loc, NOTHING);
    PushActions(loc, exit);
    
    notify(player, "Exit opened with number %d.", exit);
    
    /* check second arg to see if we should do a link */
    if (!*linkto) return;

    notify(player, "Trying to link...");
    if (!payfor(player, LINK_COST)) {
	notify(player, "You don't have enough pennies to link.");
	return;
    }
    ndest = link_exit(player, exit, linkto, dest);

    if (ndest == 0) {
	/* refund their money. */
	IncPennies(player, LINK_COST);

	SetNDest(exit, 0);
	SetDest(exit, NULL);
    } else {
	SetNDest(exit, ndest);
	MALLOC(temp, dbref, ndest);
	SetDest(exit, temp);

	for (i = 0; i < ndest; i++, temp++) {
	    *temp = dest[i];
	}
    }
}

