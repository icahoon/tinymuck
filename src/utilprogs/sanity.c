/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* sanity.c,v 2.18 1997/02/28 19:40:43 dmoore Exp */

/* Warn if some object has more than this many properties. */
#define PROP_WARN 200
/* Warn if some string (prop or field) exceeds this length. */
#define STRLEN_WARN 1024 
/* Warn if someone has more objects than this. */
#define OBJECT_CNT_WARN 200

/* Ordered steps to database repair:
   1. Break all contents/action list cycles (_o) and doubly entered lists (Y).
   2. Check all location fields for validity.  If a location is invalid or
      is part of a loop, set to NOTHING.  For this pass, accept HOME as a
      valid location.
   3. Check owners.  (What is the right thing to do w/ bad owner fields?)
   4. Check home fields of objects and players.
   5. Check all attached objects (location != NOTHING) for proper location
      and move if needed.  If an object is in the contents list of one
      object and it's location is another object then the location
      field wins.  (Map HOME to actual home before comparing locations.)
   6. Move all objects remaining with location == NOTHING to an appropriate
      place.  (They are totally detached.)
   7. Do all other various checks: destinations of exits, room droptos,
      properties, names, flags, etc.
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

#undef Typeof
#define Typeof(x) \
    ((x) == HOME ? TYPE_ROOM : ((x) < 0 || (x) >= db_top) ? TYPE_GARBAGE : (GetFlags(x) & TYPE_MASK))

/* Special version of DOLIST for the sanity checker. */
#undef DOLIST
#define DOLIST(v, first) \
    for ((v) = (first); (v) != NOTHING && ok_dbref(v); (v) = GetNext(v))


/* Loop over the actions/contents of an object.  Keep track of
   previous object in the list, so we can cut the list before the
   current object.  In case the current object is the head of the
   list, we set loop_type to let us know how to reset the real
   object.  If loop_type is 0 we use SetNext.  If it's 1 we
   use SetContents, and if it's 2 we use SetActions.
*/
static dbref loop_prev;
static dbref loop_curr;
static int loop_type;

#define LOOP_NEXT		0
#define LOOP_CONTENTS		1
#define LOOP_ACTIONS		2
#define LOOP_OWNLIST		3
#define LOOP_OWNLIST_NEXT	4
#define LOOP_LOC		5

#define Get_List(obj, type) \
    ((type) == LOOP_NEXT ? GetNext(obj) : \
     ((type) == LOOP_CONTENTS ? GetContents(obj) : \
      ((type) == LOOP_ACTIONS ? GetActions(obj) : \
       ((type) == LOOP_OWNLIST ? (obj) : \
	((type) == LOOP_OWNLIST_NEXT ? GetOwnList(obj) : \
	 ((type) == LOOP_LOC ? GetLoc(obj) : \
	  NOTHING))))))

#define Set_List(obj, val, type) \
    do { \
	if (type == LOOP_NEXT) SetNext(obj, val); \
        else if (type == LOOP_CONTENTS) SetContents(obj, val); \
	else if (type == LOOP_ACTIONS) SetActions(obj, val); \
	else if (type == LOOP_OWNLIST); \
	else if (type == LOOP_OWNLIST_NEXT) SetOwnList(obj, val); \
	else if (type == LOOP_LOC) SetLoc(obj, val); \
    } while (0)

#define Type_List(type) \
    ((type) == LOOP_NEXT ? "next" : \
     ((type) == LOOP_CONTENTS ? "contents" : \
      ((type) == LOOP_ACTIONS ? "actions" : \
       ((type) == LOOP_OWNLIST ? "owner list" : \
	((type) == LOOP_OWNLIST ? "owner list" : \
         ((type) == LOOP_LOC ? "location" : \
	  ""))))))

#define Next_List(type) \
    ((type) == LOOP_NEXT ? LOOP_NEXT : \
     ((type) == LOOP_CONTENTS ? LOOP_NEXT : \
      ((type) == LOOP_ACTIONS ? LOOP_NEXT : \
       ((type) == LOOP_OWNLIST ? LOOP_OWNLIST_NEXT : \
	((type) == LOOP_OWNLIST ? LOOP_OWNLIST_NEXT : \
	 ((type) == LOOP_LOC ? LOOP_LOC : \
	  NOTHING))))))


#define DO_List(v, obj, type) \
    for ((v) = Get_List(obj, type); \
         (v) != NOTHING; \
	 (v) = Get_List((v), Next_List(type)))

