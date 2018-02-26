/* db.h,v 2.21 1997/08/29 21:00:17 dmoore Exp */
#ifndef MUCK_DB_H
#define MUCK_DB_H


#include <stdlib.h>		/* size_t?, NULL */
#include "muck_malloc.h"
#include "flags.h"

typedef long dbref;		/* offset into db */

#define INTERNAL_PROP	1
#define NORMAL_PROP	0

#define PERM_PROP	0
#define TEMP_PROP	1


/* The various macros below assume that you are working with a normal
   object, not a garbage object.  They will blow up if you do so.
   So be sure to check things against (Typeof(x) == TYPE_GARBAGE)
   before doing something like SetName(x, "foo");
   The following are legal to do to garbage:
   Typeof, GetNext, SetNext, any Flag macro.
   Any other DB* Set*, Get*, Free* should not be attempted on garbage. */

/* The macros below do not guarantee against multiple evaluation of their
   arguments.  Keep that in mind, if you ever think about something like
   SetName(x++, "foo");  Which I hope you never think about, btw. */

/* The occurances of (0, expr) below are to force the macro to only
   be valid on the right hand side of an expression.  That is it will be
   illegal to say: DBGet(x, y) = 10; */
/* Things in do { xxx; } while (0) are written like that for a similar
   reason, and they also work well in bodies of if's. */


/* DB_ should basically be considered internal, and not used anyplace
   outside of the db code.  It's purpose is to make writing some of
   these macros a bit clearer. */
#define DB_(x)		(db[x])

/* Turn this on to have the program let you know when you try to access a
   field on a normal object, that isn't on a garbage object. */
#ifdef DEBUG_GARBAGE_ACCESS
#undef DB_
#define DB_(x) \
    (!Garbage(x) ? db[x] \
     : (log_status("Illegal field access on garbage(#%d): %s %i", \
		   x, __FILE__, __LINE__),  \
        panic("Garbage access"), db[x]))
#endif

#define DELAYED_RECYCLE
#ifdef DELAYED_RECYCLE
extern int db_check_cnt;
#define DB_CHK(x) (db_check_cnt++)
#else
#define DB_CHK(x) 0
#endif


#ifdef DEBUG_IMPROPER_LHS
static int muck_force_rhs = 0;
#define FORCE_RHS(x) (muck_force_rhs++, (x))
#else
#define FORCE_RHS(x) (x)
#endif


#define GetName(x)	(DB_CHK(x), DB_(x)->namex)
#define SetName(x, y) \
    do { FreeName(x); DB_(x)->namex = ALLOC_STRING(y); } while (0)
#define FreeName(x) \
    do { if (DB_(x)->namex) FREE_STRING(DB_(x)->namex); } while (0)


#define GetPass(x)	get_string_prop(x, "Pass", INTERNAL_PROP)
#define SetPass(x, y)	add_string_prop(x, "Pass", y, INTERNAL_PROP, PERM_PROP)
#define FreePass(x)	remove_prop(x, "Pass", INTERNAL_PROP)

#define GetDesc(x)	get_string_prop(x, "Desc", INTERNAL_PROP)
#define SetDesc(x, y)	add_string_prop(x, "Desc", y, INTERNAL_PROP, PERM_PROP)
#define FreeDesc(x)	remove_prop(x, "Desc", INTERNAL_PROP)

#define GetSucc(x)	get_string_prop(x, "Succ", INTERNAL_PROP)
#define SetSucc(x, y)	add_string_prop(x, "Succ", y, INTERNAL_PROP, PERM_PROP)
#define FreeSucc(x)	remove_prop(x, "Succ", INTERNAL_PROP)

#define GetFail(x)	get_string_prop(x, "Fail", INTERNAL_PROP)
#define SetFail(x, y)	add_string_prop(x, "Fail", y, INTERNAL_PROP, PERM_PROP)
#define FreeFail(x)	remove_prop(x, "Fail", INTERNAL_PROP)

#define GetDrop(x)	get_string_prop(x, "Drop", INTERNAL_PROP)
#define SetDrop(x, y)	add_string_prop(x, "Drop", y, INTERNAL_PROP, PERM_PROP)
#define FreeDrop(x)	remove_prop(x, "Drop", INTERNAL_PROP)

#define GetOSucc(x)	get_string_prop(x, "OSucc", INTERNAL_PROP)
#define SetOSucc(x, y)	add_string_prop(x, "OSucc", y, INTERNAL_PROP, PERM_PROP)
#define FreeOSucc(x)	remove_prop(x, "OSucc", INTERNAL_PROP)

#define GetOFail(x)	get_string_prop(x, "OFail", INTERNAL_PROP)
#define SetOFail(x, y)	add_string_prop(x, "OFail", y, INTERNAL_PROP, PERM_PROP)
#define FreeOFail(x)	remove_prop(x, "OFail", INTERNAL_PROP)

