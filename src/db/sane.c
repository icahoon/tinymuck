/* Copyright (c) 1996 by David Moore.  All rights reserved. */
/* sane.c,v 2.4 1996/09/25 04:02:10 dmoore Exp */

/* There are two large classifications of database corruption.
   1. Internal datastructures have become damaged (such as a bad
      location, or damaged contents lists) but the disk datafile can
      be reloaded to obtain the same state.
   2. The file on disk has been damaged and cannot be loaded.

   This file only attempts to discover and repair faults of the first
   type.  Database backends should try to detect damaged disk files.
   */


/* New ordered steps:
   0. Check every field on every object.  Set bad things to NOTHING.
      Set bad owners to head_player.  Also clean destinations, droptos, etc.
      Clean up HOME references.
   1. Check contents/action lists, breaking _o and Y.  Also remove
      any objects which are of an incorrect type to be on this list.
      Remove any objects which have a location which is not this object.
      (ie. location field wins over contents/action lists)
   *. Before here, only use SetLoc.  After here use db_move.
   2. Check object locations, breaking parenting loops.  Move detached
      objects to reasonable places.  And make sure everything is in
      the correct contents/actions list.
   3. Warnings about names, flags, etc.  */


/* To do above:
   0. check_fields
   1. check_cycles
   2. check_locations
*/

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ansify.h"
#include "db.h"
#include "params.h"
#include "buffer.h"
#include "externs.h"

/* Set to true when a fatal error is found. */
static int fatal_sanity = 0;

/* Set true if repairs should be attempted.  Often simple repairs will
   be done w/o checking this flag, since they cost nothing.  More expensive
   repair operations may look at this. */
static int do_repair = 0;

/* Set to true if warnings should be made for flag security issues.
   Such as wizzed/dark players, etc. */
static int check_security = 0;


static int prop_warn = 0;		/* 200 */
static int strlen_warn = 0;		/* 1024 */
static int object_cnt_warn = 0;		/* 200 */


/* Holds the functions to call for warnings and fatal errors. */
static void warn(const dbref obj, const char *fmt, ...);
static void fatal(const dbref obj, const char *fmt, ...);
static void repair(const dbref obj, const char *fmt, ...);



#define RELATION_Next		1
#define RELATION_Loc		2
#define RELATION_Owner		3
#define RELATION_Contents	4
#define RELATION_Actions	5
#define RELATION_Dest		6

static int ok_type(const dbref field, const dbref obj, const int relation)
{
    object_flag_type obj_type;
    object_flag_type field_type;

    if (obj != NOTHING) {
	obj_type = Typeof(obj);
    } else {
	log_status("PANIC: sane.c/ok_type called with obj == NOTHING");
	obj_type = NOTYPE;
    }

    if (field != NOTHING) {
	field_type = Typeof(field);
    } else {
	field_type = NOTYPE;
    }

    switch (relation) {
    case RELATION_Next:
	/* Anything is ok. */
	return 1;
	break;

    case RELATION_Loc:
	/* The global_environment must be in nothing.
	   Players and rooms can only be in rooms.
	   Objects and programs can only be in rooms or players.
	   Exits can be on rooms, players, and objects. */
	   
	if (obj == global_environment)
	    return (field == NOTHING);
	
	switch (obj_type) {
	case TYPE_ROOM:
	case TYPE_PLAYER:
	    return (field_type == TYPE_ROOM);
	case TYPE_THING:
	case TYPE_PROGRAM:
	return (field_type == TYPE_ROOM) || (field_type == TYPE_PLAYER);
	case TYPE_EXIT:
	    return ((field_type == TYPE_ROOM)
		    || (field_type == TYPE_PLAYER)
		    || (field_type == TYPE_THING));
	}
	break;

    case RELATION_Owner:
	/* Owner must always be a player. */
	return (field_type == TYPE_PLAYER);
	break;

    case RELATION_Contents:
	/* Contents and location are similar, except don't allow exits
	   in contents. (old db's allow you to carry exits???) */
	if (field == NOTHING) return 1;
	if (field_type == TYPE_EXIT) return 0;
	return ok_type(obj, field, RELATION_Loc);
	break;

    case RELATION_Actions:
	/* Rooms, players, and things can have exits.
	   Anything else in an action is illegal. */
	if (field == NOTHING) return 1;

	switch (obj_type) {
	case TYPE_ROOM:
	case TYPE_PLAYER:
	case TYPE_THING:
            return (field_type == TYPE_EXIT);
	case TYPE_PROGRAM:
	case TYPE_EXIT:
	    return 0;
	}
	break;

    case RELATION_Dest:
	/* Rooms, players must go to rooms.
	   Programs and things can go to rooms or players.
	   Exits can go anywhere.
	   HOME is legal only for rooms and exits. */

	if (field == HOME)
	    return (obj_type == TYPE_ROOM) || (obj_type == TYPE_EXIT);

	switch (obj_type) {
	case TYPE_ROOM:
	case TYPE_PLAYER:
            return (field_type == TYPE_ROOM);
	case TYPE_THING:
	case TYPE_PROGRAM:
            return (field_type == TYPE_ROOM) || (field_type == TYPE_PLAYER);
	case TYPE_EXIT:
	    return 1;
	}
	break;
    }

    /* Shouldn't reach here. */
    return 0;
}


