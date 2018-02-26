/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* prog.c,v 2.4 1994/02/04 02:13:30 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "buffer.h"
#include "externs.h"

void do_prog(const dbref player, const char *name, const char *ignore1, const char *ignore2)
{
    dbref prog;
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
    prog = match_result(&md);

    if (prog == AMBIGUOUS) {
	notify(player, "I don't know which one you mean!");
	return;
    }

    if (prog == NOTHING) {
	prog = make_object(TYPE_PROGRAM, name, player, player, player);

	PushContents(player, prog);
	notify(player, "Program %s created with number %d.",
	       name, prog);

	begin_editing(player, prog, 0);
    } else {
	if ((Typeof(prog) != TYPE_PROGRAM)
	    || !controls(player, prog, Wizard(player))) {
	    notify(player, "Permission denied!");
	    return;
	}

	begin_editing(player, prog, 1);
    }
}
