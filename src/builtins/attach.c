/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* attach.c,v 2.3 1993/04/08 20:24:39 dmoore Exp */
#include "config.h"

#include "db.h"
#include "match.h"
#include "params.h"
#include "externs.h"


/* This forward defintion should really be in some header file. */
dbref parse_source(const dbref player, const char *source_name);


void do_attach(const dbref player, const char *action_name, const char *source_name, const char *ignore)
{
    dbref action, source;
    struct match_data md;
  
    if (!Builder(player)) {
	notify(player, "That command is restricted to authorized builders.");
	return;
    }
  
    if (!*action_name || !*source_name) {
	notify(player, "You must specify an action name and a source object.");
	return;
    }
  
    init_match(player, action_name, TYPE_EXIT, &md);
    match_all_exits(&md);
    if (Wizard(player)) {
	match_absolute(&md);
    }
    action = noisy_match_result(&md);

    if (action == NOTHING) return;
  
    if (Typeof(action) != TYPE_EXIT) {
	notify(player, "That's not an action!");
	return;
    } else if (!controls(player, action, Wizard(player))) {
	notify(player, "Permission denied.");
	return;
    }

    source = parse_source(player, source_name);

    if (source == NOTHING) return;
  
    db_move(action, source);

    notify(player, "Action re-attached.");
}


dbref parse_source(const dbref player, const char *source_name)
{
    dbref source;
    struct match_data md;
  
    init_match(player, source_name, NOTYPE, &md);
    match_neighbor(&md);
    match_me(&md);
    match_here(&md);
    match_possession(&md);
    match_absolute(&md);
    source = noisy_match_result(&md);
  
    if (source == NOTHING) return NOTHING;
  
    /* You can only attach actions to things you control */
    if (!controls(player, source, Wizard(player))) {
	notify(player, "Permission denied.");
	return NOTHING;
    }

    if (Typeof(source) == TYPE_EXIT) {
	notify(player, "You can't attach an action to an action.");
	return NOTHING;
    }

    if (Typeof(source) == TYPE_PROGRAM) {
	notify(player, "You can't attach an action to a program.");
	return NOTHING;
    }

    return source;
}

