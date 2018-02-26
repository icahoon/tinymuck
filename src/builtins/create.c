/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* create.c,v 2.3 1993/04/08 20:24:39 dmoore Exp */
#include "config.h"

#include <stdlib.h>

#include "db.h"
#include "params.h"
#include "externs.h"


void do_create(const dbref player, const char *name, const char *scost, const char *ignore)
{
    dbref loc;
    dbref link;
    dbref thing;
    int cost, value;
  
    cost = atoi(scost);

    if (!Builder(player)) {
	notify(player, "That command is restricted to authorized builders.");
	return;
    } 

    if (!*name) {
	notify(player, "Create what?");
	return;
    } else if (!ok_name(name)) {
	notify(player, "That's a silly name for a thing!");
	return;
    }

    if (cost < 0) {
	notify(player, "You can't create an object for less than nothing!");
	return;
    } else if (cost < OBJECT_COST) {
	cost = OBJECT_COST;
    }
  
    if (!payfor(player, cost)) {
	notify(player, "Sorry, you don't have enough pennies.");
	return;
    }

    /* Decide where to link the object. */
    loc = GetLoc(player);
    if (controls(player, loc, Wizard(player))) {
	link = loc;		/* Use this room. */
    } else {
	link = GetLink(player); /* Use player's home. */
    }

    /* Actually create the object. */
    thing = make_object(TYPE_THING, name, player, player, link);
    SetActions(thing, NOTHING);
    PushContents(player, thing);
    
    /* Set the value of the object. */
    value = OBJECT_ENDOWMENT(cost);
    if (value > MAX_OBJECT_ENDOWMENT)
	value = MAX_OBJECT_ENDOWMENT;
    SetPennies(thing, value);
    
    notify(player, "%s created with number %d.", name, thing);
}
