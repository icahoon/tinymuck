/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* edit.c,v 2.3 1993/04/08 20:24:39 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "externs.h"


void do_edit(const dbref player, const char *name, const char *ignore1, const char *ignore2)
{
    dbref  prog;
    struct match_data md;

    if (!Mucker(player)) {
	notify(player, "You're no programmer!");
	return;
    }

    if (!*name) {
	notify(player, "No program name given.");
	return;
    }

    init_match(player, name, TYPE_PROGRAM, &md);
  
    match_possession(&md);
    match_neighbor(&md);
    match_absolute(&md);
    prog = noisy_match_result(&md);
    
    if (prog == NOTHING) {
	/* The message was already shown by noisy_match_result. */
	return;
    }
  
    if ((Typeof(prog) != TYPE_PROGRAM)
	|| !controls(player, prog, Wizard(player))) {
	notify(player, "Permission denied!");
	return;
    }

    begin_editing(player, prog, 1);
}
