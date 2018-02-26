/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* move.c,v 2.6 1994/03/10 04:13:43 dmoore Exp */
#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "ansify.h"		/* random for next */
#include "db.h"
#include "params.h"
#include "match.h"
#include "externs.h"

void send_contents(const dbref loc, const dbref dest)
{
    dbref first;
    dbref rest;
    
    first = GetContents(loc);
    SetContents(loc, NOTHING);
    
    /* blast locations of everything in list */
    DOLIST(rest, first) {
	SetLoc(rest, NOTHING);
    }
    
    while(first != NOTHING) {
	rest = GetNext(first);
	db_move(first, loc);
	if ((Typeof(first) == TYPE_THING) || (Typeof(first) == TYPE_PROGRAM)) {
	    move_object(first,
			HasFlag(first, STICKY) ? GetHome(first) : dest,
			NOTHING);
	}
	first = rest;
    }
    
    SetContents(loc, reverse(GetContents(loc)));
}


int moveto_type_ok(const dbref thing, const dbref dest)
{
    int dest_type = Typeof(dest);

    if (HasFlag(dest, BEING_RECYCLED)) /* room is being emptied for recycle */
        return 0;

    switch (Typeof(thing)) {
    case TYPE_ROOM:
    case TYPE_PLAYER:
	return (dest_type == TYPE_ROOM);
	break;
    case TYPE_THING:
    case TYPE_PROGRAM:
	return (dest_type == TYPE_ROOM) || (dest_type == TYPE_PLAYER);
	break;
    case TYPE_EXIT:
    case TYPE_GARBAGE:
    default:
	return 0;
	break;
    }
}

/* Note about below.  JUMP_OK is stupid.  It means 2 things.  And it should
   have been two flags.  1) If it's not a room you can move this thing.
   2) You can stick stuff in this thing.  This is fine, except on players.
   A player who is JUMP_OK can both be moved and have stuff stuck on them.
   This is noticably stupid.  It'd be much smarter to have two flags:
   JUMP_OK and MOVE_OK.  Jump_ok would control if you could stick things
   on it, and move_ok would control if you could move it.  I've written
   the rules below to support this scheme.  You can decide if you want
   MOVE_Ok to really be a seperate flag. */
#define MOVE_OK JUMP_OK

/* I'm not sure if I like the rules which reason:
   ''You could steal it.'', ''You could walk an exit.'',
   ''You could home it to there.''
   It complicates matters a fair amount...and do you gain much?
*/


/* You can move something out of a source if one of the following is true:
   1) You control the source.
   2) You control the thing.
   3) The source is JUMP_OK.
   4) (?) The thing is an object and link_ok?  (You could steal it...)
   Otherwise it's illegal. */
int moveto_source_ok(const dbref thing, const dbref dest, const dbref player, const int wizard)
{
    dbref source = GetLoc(thing);

    if (source == NOTHING) return 1; /* In case something gets in there? */
    if (controls(player, source, wizard)) return 1;
    if (controls(player, thing, wizard)) return 1;
    if (HasFlag(source, JUMP_OK)) return 1;

    return 0;
}


/* You can move something if one of the following is true:
   1) You control it.
   2) It's set MOVE_OK.
   3) You control the source, and the destination is the object's home.
   Otherwise you can't move it. */
int moveto_thing_ok(const dbref thing, const dbref dest, const dbref player, const int wizard)
{
    if (controls(player, thing, wizard)) return 1;
    if (HasFlag(thing, MOVE_OK)) return 1;
    if (controls(player, GetLoc(thing), wizard)
	&& (GetHome(thing) == dest)) return 1;

    return 0;
}


/* You can move something to a destination if one of the following is true:
   1) You control the destination.
   2) The destination is the HOME of the thing.
   3) The destination is JUMP_OK.
   4) The destination and thing are both rooms, and the destination
      is LINK_OK or ABODE.                            (Parenting rule.)
   5) (?) The destination is a room which is LINK_OK,
      and the thing is a player.                      (Could walk an exit.)
   6) (?) You can home the thing to the destination. (Could home it there.)
   Otherwise you can't stick it there. */
int moveto_dest_ok(const dbref thing, const dbref dest, const dbref player, const int wizard)
{
    if (controls(player, dest, wizard)) return 1;
    if (GetHome(thing) == dest) return 1;
    if (HasFlag(dest, JUMP_OK)) return 1;
    if ((Typeof(dest) == TYPE_ROOM)
	&& (Typeof(thing) == TYPE_ROOM)
	&& (HasFlag(dest, LINK_OK|ABODE))) return 1;

    return 0;
}


/* Checks if a parent chain loop would occur if this move was made. */
int moveto_loop_ok(const dbref thing, dbref dest)
{
    while (dest != NOTHING) {
	if (thing == dest) return 0;
	dest = GetLoc(dest);
    }

    return 1;
}


static void sticky_dropto(const dbref loc)
{
    dbref dropto = GetLink(loc);
    dbref thing;
    
    if (dropto == NOTHING) return;
    if (loc == dropto) return;	/* bizarre special case */
    
    /* check for players */
    DOLIST(thing, GetContents(loc)) {
	if (Typeof(thing) == TYPE_PLAYER) return;
    }
    
    /* no players, send everything to the dropto */
    send_contents(loc, dropto);
}


/* This part of the move is dark if the player, location, or exit is dark. */
static int check_dark_move(const dbref player, const dbref loc, const dbref exit)
{
    if (Dark(player)) return 1;
    if (Dark(loc)) return 1;
    if (Garbage(exit)) return 0; /* Probably exit == NOTHING. */
    if ((Typeof(exit) == TYPE_EXIT) && Dark(exit)) return 1;

    return 0;
}


static void player_exit_room(const dbref player, const dbref old, const dbref new, const dbref exit)
{
    if (Garbage(old)) return;	/* Takes care of NOTHING and garbage. */
    if (old == new) return;	/* Don't show messages if not moving. */

    if (!check_dark_move(player, old, exit)) {
	notify_except(old, player, "%n has left.", player);
    }
	
    if (HasFlag(old, STICKY)) {
	sticky_dropto(old);
    }
}


void player_enter_room(const dbref player, const dbref old, const dbref new, const dbref exit)
{
    if (Garbage(new)) return;

    if (old != new) {
	if (!check_dark_move(player, new, exit)) {
	    notify_except(new, player, "%n has arrived.", player);
	}
    }

    do_look(player, "", "", "");
    
#if (PENNY_RATE) != 0
    /* check for pennies */
    if (!controls(player, new, Wizard(player))
	&& GetPennies(player) <= MAX_PENNIES
	&& random() % PENNY_RATE == 0) {
	notify(player, "You found a penny!");
	IncPennies(player, 1);
    }
#endif
}


void move_object(const dbref thing, dbref dest, const dbref exit)
{
    dbref source = GetLoc(thing);
    
    if (dest == HOME)
	dest = GetHome(thing);
    
    /* This is different than 2.2 w/ respect to players.  Before it wouldn't
       alter a players position in the contents list if they didn't actually
       move.  By having it reorder all objects makes it far more consistent. */
    db_move(thing, dest);

    if (Typeof(thing) == TYPE_PLAYER) {
	player_exit_room(thing, source, dest, exit);
	player_enter_room(thing, source, dest, exit);
    }

}


void send_home(const dbref thing)
{
    if (Typeof(thing) == TYPE_PLAYER) {
	send_contents(thing, HOME);
    }

    move_object(thing, GetHome(thing), NOTHING);
}