/* Note does not verify the items as ok_dbref. */
/* Important: it is not reentrant. */
#define LOOP_List(v, obj, type) \
    for ((loop_prev = (obj), loop_type = (type), \
	  loop_curr = (v) = Get_List(obj, loop_type)); \
         (v) != NOTHING; \
         (loop_prev = loop_curr, loop_type = Next_List(type), \
	  loop_curr = (v) = Get_List(loop_curr, loop_type)))

#define LOOP_CUT_AFTER() \
    Set_List(loop_curr, NOTHING, loop_type)

#define LOOP_CUT_BEFORE() \
    Set_List(loop_prev, NOTHING, loop_type)


#define LOOP_REMOVE() \
    do { \
        Set_List(loop_prev, NOTHING, loop_type); \
	Set_List(loop_curr, NOTHING, Next_List(loop_type)); \
	loop_curr = loop_prev; \
    } while (0)


#ifndef VISITED			   /* Now in flags.h prepare for sane.c */
#define VISITED		IN_PROGRAM /* A flag we know isn't set on anything. */
#endif
#define CYCLE_CHK	IN_EDITOR  /* another free flag. */


/* Set to true when a fatal error is found. */
static int fatal_sanity = 0;

/* Set true if repairs should be attempted. */
static int do_repair = 0;

/* Set to true if warnings should be made for flag security issues.
   Such as wizzed/dark players, etc. */
static int check_security = 0;


static void warn(const dbref obj, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "Warning object #%ld: ", obj);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}


static void fatal(const dbref obj, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "Fatal object #%ld: ", obj);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    fatal_sanity = 1;
    va_end(ap);
}


static void repair(const dbref obj, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "Repairing object #%ld: ", obj);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}


/* This routine verifies that the object it is passed is ''ok''.
   That means, it must be within the legal range of dbrefs, be of
   normal object type, or be the special HOME or NOTHING object. */
static int ok_dbref(const dbref obj)
{
    if (obj >= db_top) return 0;
    
    /* obj < db_top */
    if (obj >= 0) {
	return ((Typeof(obj) == TYPE_ROOM)
		|| (Typeof(obj) == TYPE_PLAYER)
		|| (Typeof(obj) == TYPE_THING)
		|| (Typeof(obj) == TYPE_EXIT)
		|| (Typeof(obj) == TYPE_PROGRAM));
    }
    
    /* obj < 0 */
    return ((obj == HOME) || (obj == NOTHING));
}


static int find_in_list(const dbref obj, const dbref first)
{
    int   found;
    dbref i;
    
    found = 0;
    DOLIST(i, first) {
	if (i == obj) {
	    found = 1;
	    break;
	}
     }
    return found;
}


static int bad_type(const dbref obj, const dbref parent, const int type)
{
    object_flag_type obj_type;

    obj_type = Typeof(obj);

    if (type == LOOP_CONTENTS) {
        switch (Typeof(parent)) {
	case TYPE_ROOM:
	    return (obj == global_environment);
	    break;
	case TYPE_PLAYER:
            /* Player's can't carry rooms and other players. */
            return (obj_type == TYPE_ROOM) || (obj_type == TYPE_PLAYER);
	    break;
	case TYPE_THING:
	case TYPE_PROGRAM:
	case TYPE_EXIT:
            return 1;		/* Can't carry anything. */
	    break;
	}
    } else if (type == LOOP_ACTIONS) {
        switch (Typeof(parent)) {
	case TYPE_ROOM:
	case TYPE_PLAYER:
	case TYPE_THING:
	    /* As long as it's an exit, it's ok. */
            return (obj_type != TYPE_EXIT);
	    break;
	case TYPE_PROGRAM:
	case TYPE_EXIT:
            return 1;		/* No actions on these. */
	    break;
	}
    }

    return -1;
}


