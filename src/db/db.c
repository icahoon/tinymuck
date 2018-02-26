/* Copyright (c) 1992 by David Moore and Ben Jackson.  All rights reserved. */
/* db.c,v 2.20 1997/08/16 22:30:36 dmoore Exp */
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "db.h"
#include "params.h"
#include "buffer.h"
#include "externs.h"

static void db_free_object(const dbref i);

union free_object {
    struct general_object obj;
    union free_object *next;
};

struct general_object **db = NULL;
dbref db_top = 0;
int object_counts[TYPE_MASK];	/* Number of each type of object in db. */

dbref player_start;
dbref global_environment;
dbref head_player;

#ifdef DELAYED_RECYCLE
int db_check_cnt = 0;
#endif

static dbref recyclable = NOTHING;
static union free_object *available_objects = NULL;

static object_flag_type default_flags[TYPE_MASK] = {
	DEFAULT_ROOM_FLAGS,
	DEFAULT_THING_FLAGS,
	DEFAULT_EXIT_FLAGS,
	DEFAULT_PLAYER_FLAGS,
	DEFAULT_PROGRAM_FLAGS,
	NOFLAGS
};

static void generate_objects(long count)
{
    union free_object *hunk;
    int num_4k;
    int i, j;

    /* Try allocating them in 4K sets.  The -4 is for some mallocs
       which want to stick some overhead in the space.  This keeps
       things on a single page (hopefully). */
    num_4k = (4096 - 4) / sizeof(union free_object);

    count = (count / num_4k) + 1;

    for (i = 0; i < count; i++) {
	MALLOC(hunk, union free_object, num_4k);
	for (j = 0; j < num_4k; j++) {
	    hunk->next = available_objects;
	    available_objects = hunk;
	    hunk++;
	}
    }
}


static struct general_object *make_new_object(void)
{
    union free_object *result;

    /* Create the node. */
    if (!available_objects) {
	generate_objects(1);
    }
    result = available_objects;
    available_objects = result->next;

    return (struct general_object *) result;
}


static void free_object(struct general_object *object)
{
    union free_object *which;

    which = (union free_object *) object;
    which->next = available_objects;
    available_objects = which;
}


void db_grow(const dbref newtop)
{
    static dbref db_size = 0;
    dbref old_size;
    dbref i;

    db_top = newtop;

    if (newtop <= db_size) return;

    old_size = db_size;

    if (!db) {
	/* make the initial one */
	db_size = newtop;
	MALLOC(db, struct general_object *, db_size);
    } else {
	/* Grow by 1000 objects on reach resize. */
	db_size = newtop + 1000;
	REALLOC(db, struct general_object *, db_size);
    }

    for (i = old_size; i < db_size; i++)
	db[i] = NULL;
}


void init_db(int old_dbtop, int old_growth)
{
    /* Start the new database w/ hopefully sufficient room to grow in.
       This will save you from reallocing the huge main db array.  If you
       regrow the array, you will most likely leave a big hole, and waste
       a lot more memory than overestimating in the first place. */
    if (old_dbtop < 1000)
	old_dbtop = 1000;
    if (old_growth < 10)
	old_growth = 10;

    db_grow(old_dbtop + (6 * old_growth));
    db_top = 0;
}


static dbref new_object(void)
{
    dbref newobj;
    
    if (recyclable != NOTHING) {
	newobj = recyclable;
	recyclable = GetNext(newobj);
	db_free_object(newobj);
    } else {
	newobj = db_top;
	db_grow(db_top + 1);
    }

    return newobj;
}


static void make_garbage_object(const dbref object)
{
    db[object] = make_new_object();

    SetFlags(object, TYPE_GARBAGE);
    SetNext(object, recyclable);
    recyclable = object;

    object_counts[TYPE_GARBAGE]++;
}

static void make_general_object(const dbref object)
{
    db[object] = make_new_object();
}


void make_object_internal(const dbref newobj, const object_flag_type type)
{
    struct general_object *object;

    /* The position in the array is available but NULL at this point. */
    switch (type & TYPE_MASK) {
    case TYPE_ROOM:
    case TYPE_EXIT:
    case TYPE_THING:
    case TYPE_PLAYER:
    case TYPE_PROGRAM:
	make_general_object(newobj);
	break;
    case TYPE_GARBAGE:
	make_garbage_object(newobj);
	return;
	break;
    }

    object = db[newobj];

    object->flagsx = type;
    object->namex = NULL;
    object->nextx = NOTHING;
    object->locationx = NOTHING;
    object->ownerx = NOTHING;
    object->ownlistx = NOTHING;
    object->contentsx = NOTHING;
    object->actionsx = NOTHING;
    object->ndestx = 0;
    object->destx = NULL;
    clear_prop_list(&(object->propertiesx));

    object_counts[type & TYPE_MASK]++;
}


