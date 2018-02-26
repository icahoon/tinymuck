/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* fail.c,v 2.3 1993/04/08 20:24:37 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"


void do_fail(const dbref player, const char *name, const char *message, const char *ignore)
{
    dbref thing;
    
    thing = match_controlled(player, name);
    if (thing != NOTHING) {
	SetFail(thing, message);
	notify(player, "Message set.");
    }
}
