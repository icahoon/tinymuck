/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* dump.c,v 2.5 1993/12/16 08:25:04 dmoore Exp */
#include "config.h"

#include "db.h"
#include "externs.h"

extern int dump_flag;
extern const char *dumpfile;

void do_dump(const dbref player, const char *ignore1, const char *ignore2, const char *newfile)
{
    if (!Wizard(player)) {
	notify(player, "Sorry, you are in a no dumping zone.");
	return;
    }

    dump_flag = 1;
    if (*newfile && God(player)) {
        if (dumpfile) FREE_STRING(dumpfile);
        dumpfile = ALLOC_STRING(newfile);
	notify(player, "Dumping to file %s...", dumpfile);
    } else {
        notify(player, "Dumping...");
    }
}

