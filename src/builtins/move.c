/* Copyright (c) 1992 by David Moore.  All rights reserved. */
#include "copyright.h"
/* move.c,v 2.4 1993/12/16 07:26:26 dmoore Exp */

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "db.h"
#include "match.h"
#include "buffer.h"
#include "externs.h"

/*
 * trigger()
 *
 * This procedure triggers a series of actions, or meta-actions
 * which are contained in the 'dest' field of the exit.
 * Locks other than the first one are over-ridden.
 * 
 * `player' is the player who triggered the exit
 * `exit' is the exit triggered
 * `pflag' is a flag which indicates whether player and room exits
 * are to be used (non-zero) or ignored (zero).  Note that
 * player/room destinations triggered via a meta-link are
 * ignored.
 *
 */


static void trigger(const dbref player, const dbref exit, const int pflag)
{
    int  i, ndest;
    dbref dest, *temp;
    int  sobjact;	/* sticky object action flag, sends home source obj */
    int  succ;
    sobjact = 0;
    succ = 0;
    
    ndest = GetNDest(exit);
    temp = GetDest(exit);
    for (i = 0; i < ndest; i++, temp++) {
	dest = *temp;
	if (dest == HOME) dest = GetLink(player);
	switch (Typeof(dest)) {
	case TYPE_ROOM:
	    if (pflag) {
		if (GetDrop(exit))
		    exec_or_notify(player, exit, GetDrop(exit), 0);
		if (GetODrop(exit) && !Dark(player)) {
		    notify_except(dest, player, "%n %p",
				  player, player, GetODrop(exit));
		}
		move_object(player, dest, exit);
		succ = 1;
	    }
	    break;
	case TYPE_THING:
	    if (Typeof(GetLoc(exit)) == TYPE_THING) {
		move_object(dest, GetLoc(GetLoc(exit)), exit);
		if (!HasFlag(exit, STICKY)) {
		    /* send home source object */
		    sobjact = 1;
		}
	    } else {
		move_object(dest, GetLoc(exit), exit);
	    }
	    if (GetSucc(exit)) succ = 1;
	    break;
	case TYPE_EXIT:		/* It's a meta-link(tm)! */
	    trigger(player, dest, 0);
	    if (GetSucc(exit)) succ = 1;
	    break;
	case TYPE_PLAYER:
	    if (pflag && GetLoc(dest) != NOTHING) {
		succ = 1;
		if (HasFlag(dest, JUMP_OK)) {
		    if (GetDrop(exit))
			exec_or_notify(player, exit, GetDrop(exit), 0);
		    if (GetODrop(exit) && !Dark(player)) {
			notify_except(GetLoc(dest), player,
				      "%n %p", player, player, GetODrop(exit));
		    }
		    move_object(player, GetLoc(dest), exit);
		} else {
		    notify(player,
			   "That player does not wish to be disturbed.");
		}
	    }
	    break;
	case TYPE_PROGRAM:
	    interp(player, dest, exit, INTERP_OK_THREAD, Buftext(&match_args));
	    return;
	}
    }
    if (sobjact) send_home(GetLoc(exit));
    if (!succ && pflag) notify(player, "Done.");
}


static void do_move_command(const dbref player, const dbref exit)
{    
    switch (exit) {
    case NOTHING:
	notify(player, "You can't go that way.");
	break;
    case AMBIGUOUS:
	notify(player, "I don't know which way you mean!");
	break;
    case HOME:
	/* send him home */
	/* but steal all his possessions */
	notify_except(GetLoc(player), player, "%n goes home.", player);
	/* give the player the messages */
	notify(player, "There's no place like home...");
	notify(player, "There's no place like home...");
	notify(player, "There's no place like home...");
	notify(player, "You wake up back home, without your possessions.");
	send_home(player);
	break;
    default:
	/* we got one */
	/* check to see if we got through */
	if (can_doit(player, exit, "You can't go that way.")) {
	    trigger(player, exit, 1);
	}
	break;
    }
}


/* go <direction> */
void do_move(const dbref player, const char *ign1, const char *ign2, const char *direction)
{
    struct match_data md;

    /* check_keys makes sure that for multiple matches, only 'doable' exits
       are chosen as matches */
    init_match_check_keys(player, direction, TYPE_EXIT, &md);
    match_home(&md);
    match_all_exits(&md);

    do_move_command(player, match_result(&md));
}


/* returns true if it was a valid exit and it has been gone through. */
int try_move(const dbref player, const char *direction)
{
    struct match_data md;

    /* check_keys makes sure that for multiple matches, only 'doable' exits
       are chosen as matches */
    init_match_check_keys(player, direction, TYPE_EXIT, &md);
    match_home(&md);
    match_all_exits(&md);

    if (last_match_result(&md) == NOTHING) {
	return 0;
    } else {
	do_move_command(player, match_result(&md));
	return 1;
    }
    
}