static void check_location(const dbref obj)
{
    dbref loc;
    
    if (obj == 0) return;  /* #0 has no location. */

    loc = GetLoc(obj);

    if (loc == HOME) {
	if (do_repair) {
	    if (Typeof(obj) == TYPE_EXIT)
		SetLoc(obj, NOTHING);
	    else
		SetLoc(obj, GetHome(obj));
	    repair(obj, "Location was HOME, changed to actual home.");
	} else {
	    fatal(obj, "Location field is HOME (#%ld).", HOME);
	    return;
	}
    }
    
    loc = GetLoc(obj);

    if (!ok_dbref(loc)) {
	if (do_repair) {
	    SetLoc(obj, NOTHING);
	    repair(obj, "Bad location, set to NOTHING.");
	} else {
	    fatal(obj, "Location field is bad, #%ld.", loc);
	    return;
	}
    }

    loc = GetLoc(obj);

    switch (Typeof(obj)) {
    case TYPE_ROOM:
	if (loc == NOTHING || Typeof(loc) != TYPE_ROOM) {
	    if (do_repair) {
		SetLoc(obj, global_environment);
		repair(obj, "Parent of room bad, set to global environment.");
	    } else {
		fatal(obj, "Parent of this room isn't a room, #%ld.", loc);
	    }
	}
	break;
    case TYPE_PLAYER:
	if (loc == NOTHING || Typeof(loc) != TYPE_ROOM) {
	    if (do_repair) {
		SetLoc(obj, player_start);
		repair(obj, "Player's location bad, set to player start.");
	    } else {
		fatal(obj, "Player's location isn't a room, #%ld.", loc);
	    }
	}
	break;
    case TYPE_THING:
    case TYPE_PROGRAM:
	if (loc == NOTHING
	    || ((Typeof(loc) != TYPE_ROOM)
		&& (Typeof(loc) != TYPE_PLAYER))) {
	    if (do_repair) {
		SetLoc(obj, player_start);
		repair(obj, "Location bad, set to player start.");
	    } else {
		fatal(obj, "Location, #%ld, not a player or room.", loc);
	    }
	}
	break;
    case TYPE_EXIT:
	if (((Typeof(loc) != TYPE_ROOM) && (Typeof(loc) != TYPE_PLAYER)
		&& (Typeof(loc) != TYPE_THING))) {
	    if (do_repair) {
		db_move(obj, NOTHING);
		repair(obj, "Exit location is bad.  Detaching exit.");
	    } else {
		fatal(obj, "Location, #%ld, not player, room, or thing.", loc);
	    }
	}
	break;
    }

    loc = GetLoc(obj);
    
    if (Typeof(obj) == TYPE_EXIT) {
	if (loc != NOTHING && !find_in_list(obj, GetActions(loc))) {
	    if (do_repair) {
		PushActions(loc, obj);
		repair(obj, "Adding exit to #%ld's action list.", loc);
	    } else {
		fatal(obj, "Exit does not appear in #%ld's action list.", loc);
	    }
	}
    } else {
	if (loc != NOTHING && !find_in_list(obj, GetContents(loc))) {
	    if (do_repair) {
		PushContents(loc, obj);
		repair(obj, "Adding object to #%ld's contents list.", loc);
	    } else {
		fatal(obj, "Does not appear in #%ld's contents list.", loc);
	    }
	}
    }
}


static long total_strings = 0;
static long total_bytes = 0;

static int check_string(const dbref obj, const char *str, const char *msg)
{
    int len;
    int illegal;

    if (!str)
	return 0;

    len = strlen(str);

    total_strings++;
    total_bytes += len;

    if (len >= STRLEN_WARN)
	warn(obj, "%s too long.  %d bytes.", msg, len);

    illegal = 0;
    for (; *str; str++) {
	if (!(isascii(*str) && isprint(*str))) illegal++;
    }

    if (illegal)
	warn(obj, "%s has %d illegal characters present.", msg, illegal);

    return len;
}


static int most_props = 0;
static int curr_props = 0;
static int curr_bytes = 0;

static void check_str_prop(const dbref obj, const char *name, const char *string)
{
    Buffer buf;

    curr_props++;
    
    Bufsprint(&buf, "Property(name): %s", name);
    curr_bytes += check_string(obj, name, Buftext(&buf));

    Bufsprint(&buf, "Property(text): %s", name);
    curr_bytes += check_string(obj, string, Buftext(&buf));
}


static void check_properties(const dbref obj)
{

    curr_props = 0;
    curr_bytes = 0;
    map_string_props(obj, check_str_prop);
    
    if (curr_props >= PROP_WARN)
	warn(obj, "Large number of properties.  %d props, %d bytes.",
	     curr_props, curr_bytes);
    
    if (curr_props > most_props)
	most_props = curr_props;
}


static int name_bytes = 0;

static void check_name(const dbref obj)
{
    int tmp;

    tmp = check_string(obj, GetName(obj), "Name");
    if (Typeof(obj) == TYPE_PLAYER) {
	if (tmp > PLAYER_NAME_LIMIT)
	    fatal(obj, "Player name too long.  %d bytes.", tmp);
    }
    if (tmp == 0)
	fatal(obj, "Object has empty name.");
	
    name_bytes += tmp;
}


