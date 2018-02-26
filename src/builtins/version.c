/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* version.c,v 2.3 1993/04/08 20:24:38 dmoore Exp */
#include "config.h"

#include "patchlevel.h"
#include "db.h"
#include "params.h"
#include "externs.h"


void do_version(const dbref player, const char *ignore1, const char *ignore2, const char *ign3)
{
#if PATCHLEVEL
    notify(player, "Version: %s PatchLevel: %i", MUCK_VERSION, PATCHLEVEL);
#else
    notify(player, "Version: %s", MUCK_VERSION);
#endif
}

