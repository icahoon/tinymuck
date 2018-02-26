/* Copyright (c) 1992 by David Moore.  All rights reserved. */
#include "copyright.h"
/* utils.c,v 2.5 1993/12/16 07:27:02 dmoore Exp */

#include "config.h"

#include <ctype.h>

#include "db.h"
#include "externs.h"

#define EXEC_SIGNAL '@'    /* Symbol which tells us what we're looking at
			    * is an execution order and not a message.    */


/* remove the first occurence of what in list headed by first */
dbref remove_first(const dbref first, const dbref what)
{
    dbref prev;
  
    /* special case if it's the first one */
    if (first == what) {
	return GetNext(first);
    } else {
	/* have to find it */
	DOLIST(prev, first) {
	    if (GetNext(prev) == what) {
		SetNext(prev, GetNext(what));
		return first;
	    }
	}
	return first;
    }
}


int member(const dbref thing, dbref list)
{
    DOLIST(list, list) {
	if (list == thing) return 1;
    }
  
    return 0;
}


dbref reverse(dbref list)
{
    dbref newlist;
    dbref rest;
  
    newlist = NOTHING;
    while (list != NOTHING) {
	rest = GetNext(list);
	SetNext(list, newlist);
	newlist = list;
	list = rest;
    }
    return newlist;
}


void exec_or_notify(const dbref player, const dbref thing, const char *msg, const int quiet)
{
    Buffer buf;
    dbref program;
    int try_to_run;

    if (!msg) return;

    if (*msg != EXEC_SIGNAL) {
	notify(player, "%s", msg);
	return;
    }

    /* Store off the program info into the buffer. */
    Bufcpy(&buf, "");
    msg++;
    while (*msg && !isspace(*msg)) Bufcat_char(&buf, *msg++);
    if (*msg) msg++;

    /* Look up the program. */
    program = parse_dbref(Buftext(&buf));

    /* Only try to run it, if it's a valid dbref of type program, and
       if the player isn't already in a program. */
    try_to_run = (!Garbage(program) && (Typeof(program) == TYPE_PROGRAM)
		  && !running_program(player));
    
    if (try_to_run) {
	/* interp_no_thread means that read/sleep are illegal. */
	interp(player, program, thing, INTERP_NO_THREAD, msg);
    } else if (!quiet) {
	if (*msg) {
	    notify(player, "%s", msg);
	} else {
	    notify(player, "You see nothing special.");
	}
    }
}