static void check_owner(const dbref obj)
{
    dbref owner;

    owner = GetOwner(obj);

    if ((!ok_dbref(owner)) || (Typeof(owner) != TYPE_PLAYER)) {
	fatal(obj, "Owner is illegal, #%ld.", owner);
	return;
    }

    if ((Typeof(obj) == TYPE_PLAYER) && (obj != owner))
	fatal(obj, "Player isn't owned by self, #%ld.", owner);
}


static void check_ownership(const dbref obj)
{
    int cnt;
    dbref temp;

    cnt = 0;

    for (temp = obj; temp != NOTHING; temp = GetOwnList(temp)) {
	cnt++;
    }

    if (cnt > OBJECT_CNT_WARN)
	warn(obj, "Owns %d objects.", cnt);
}


static void check_home(const dbref obj)
{
    dbref home;
    int bad = 0;

    if (GetNDest(obj) > 1)
	fatal(obj, "Home is meta-link.");

    home = GetLink(obj);

    bad = !ok_dbref(home);
    if (!bad) {
	if (Typeof(obj) == TYPE_PLAYER) {
	    bad = (Typeof(home) != TYPE_ROOM);
	} else {
	    bad = ((Typeof(home) != TYPE_ROOM)
                   && (Typeof(home) != TYPE_PLAYER));
	}
    }

    if (bad) {
	if (!do_repair) {
	    fatal(obj, "Home is illegal, #%ld.", home);
	} else {
            repair(obj, "Home is illegal, linking #%ld to PLAYER_START.", obj);
            SetLink(obj, player_start);
        }
    }
}


static void check_dropto(const dbref obj)
{
    dbref  dropto;

    return;

    if (GetNDest(obj) > 1)
	fatal(obj, "Dropto is meta-link.");

    dropto = GetLink(obj);

    if ((ok_dbref(dropto) == 0)  /* FIX FIX FIX buss error on this line? bj */
	|| (Typeof(dropto) != TYPE_ROOM
	    && dropto != NOTHING
	    && dropto != HOME))
	fatal(obj, "Dropto is illegal, #%ld.", dropto);
}


static void check_exits(const dbref obj)
{
    int   i, ndest;
    dbref *exits;
    
    ndest = GetNDest(obj);
    exits = GetDest(obj);

    if ((ndest < 0) || (ndest > MAX_LINKS)) {
	fatal(obj, "Illegal number of exits, %d.", ndest);
    } else if (ndest && !exits) {
	fatal(obj, "Exit destinations list is null.");
    } else {
	for (i = 0; i < ndest; i++, exits++) {
	    if (!ok_dbref(*exits))
		fatal(obj, "Illegal object as exit destination, #%ld.",
		      *exits);
	    /* Might want to put in checks for cycles or something. */
	}
    }
}


static void check_flags(const dbref obj)
{
    if (!check_security) return;

    switch (Typeof(obj)) {
    case TYPE_ROOM:
	break;
    case TYPE_THING:
	if (HasFlag(obj, DARK))
	    warn(obj, "Object is dark.");
	break;
    case TYPE_EXIT:
	if (HasFlag(obj, DARK))
	    warn(obj, "Exit is dark.");
	break;
    case TYPE_PLAYER:
	if (HasFlag(obj, DARK))
	    warn(obj, "Player is dark.");
	if (HasFlag(obj, WIZARD))
	    warn(obj, "Player is wizard.");
	break;
    case TYPE_PROGRAM:
	if (HasFlag(obj, WIZARD))
	    warn(obj, "Program is wizzed.");
	break;
    }
}


static void check_special_dbrefs(const dbref obj)
{
    dbref owner;

    if (!check_security) return;

    if ((obj == head_player) && !HasFlag(obj, WIZARD)) {
	warn(obj, "Head player is not set wizard.");
    }

    if (obj == global_environment) {
	owner = GetOwner(global_environment);
        if (!TrueWizard(owner))
	    warn(obj, "Global environment owned by non-wizard #%ld.", owner);
    }

    if (obj == player_start) {
	owner = GetOwner(player_start);
        if (!TrueWizard(owner))
	    warn(obj, "Player start owned by non-wizard #%ld.", owner);
    }
}