#define GetODrop(x)	get_string_prop(x, "ODrop", INTERNAL_PROP)
#define SetODrop(x, y)	add_string_prop(x, "ODrop", y, INTERNAL_PROP, PERM_PROP)
#define FreeODrop(x)	remove_prop(x, "ODrop", INTERNAL_PROP)

#define GetKey(x)	get_boolexp_prop(x, "Key", INTERNAL_PROP)
#define SetKey(x, y)	add_boolexp_prop(x, "Key", y, INTERNAL_PROP, PERM_PROP)
#define FreeKey(x)	remove_prop(x, "Key", INTERNAL_PROP)

/* Clever trick, offset penny values by 1.  This pays off since nearly every
   object in the db will probably have value 1, saving a prop. */
#define GetPennies(x)		(get_int_prop(x, "Pennies", INTERNAL_PROP) + 1)
#define SetPennies(x, y) \
	add_int_prop(x, "Pennies", (y)-1, INTERNAL_PROP, PERM_PROP)
#define IncPennies(x, y)        SetPennies(x, GetPennies(x) + (y))

/* Only the editor should be messing w/ this.  Also, recycle checks it. */
#define GetEditPrg(x)		get_dbref_prop(x, "EditPrg", INTERNAL_PROP)
#define SetEditPrg(x, y) \
	add_dbref_prop(x, "EditPrg", y, INTERNAL_PROP, PERM_PROP)
#define FreeEditPrg(x)		remove_prop(x, "EditPrg", INTERNAL_PROP)

/* Editor only uses this. */
#define GetCurrLine(x)		get_int_prop(x, "CurrLine", INTERNAL_PROP)
#define SetCurrLine(x, y) \
	add_int_prop(x, "CurrLine", y, INTERNAL_PROP, PERM_PROP)
#define FreeCurrLine(x)		remove_prop(x, "CurrLine", INTERNAL_PROP)

/* If you want the interpreter to save out program run time info. */
/*
#define IncRunCnt(x, y) \
    add_int_prop(x, "TimesRun", \
		 get_int_prop(x, "TimesRun", INTERNAL_PROP) + (y), \
		 INTERNAL_PROP, PERM_PROP)
#define IncInstCnt(x, y) \
    add_int_prop(x, "InstCnt", \
		 get_int_prop(x, "InstCnt", INTERNAL_PROP) + (y), \
		 INTERNAL_PROP, PERM_PROP)
*/
#define IncRunCnt(x, y)		/* do nothing */
#define IncInstCnt(x, y)	/* do nothing */


/* These use db[x], not DB_(x) because flags and next can be gotten
   even for garbage objects. */
#define GetFlags(x)	(DB_CHK(x), (db[x]->flagsx))
#define SetFlags(x, y)	do { db[x]->flagsx = (y); } while (0)
#define SetOnFlag(x, y)	do { db[x]->flagsx |= (y); } while (0)
#define SetOffFlag(x, y)	do { db[x]->flagsx &= ~(y); } while (0)
#define HasFlag(x, y)	(DB_CHK(x), ((db[x]->flagsx & (y)) != 0))
#define Typeof(x) \
    ((x) == HOME ? TYPE_ROOM : (GetFlags(x) & TYPE_MASK))

#define GetNext(x)	(DB_CHK(x), (db[x]->nextx))
#define SetNext(x, y)	do { db[x]->nextx = (y); } while (0)


/* SetOwner shouldn't be used any more really, checkout chown_object. */
#define GetOwner(x)	(DB_CHK(x), (DB_(x)->ownerx))
#define SetOwner(x, y)	do { DB_(x)->ownerx = (y); } while (0)
#define GetOwnList(x)	(DB_CHK(x), (DB_(x)->ownlistx))
#define SetOwnList(x, y)	do { DB_(x)->ownlistx = (y); } while (0)
#define GetLoc(x)	(DB_CHK(x), (DB_(x)->locationx))
#define SetLoc(x, y)	do { DB_(x)->locationx = (y); } while (0)
#define GetContents(x)	(DB_CHK(x), (DB_(x)->contentsx))
#define SetContents(x, y)	do { DB_(x)->contentsx = (y); } while (0)
#define GetActions(x)	(DB_CHK(x), (DB_(x)->actionsx))
#define SetActions(x, y)	do { DB_(x)->actionsx = (y); } while (0)


/* Dest macros are for generic multiple destination things, exits, etc.
   They are also for objects/rooms/muf programs/people.
   Link macros are just for 1 destination things: obj/room/muf/people. */
#define GetNDest(x)	(DB_CHK(x), (DB_(x)->ndestx))
#define SetNDest(x, y)	do { DB_(x)->ndestx = (y); } while (0)
#define GetDest(x)	(DB_CHK(x), (DB_(x)->destx))
#define SetDest(x, y)	do { FreeDest(x); DB_(x)->destx = (y); } while (0)
#define FreeDest(x) \
    do { if (DB_(x)->destx) FREE(DB_(x)->destx); } while (0)