/* Make a new object with the specified type, name, owner, location, and link.
   As a special case, if the owner is NOTHING, then have it own itself. */
dbref make_object(const object_flag_type type, const char *name, const dbref owner, const dbref location, const dbref link)
{
    dbref newobj;

    newobj = new_object();
    make_object_internal(newobj, type);

    if (owner == NOTHING) {
	SetOwner(newobj, newobj); /* Special own yourself case. */
    } else {
	chown_object(newobj, owner);
    }

    SetFlags(newobj, default_flags[type] | type);
    SetName(newobj, name);
    SetLoc(newobj, location);
    SetLink(newobj, link);

    /* In case this was a program left over from an old crash or such. */
    remove(make_progname(newobj));

    return newobj;
}


static void db_free_object(const dbref i)
{
    object_counts[Typeof(i)]--;

    if (!Garbage(i)) {
	FreeName(i);
	FreeLink(i);
	FreeProps(i);
	if (Typeof(i) == TYPE_PLAYER) delete_frame(i);
	if (Typeof(i) == TYPE_PROGRAM) delete_code(i);
    }
    free_object(db[i]);
}


void db_FREE(void)
{
    dbref i;
    int type;
    
    if (db) {
	for (i = 0; i < db_top; i++) {
	    db_free_object(i);
	}
	FREE(db);
	db = 0;
	db_top = 0;
    }
    recyclable = NOTHING;

    for (type = 0; type < TYPE_MASK; type++)
	object_counts[type] = 0;
}



static void mark(const dbref);
static void sweep(void);
static void nuke(const dbref);
 
void recycle(const dbref player, const dbref obj)
{
    mark(obj);
    sweep();
    nuke(obj);

    notify(player, "Thank you for recycling.");
}

 
void purge(const dbref player, const dbref victim)
{
    dbref obj;
 
    for (obj = GetOwnList(victim); obj != NOTHING; obj = GetOwnList(obj)) {
	mark(obj);
    }

    sweep();

    obj = GetOwnList(victim);
    for (obj = 0; obj < db_top; obj++) {
	if (HasFlag(obj, BEING_RECYCLED)) nuke(obj);
    }

    notify(victim, "All of your objects have been recycled.");
    if (player != victim)
        notify(player, "All of %n's objects have been recycled.", victim);
}
 

/* marks an object to be recycled */
static void mark(const dbref thing)
{
    int payback = 0;
    dbref act, next;
 
    if (HasFlag(thing, BEING_RECYCLED)) return; /* mark only once */
 
    if ((thing == player_start)
	|| (thing == global_environment)
	|| (thing == head_player))
        return;  /* don't nuke these! */

    switch (Typeof(thing)) {
        case TYPE_ROOM:
            payback = ROOM_COST;
            /* fall through */
 
        case TYPE_THING:
            if (!payback) payback = GetPennies(thing);
 
            for (act = GetActions(thing); act != NOTHING; act = next) {
		next = GetNext(act);
                mark(act);
            }
            break;
 
        case TYPE_EXIT:
            payback = EXIT_COST;
            if (GetNDest(thing)) payback += LINK_COST;
            break;
 
        case TYPE_PROGRAM:
            payback = 0;
            break;
    }
 
    IncPennies(GetOwner(thing), payback);
    SetOnFlag(thing, BEING_RECYCLED);
}
 
#define ObjGoing(x) \
    (x >= (dbref)0 && (Typeof(x) == TYPE_GARBAGE \
		       || HasFlag(x, BEING_RECYCLED)))
 
