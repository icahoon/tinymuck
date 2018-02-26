/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* gripe.c,v 2.3 1993/04/08 20:24:35 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"


void do_gripe(const dbref player, const char *ignore1, const char *ignore2, const char *message)
{
    dbref loc = GetLoc(player);
    
    log_gripe("GRIPE from %u in %u: %s", player, player, loc, loc, message);
    notify(player, "Your complaint has been duly noted.");
}

