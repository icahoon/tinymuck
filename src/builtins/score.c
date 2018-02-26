/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* score.c,v 2.3 1993/04/08 20:24:36 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"


void do_score(const dbref player, const char *ignore1, const char *ignore2, const char *ignore3)
{
    int p = GetPennies(player);
  
    notify(player, "You have %d penn%s.", p, (p == 1) ? "y" : "ies");
}

