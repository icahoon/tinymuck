/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* list.c,v 2.3 1993/04/08 20:24:38 dmoore Exp */
#include "config.h"

#include <ctype.h>

#include "db.h"
#include "buffer.h"
#include "match.h"
#include "params.h"
#include "externs.h"


void do_list(const dbref player, const char *name, const char *lines, const char *ignore)
{
    struct match_data md;
    dbref thing;
    const char *edit_args[MAX_ARG];
    int i;
    Buffer buf;
    
    init_match(player, name, TYPE_PROGRAM, &md);
    match_neighbor(&md);
    match_possession(&md);
    match_absolute(&md);

    thing = noisy_match_result(&md);
    if (thing == NOTHING) return;

    if (Typeof(thing) != TYPE_PROGRAM) {
	notify(player, "You can't list anything but a program.");
	return;
    }

    if (!(controls(player, thing, Wizard(player)) || Linkable(thing))) {
	notify(player, "Permission denied.");
	return;
    }

    /* 0 out the args. */
    for (i = 0; i < MAX_ARG; i++) edit_args[i] = "";
    

    if (!*lines) {
	edit_args[0] = "1";
	edit_args[1] = "-1";
    } else {
	/* Get first argument. */
	Bufcpy(&buf, "");
	while (*lines && isdigit(*lines)) Bufcat_char(&buf, *lines++);
	if (*Buftext(&buf)) edit_args[0] = Buftext(&buf);
	else edit_args[0] = "1";

	/* Get second argument. */
	if (*lines) {
	    while (*lines && !isdigit(*lines)) lines++;
	    if (*lines) edit_args[1] = lines;
	    else edit_args[1] = "-1";
	}
    }

    do_edit_list(player, thing, edit_args);
}