static int check_list_cycle(dbref parent, int type)
{
    dbref obj;

    LOOP_List(obj, parent, type) {
        if (!ok_dbref(obj)) {
	    if (do_repair) {
		/* FIX FIX FIX: does garbage have a valid next field?
		   if not, should use LOOP_CUT_BEFORE and break out of
		   loop. */
	        LOOP_CUT_BEFORE();
		repair(parent, "Garbage in %s list.", Type_List(type));
		break;
	    } else {
		fatal(parent, "Garbage detected in %s list.", Type_List(type));
		return 0;
	    }
	}

	if (bad_type(obj, parent, type)) {
	    if (do_repair) {
	        LOOP_REMOVE();
		repair(parent, "Bad object type in %s list.", Type_List(type));
	    } else {
		fatal(parent, "Bad object type in %s list.", Type_List(type));
		return 0;
	    }
	}

	if (HasFlag(obj, CYCLE_CHK)) {
	    /* This object is either the head of a cycle, or the merge
	       point of two seperate lists. __o  or Y.  */
	    if (do_repair) {
	        LOOP_CUT_BEFORE();
		repair(parent, "Cycle or double entrance in %s list.",
		      Type_List(type));
		break;		/* we've already seen the rest */
	    } else {
	        fatal(parent, "Cycle or double entrance detected in %s list.", 
		      Type_List(type));
		return 0;	/* Bad problem. */
	    }
	}
	SetOnFlag(obj, CYCLE_CHK);

	if (GetLoc(obj) != parent) {
	    if (do_repair) {
	        SetLoc(obj, parent);
		repair(obj, "Setting location to be in #%ld.", parent);
	    } else {
	        fatal(obj, "In %s list of #%ld, but location is #%ld.",
		      Type_List(type), parent, GetLoc(obj));
	    }
	}
    }

    return 1;			/* All ok. */
}


/* Here we check all contents/actions lists for cycles and merges.  We
   also clean up any garbage items in those lists.  If an object in these
   lists has it's home set to somewhere else, we fix it. */
static int check_cycles(void)
{
    dbref i;

    for (i = 0; i < db_top; i++) {
	if (!ok_dbref(i)) continue; /* Skip garbage. */

	if (!check_list_cycle(i, LOOP_CONTENTS))
	    return 0;

	if (!check_list_cycle(i, LOOP_ACTIONS))
	    return 0;
    }
    
    return 1;			/* All was ok. */
}


static void check_locations(void)
{
    dbref i;

    for (i = 0; i < db_top; i++) {
	if (!ok_dbref(i)) continue; /* Skip garbage. */

	check_location(i);
    }
}


static void recursive_descent(dbref obj, int type)
{
    DO_List(obj, obj, type) {

        if ((!ok_dbref(obj)) || obj == HOME) {
	    fatal(obj, "bad object found in recursive descent");
	    return;
        }

	/* mark this node as visited. */
	if (HasFlag(obj, VISITED)) {
	    fatal(obj, "Object was visited twice in recursive descent.");
	    /* This means it's in two contents or action lists. */
	    /* This should not happen if you have repair turned on,
	       because check_cycles should fix it. */
	    return;
	}
	SetOnFlag(obj, VISITED);

	/* Recurse. */
	switch (Typeof(obj)) {
	case TYPE_ROOM:
	case TYPE_PLAYER:
	    recursive_descent(obj, LOOP_CONTENTS);
	    recursive_descent(obj, LOOP_ACTIONS);
	    break;
	case TYPE_THING:
	    recursive_descent(obj, LOOP_ACTIONS);
	    break;
	}
    } /* DO_List */
}


static void check_db_graph()
{
    dbref i;

    SetOnFlag(global_environment, VISITED);
    recursive_descent(global_environment, LOOP_CONTENTS);
    recursive_descent(global_environment, LOOP_ACTIONS);

    for (i = 0; i < db_top; i++) {
	if (ok_dbref(i) && !HasFlag(i, VISITED)) {
#if 0
	    /* db_free_object(i);  -- mem leak, whee. */
	    make_object_internal(i, TYPE_GARBAGE);
	    repair(i, "nuking detached object");
#endif
	    fatal(i, "object is detached from the database");
	}
    }
}