#define GetLink(x) \
   (DB_CHK(x), (DB_(x)->ndestx ? *(DB_(x)->destx) : NOTHING))
#define SetLink(x, y) \
    do { \
        FreeDest(x); \
        MALLOC(DB_(x)->destx, dbref, 1); \
        *(DB_(x)->destx) = (y); \
        if (*(DB_(x)->destx) == NOTHING) { \
            FreeLink(x); \
        } else { \
            SetNDest(x, 1); \
        } \
    } while (0)
#define FreeLink(x)	do { SetNDest(x, 0); FreeDest(x); } while (0)

/* Home of a room is global_environment, home of an exit is the room it's
   in, and home of anything else (player/muf/thing) is it's link.  This
   expands relatively large, I'd not use it much, or maybe should go into
   a function. */
#define GetHome(x) \
    (Typeof(x) == TYPE_ROOM ? global_environment : \
     (Typeof(x) == TYPE_EXIT ? GetLoc(x) : GetLink(x)))
      

#define GetProps(x)	(DB_CHK(x), &(DB_(x)->propertiesx))
#define	FreeProps(x)	free_all_props(x)

#define DOLIST(var, first) \
    for ((var) = (first); (var) != NOTHING; (var) = GetNext(var))

#define PushContents(x, y) \
    do { SetNext(y, GetContents(x)); SetContents(x, y); } while (0)
#define PushActions(x, y) \
    do { SetNext(y, GetActions(x)); SetActions(x, y); } while (0)

/* special dbref's */
#define NOTHING ((dbref) -1)	/* null dbref */
#define AMBIGUOUS ((dbref) -2)	/* multiple possibilities, for matchers */
#define HOME ((dbref) -3)	/* virtual room, represents mover's home */

#ifdef GOD_FLAGS
#define TrueGod(x) ((x) == head_player || HasFlag(x, GOD))
#else
#define TrueGod(x) ((x) == head_player)
#endif

#define God(x) (TrueGod(x) \
	        && ((Typeof(x) != TYPE_PLAYER) \
		    || !(HasFlag(x, QUELL))))


/* Wizard is a player who has a wizbit who isn't quelled. */
#define TrueWizard(x) (TrueGod(x) || HasFlag(x, WIZARD))
#define Wizard(x) (TrueWizard(x) \
		   && ((Typeof(x) != TYPE_PLAYER) \
		       || !(HasFlag(x, QUELL))))

#define Dark(x)   (HasFlag(x, DARK))

#ifdef MUCKER_ALL
#define Mucker(x) (Typeof(x) == TYPE_PLAYER)
#else /* !MUCKER_ALL */
#define Mucker(x) (Wizard(x) || HasFlag(x, MUCKER))
#endif /* MUCKER_ALL */

#ifndef RESTRICTED_BUILDING
#define Builder(x) (Typeof(x) == TYPE_PLAYER)
#else /* RESTRICTED_BUILDING */
#define Builder(x) (Wizard(x) || HasFlag(x, BUILDER))
#endif /* RESTRICTED_BUILDING */

#define Linkable(x) ((x) == HOME || (Typeof(x) == TYPE_ROOM ? \
				     HasFlag(x, ABODE) : \
				     HasFlag(x, LINK_OK)))

#define Garbage(x) ((x) < 0 || (x) >= db_top || Typeof(x) == TYPE_GARBAGE)

#define dbref2long(x) ((long) (x))
#define long2dbref(x) ((dbref) (x))


struct text;
struct interned_string;
struct shared_string		   /* for sharing strings in programs */
{
    int links;			   /* number of pointers to this struct */
    unsigned int length;	   /* length of string data */
    char data[1];		   /* shared string data */
};


struct prop_list
{
    struct prop_node *head;     /* First item in property list. */
    struct prop_node *tail;     /* Last item in property list. */
};


/* This is the form of the general object used by everything except garbage. */
struct general_object {
    object_flag_type flagsx;
    const char *namex;
    dbref nextx;		/* pointer to next in contents/exits chain */
    dbref locationx;		/* pointer to container */
    dbref ownerx;
    dbref ownlistx;		/* linked list of owned objects of player. */
    dbref contentsx;
    dbref actionsx;
    int ndestx;			/* Number of destinations of this thing. */
    dbref *destx;		/* home for obj/player, dropto on room */
				/* destinations for an exit */

    struct prop_list propertiesx;
};


/* For stuff using interpreter. */
#define INTERP_NO_THREAD 1
#define INTERP_OK_THREAD 0
struct frame;

/* For things using hash tables. */
struct hash_tab;
typedef struct hash_tab hash_tab;


/* Boolexps */
struct boolexp;
#define TRUE_BOOLEXP ((struct boolexp *) 0)


extern struct general_object **db;
extern dbref db_top;
extern dbref global_environment, player_start, head_player;
extern int object_counts[TYPE_MASK];

#endif /* MUCK_DB_H */