/* Cleans up references to objects going away. */
static void sweep(void)
{
    dbref obj;
    dbref owner;
 
    for (obj = 0; obj < db_top; obj++) {
 
	/* If this object is going away, we'll get rid of it in nuke() */
	if (HasFlag(obj, BEING_RECYCLED)) continue;

	if (Typeof(obj) == TYPE_GARBAGE) continue;
	
	owner = GetOwner(obj);
	if (ObjGoing(owner)) {
	    chown_object(obj, head_player);
	    owner = head_player;
	}
 
	switch (Typeof(obj)) {
	case TYPE_ROOM:
	    if (ObjGoing(GetLink(obj)))
		FreeLink(obj);
	    break;

	case TYPE_THING:
	    if (ObjGoing(GetLink(obj))) {
		if (ObjGoing(GetLink(owner))) {
		    SetLink(obj, player_start);
		} else {
		    SetLink(obj, GetLink(owner));
		}
	    }
	    break;

	case TYPE_EXIT:
	    {
		int ndest, i;
		dbref *a, *b;

		ndest = GetNDest(obj);

		if (ndest > MAX_LINKS) {
		    log_status("AIEEEE: %u has %d destinations",
                                owner, obj, ndest);
		    ndest = 0;
		    SetDest(obj, NULL);
		}
 
		/* Shift down all remaining items in exit list. */
		a = b = GetDest(obj);
		for (i = 0; ndest--; a++) {
		    if (!ObjGoing(*a)) {
			*b++ = *a;
			i++;
		    }
		}
 
		if ((i == 0) && (GetNDest(obj) > 0)) {
		    IncPennies(owner, LINK_COST);
		    SetDest(obj, NULL);
		}
		SetNDest(obj, i);
	    }
	    break;
 
	case TYPE_PLAYER:
	    if (ObjGoing(GetLink(obj)))
		SetLink(obj, player_start);
	    break;

	case TYPE_PROGRAM:
	    if (ObjGoing(GetLink(obj))) {
		if (ObjGoing(GetLink(owner))) {
		    SetLink(obj, player_start);
		} else {
		    SetLink(obj, GetLink(owner));
		}
	    }
	    break;

	}
    }
}


/* Actually remove this object. */
static void nuke(const dbref obj)
{
    dbref owner;
    dbref loc;
    dbref home;
    dbref cont;
    dbref next;

    if (!HasFlag(obj, BEING_RECYCLED)) return;	/* Already got it. */
    if (Garbage(obj)) return;

    owner = GetOwner(obj);
    loc = GetLoc(obj);

    if (Typeof(obj) == TYPE_ROOM) {
	notify_except(obj, NOTHING, "You feel a wrenching sensation...");
    }

    if (Typeof(obj) == TYPE_PROGRAM) {
	edit_quit_recycle(owner);
	remove(make_progname(obj));
    }

    /* send the contents on their merry ways */
    for (cont = GetContents(obj); cont != NOTHING; cont = next) {
	home = GetHome(cont);
	next = GetNext(cont);

	if (Typeof(cont) == TYPE_ROOM)
	    home = global_environment;
 
	/* don't put it in garbage or in a marked room */
	if (ObjGoing(home)) {
	    /* try to put it in the owner's home */
	    home = GetLink(GetOwner(cont));
	    if (ObjGoing(home))
		home = player_start;
	}
	move_object(cont, home, obj);
    }

    /* Remove all actions on this object, they're already marked too. */
    for (cont = GetActions(obj); cont != NOTHING; cont = next) {
	next = GetNext(cont);
	db_move(cont, NOTHING);
	nuke(cont);
    }

    db_move(obj, NOTHING);
    chown_object(obj, NOTHING);
    db_free_object(obj);
    make_garbage_object(obj);
 
}


void chown_object(const dbref thing, const dbref newowner)
{
    dbref oldowner;
    dbref temp;
    dbref next;

    /* Take the thing out of the old owner's list. */
    oldowner = GetOwner(thing);
    if ((oldowner != thing) && (oldowner != NOTHING)) {
	for (temp = oldowner; temp != NOTHING; temp = GetOwnList(temp)) {
	    if (GetOwnList(temp) == thing) {
		SetOwnList(temp, GetOwnList(thing));
		break;
	    }
	}
    }

    SetOwner(thing, newowner);

    /* Stick this object in the new owner list, in sorted order. */
    /* Link this object into this players ownership list. */
    /* Maybe this should actually be doing a sorted insert? */
    if ((newowner != thing) && (newowner != NOTHING)) {
	for (temp = newowner; ; temp = next) {
	    next = GetOwnList(temp);
	    if ((next == NOTHING) || (next > thing)) {
		/* Either hit end of list, or the right position. */
		SetOwnList(thing, next);
		SetOwnList(temp, thing);
		break;
	    }
	}
    }
}


void db_move(const dbref what, dbref where)
{
    dbref loc;
    
    /* remove what from old loc */
    loc = GetLoc(what);
    if (loc != NOTHING) {
	if (Typeof(what) != TYPE_EXIT) {
	    SetContents(loc, remove_first(GetContents(loc), what));
	} else {
	    SetActions(loc, remove_first(GetActions(loc), what));
	}
    }
    
    if (Garbage(what)) return;	/* Garbage doesn't go anywhere. */

    if (where == HOME)
	where = GetHome(what);

    /* now put what in where */
    SetLoc(what, where);

    if (where != NOTHING) {
	/* NOTHING doesn't like having contents. */
	if (Typeof(what) != TYPE_EXIT) {
	    PushContents(where, what);
	} else {
	    PushActions(where, what);
	}
    }
}



