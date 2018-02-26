/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* action.c,v 2.3 1993/04/08 20:24:39 dmoore Exp */
#include "config.h"

#include "db.h"
#include "params.h"
#include "externs.h"


/* Uses parse_source from builtins/attach.c.  Should really be in some
   header file. */
extern dbref parse_source(const dbref player, const char *source_name);


void do_action(const dbref player, const char *action_name, const char *source_name, const char *ignore)
{
    dbref action, source;
  
    if (!Builder(player)) {
	notify(player, "That command is restricted to authorized builders.");
	return;
    }
  
    if (!*action_name || !*source_name) {
	notify(player, "You must specify an action name and a source object.");
	return;
    } else if (!ok_name(action_name)) {
	notify(player, "That's a strange name for an action!");
	return;
    }
    
    if (!payfor(player, EXIT_COST)) {
	notify(player,
	       "Sorry, you don't have enough pennies to make an action.");
	return;
    }
   
    source = parse_source(player, source_name);

    if (source == NOTHING) return;
  
    action = make_object(TYPE_EXIT, action_name, player, source, NOTHING);
    PushActions(source, action);

    notify(player, "Action created with number %d and attached.", action);
}