#define ok_dbref(x) \
    ((((x) >= 0) && ((x) < db_top) && (Typeof(x) != TYPE_GARBAGE)) || \
     ((x) == HOME) || \
     ((x) == NOTHING))

#define check_field(accessor, bad, home) \
    do { \
	dbref x = Get##accessor(obj); \
	if (!ok_dbref(x) || !ok_type(x, obj, RELATION_##accessor)) { \
	    repair(obj, "Bad field %s was #%d, set to %s (#%d).", \
		   x, #accessor, #bad, bad); \
	    Set##accessor(obj, bad); \
	} else if ((x) == HOME) { \
	    repair(obj, "Bad field %s was HOME (#%d), set to %s (#%d).", \
		   x, #accessor, #home, home); \
	    Set##accessor(obj, home); \
	} \
    } while (0)

		  
static void check_fields(void)
{
    dbref obj;
    dbref *dest, *fix_dest;
    int ndest, i;

    for (obj = 0; obj < db_top; obj++) {
	if (fatal_sanity) return;
	if (Typeof(obj) == TYPE_GARBAGE) continue;
	
	/* Check the normal fields. */
	check_field(Next, NOTHING, NOTHING);
	check_field(Loc, NOTHING, HOME);
	check_field(Owner, head_player, head_player);
	check_field(Contents, NOTHING, NOTHING);
	check_field(Actions, NOTHING, NOTHING);

	/* Check the destination field. */
	ndest = GetNDest(obj);
	if ((ndest > 1) && (Typeof(obj) != TYPE_EXIT)) {
	    /* Illegal multi-link. */
	    repair(obj,
		   "Non-exit object had multi-destination link; truncated.");
	    SetLink(obj, GetLink(obj));
	    ndest = GetNDest(obj);
	}

	if (ndest > MAX_LINKS) {
	    /* Too many destinations. */
	    repair(obj, "Exit has too many destinations, %i; truncated",
		   ndest);
	    SetNDest(obj, MAX_LINKS);
	    ndest = GetNDest(obj);
	}

	dest = fix_dest = GetDest(obj);
	for (i = 0; ndest--; dest++) {
	    if (ok_dbref(*dest) && ok_type(*dest, obj, RELATION_Dest)) {
		*fix_dest++ = *dest;
		i++;
	    } else {
		repair(obj, "Bad object (#%d) in destination; removed",
		       *dest);
	    }
	}

	if (i == 0) SetDest(obj, NULL);
	SetNDest(obj, i);

	/* Players, objects, programs must always have a home. */
	if (((Typeof(obj) == TYPE_PLAYER)
	     ||(Typeof(obj) == TYPE_THING)
	     ||(Typeof(obj) == TYPE_PROGRAM))
	    && (GetNDest(obj) != 1)) {
	    repair(obj, "Object has no home, changed to player_start.");
	    SetLink(obj, player_start);
	}

	/* If something's loc is HOME, fix that now. */
	if (GetLoc(obj) == HOME) {
	    if ((Typeof(obj) == TYPE_EXIT)
		|| (!ok_type(GetHome(obj), obj, RELATION_Loc))) {
		repair(obj, "Location was HOME, changed to NOTHING.");
		SetLoc(obj, NOTHING);
	    } else {
		repair(obj, "Location was HOME, changed to actual home.");
		SetLoc(obj, GetHome(obj));
	    }
	}

	/* At this point the object by itself is pretty sane. */
    }
}
#undef ok_dbref
#undef check_field



#define set_prev(x) \
    if (obj == first) { \
	first = (x); \
    } else { \
	SetNext(prev, (x)); \
    }


static dbref check_list_cycle(dbref first, const dbref parent, const int relation)
{
    dbref obj;
    dbref prev;			/* This is only used when first!=obj. */
    dbref next;
    const char *type_string;

    if (relation == RELATION_Contents) type_string = "contents";
    else type_string = "actions";
    
    /* obj is always ok_dbref */
    for (obj = first; obj != NOTHING; prev = obj, obj = next) {
	next = GetNext(obj);

	if (HasFlag(obj, VISITED)) {
	    /* Either a cycle (_o) or a doubly entered list (Y). */
	    /* Break list before this object. */
	    repair(parent, "Cycle or double entered list in %s list.",
		   type_string);
	    set_prev(NOTHING);
	    break;
	}
	SetOnFlag(obj, VISITED);

	if (!ok_type(obj, parent, relation)) {
	    repair(parent, "Bad object (#ld) in %s list; removed.",
		   obj, type_string);
	    set_prev(GetNext(obj));
	    SetNext(obj, NOTHING);
	}

	if (GetLoc(obj) != parent) {
	    if (GetLoc(obj) == NOTHING) {
		repair(obj,
		       "Missing location field set to #%d.", parent);
		SetLoc(obj, parent);
	    } else {
		repair(parent,
		       "Object (#%d) in %s list has other location; removed.",
		       obj, type_string);
		set_prev(GetNext(obj));
		SetNext(obj, NOTHING);
	    }
	}
    }

    return first;
}
#undef set_prev


#define check_list(how) \
    Set##how(obj, check_list_cycle(Get##how(obj), obj, RELATION_##how))

static void check_cycles(void)
{
    dbref obj;

    for (obj = 0; obj < db_top; obj++) {
	if (Typeof(obj) == TYPE_GARBAGE) continue;

	check_list(Contents);
	if (fatal_sanity) return;

	check_list(Actions);
	if (fatal_sanity) return;
    }

    for (obj = 0; obj < db_top; obj++)
	SetOffFlag(obj, VISITED);

}
#undef check_list



static int find_in_list(const dbref first, const dbref what)
{
    dbref x;

    DOLIST(x, first) {
	if (x == what) return 1;
    }

    return 0;
}


static void check_locations(void)
{
    dbref exit_room = NOTHING;
    dbref obj;
    dbref loc;
    dbref cur;

    for (obj = 0; obj < db_top; obj++) {
	if (fatal_sanity) return;
	if (Typeof(obj) == TYPE_GARBAGE) continue;

	loc = GetLoc(obj);

	if (obj == global_environment) {
	    /* Special case. */
	    if (loc != NOTHING) {
		repair(obj, "Bad location %d for global environment.", loc);
		db_move(obj, NOTHING);
	    }
	    continue;
	}

	/* Check location chain. */
	for (cur = obj; cur != NOTHING; cur = GetLoc(cur)) {
	    if (cur == global_environment) break;
	    if (cur == HOME) break;

	    SetOnFlag(cur, VISITED);

	    loc = GetLoc(cur);
	    
	    if (loc != NOTHING && HasFlag(loc, VISITED)) {
		repair(cur, "Location chain has a loop.");
		db_move(cur, NOTHING);
	    }
	}

	for (cur = obj; cur != NOTHING; cur = GetLoc(cur)) {
	    if (cur == global_environment) break;
	    if (cur == HOME) break;

	    SetOffFlag(cur, VISITED);
	}

	/* Find locations for detached objects. */
	if (GetLoc(obj) == NOTHING) {
	    switch (Typeof(obj)) {
	    case TYPE_ROOM:
		repair(obj, "Parent of room bad, set to global environment.");
		db_move(obj, global_environment);
		break;
	    case TYPE_PLAYER:
		repair(obj, "Player's location bad, set to player start.");
		db_move(obj, player_start);
		break;
	    case TYPE_THING:
	    case TYPE_PROGRAM:
		repair(obj, "Location bad, set to player start.");
		db_move(obj, player_start);
		break;
	    case TYPE_EXIT:
		if (exit_room == NOTHING) {
		    exit_room = make_object(TYPE_ROOM, "Floating Exit Room",
					    head_player, global_environment,
					    NOTHING);
		    PushContents(global_environment, exit_room);
		}
		repair(obj, "Exit location bad, placing in special room #%d.",
		       exit_room);
		db_move(obj, exit_room);
		break;
	    }
	}


	/* Make sure objects appear in the contents/actions lists. */
	/* Note: using Push* instead of db_move, simply to avoid the
	   attempt to remove the object from the list, when we've already
	   checked that it wasn't there. */
	loc = GetLoc(obj);
    
	if (Typeof(obj) == TYPE_EXIT) {
	    if (!find_in_list(GetActions(loc), obj)) {
		repair(obj, "Adding exit to #%d's action list.", loc);
		PushActions(loc, obj); 
	    }
	} else {
	    if (!find_in_list(GetContents(loc), obj)) {
		repair(obj, "Adding object to #%d's contents list.", loc);
		PushContents(loc, obj);	
	    }
	}
    }
}


int sanity_check(void)
{
    if (Typeof(global_environment) != TYPE_ROOM) {
	fatal(global_environment, "Global environment isn't a room.");
    }

    if (Typeof(player_start) != TYPE_ROOM) {
	fatal(player_start, "Player start isn't a room.");
    }

    if (Typeof(head_player) != TYPE_PLAYER) {
	fatal(head_player, "Head player isn't a player.");
    }

    if (fatal_sanity) return 0;

    check_fields();
    if (fatal_sanity) return 0;

    check_cycles();
    if (fatal_sanity) return 0;

    check_locations();
    if (fatal_sanity) return 0;

    return 1;
}


static void warn(const dbref obj, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    log_sanity(0, obj, fmt, ap);
    va_end(ap);
}

static void fatal(const dbref obj, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    log_sanity(1, obj, fmt, ap);
    fatal_sanity++;
    va_end(ap);
}

static void repair(const dbref obj, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    if (do_repair) {
	log_sanity(2, obj, fmt, ap);
    } else {
	log_sanity(1, obj, fmt, ap);
	fatal_sanity++;
    }
    va_end(ap);
}