int main(const int argc, const char * const *argv)
{
    dbref i;
    int player_cnt  = 0;
    int thing_cnt   = 0;
    int exit_cnt    = 0;
    int room_cnt    = 0;
    int muf_cnt     = 0;
    int garbage_cnt = 0;
    int non_garbage = 0;
    int j;
    char *outputfile = NULL;
    char usage[] = "[-repair] [-norepair] [-o<repaired.db>] [-security]";

    if (!strcmp(argv[0], "repair")) {
	do_repair = 1;
    } else if (!strcmp(argv[0], "sanity")) {
	do_repair = 0;
    }

    for (j=1; j<argc; j++) {
	if (argv[j][0] == '-') {
	    switch(argv[j][1]) {
	    case 'r':
		do_repair = 1;
		break;
	    case 'n':
		do_repair = 0;
		break;
	    case 'o':
		MALLOC(outputfile, char, strlen(&(argv[j][2]) + 1));
		strcpy(outputfile, &(argv[j][2]));
		break;
	    case 's':
		check_security = 1;
		break;
	    default:
		fprintf(stderr, "Usage:  %s %s\n", argv[0], usage);
		return 1;
	    }
	}
    }
    
    printf("Reading database (this may take a while): ");
    fflush(stdout);

    db_read(stdin);
    
    printf("dbtop = %ld\n", db_top);
    fflush(stdout);

    if (Typeof(global_environment) != TYPE_ROOM) {
	fatal(global_environment, "Global environement isn't a room.");
	exit(1);
    }

    if (Typeof(player_start) != TYPE_ROOM) {
	fatal(player_start, "Player start isn't a room.");
	exit(1);
    }

    if (Typeof(head_player) != TYPE_PLAYER) {
	fatal(head_player, "Head player isn't a player.");
	exit(1);
    }

    if (!check_cycles()) {
	fatal(0, "Cannot continue sanity check.");
	exit(1);
    }

    /* At this point, if repair is on:
       1. all contents and action lists are free of bad dbrefs.
       2. all contents and action lists are free of cycles/merges (_o Y)
       3. all non-detached objects have have correct locations.
     */

    check_locations();

    /* Now all locations are valid. */

    check_db_graph();		/* unneeded?  db all attached already? */

    for (i = 0; i < db_top; i++) {
	if (ok_dbref(i)) {
	    /* Basically not garbage or daemon. */
	    check_properties(i);
	    check_name(i);
	    check_owner(i);
	    check_exits(i);
	    check_flags(i);
	    check_special_dbrefs(i);
	}
	switch (Typeof(i)) {
	case TYPE_ROOM:
	    room_cnt++;
	    check_dropto(i);
	    break;
	case TYPE_THING:
	    thing_cnt++;
	    check_home(i);
	    break;
	case TYPE_EXIT:
	    exit_cnt++;
	    break;
	case TYPE_PLAYER:
	    player_cnt++;
	    check_home(i);
	    check_ownership(i);
	    break;
	case TYPE_PROGRAM:
	    muf_cnt++;
	    break;
	case TYPE_GARBAGE:
	    garbage_cnt++;
	    break;
	default:
	    fatal(i, "Unknown object type, %d.", Typeof(i));
	    break;
	}
    }				/* For loop */

    non_garbage = room_cnt + exit_cnt + thing_cnt + muf_cnt + player_cnt;

    printf("\nDatabase statistics:\n");
    printf("  Global environment = #%ld.  Player start = #%ld.  Head player = #%ld.\n",
	   global_environment, player_start, head_player);
    printf("  %d rooms, %d exits, %d things, %d programs, %d players, %d garbage.\n",
	   room_cnt, exit_cnt, thing_cnt, muf_cnt, player_cnt, garbage_cnt);
    printf("  Total dbrefs: %d (actual), %ld (expected)\n",
	   non_garbage + garbage_cnt, db_top);

    printf("\nString statistics:\n");
    printf("  Bytes for all strings = %ld.\n", total_bytes);
    if (non_garbage != 0) {
	printf("  Bytes for names = %d.  Avg name bytes per object = %2.2f.\n\n",
	       name_bytes, (double) name_bytes / (double) non_garbage);
	printf("  Avg string bytes per object = %2.2f.\n\n",
	       (double) total_bytes / (double) non_garbage);
    } else {
	printf("  Bytes for names = %d.\n\n", name_bytes);
    }
    printf("  Total number of strings = %ld.\n", total_strings);
    if (total_strings != 0) {
	printf("  Avg bytes per string = %2.2f.\n",
	       (double) total_bytes / (double) total_strings);
    }

    if (do_repair) {
	if (outputfile == NULL)
	    outputfile = "repaired";
	printf("Saving repaired db into file: %s\n", outputfile);
	fflush(stdout);
        db_write(outputfile);
    }

    exit(fatal_sanity);
}
