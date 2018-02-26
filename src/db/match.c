#include "copyright.h"
/* match.c,v 2.8 1997/08/15 22:54:03 dmoore Exp */

#include "config.h"

/* Routines for parsing arguments */
#include <ctype.h>

#include "ansify.h"		/* random for next */
#include "db.h"
#include "params.h"
#include "match.h"
#include "buffer.h"
#include "externs.h"

Buffer match_args;		/* Arguments to an action link to program. */

void init_match(dbref player, const char *name, int type, struct match_data *md)
{
    md->exact_match = md->last_match = NOTHING;
    md->match_count = 0;
    md->match_who = player;
    md->match_name = name;
    md->check_keys = 0;
    md->preferred_type = type;
    md->longest_match = 0;
}

void init_match_check_keys(dbref player, const char *name, int type, struct match_data *md)
{
    init_match(player, name, type, md);
    md->check_keys = 1;
}

static dbref choose_thing(dbref thing1, dbref thing2, struct match_data *md)
{
    int has1;
    int has2;
    int preferred = md->preferred_type;
    
    if (thing1 == NOTHING) {
	return thing2;
    } else if (thing2 == NOTHING) {
	return thing1;
    }
    
    if (preferred != NOTYPE) {
	if (Typeof(thing1) == preferred) {
	    if (Typeof(thing2) != preferred) {
		return thing1;
	    }
	} else if (Typeof(thing2) == preferred) {
	    return thing2;
	}
    }
    
    if (md->check_keys) {
	has1 = could_doit(md->match_who, thing1);
	has2 = could_doit(md->match_who, thing2);
	
	if (has1 && !has2) {
	    return thing1;
	} else if (has2 && !has1) {
	    return thing2;
	}
	/* else fall through */
    }
    
    return (random() % 2 ? thing1 : thing2);
}

void match_player(struct match_data *md)
{
    dbref match;
    const char *p;
    
    if (*(md->match_name) == LOOKUP_TOKEN) {
	for (p = (md->match_name) + 1; isspace(*p); p++);
	if ((match = lookup_player(p)) != NOTHING) {
	    md->exact_match = match;
	}
    }
}

/* returns nnn if name = #nnn, else NOTHING */
static dbref absolute_name(struct match_data *md)
{
    dbref match;
    
    if (*(md->match_name) == NUMBER_TOKEN) {
	match = parse_dbref((md->match_name)+1);
	if (match < 0 || match >= db_top) {
	    return NOTHING;
	} else {
	    return match;
	}
    } else {
	return NOTHING;
    }
}

void match_absolute(struct match_data *md)
{
    dbref match;
    
    if ((match = absolute_name(md)) != NOTHING) {
	md->exact_match = match;
    }
}

void match_me(struct match_data *md)
{
    if (!muck_stricmp(md->match_name, "me")) {
	md->exact_match = md->match_who;
    }
}

void match_here(struct match_data *md)
{
    if (!muck_stricmp(md->match_name, "here")
	&& GetLoc(md->match_who) != NOTHING) {
	md->exact_match = GetLoc(md->match_who);
    }
}

void match_home(struct match_data *md)
{
    if (!muck_stricmp(md->match_name, "home"))
	md->exact_match = HOME;
}


static void match_list(dbref first, struct match_data *md)
{
    dbref absolute;
    
    absolute = absolute_name(md);
    if (!controls(md->match_who, absolute, Wizard(md->match_who)))
	absolute = NOTHING;
    
    DOLIST(first, first) {
	if (Garbage(first)) continue; /* Can this happen?? */
	if (first == absolute) {
	    md->exact_match = first;
	    return;
	} else if (!muck_stricmp(GetName(first), md->match_name)) {
	    /* if there are multiple exact matches, randomly choose one */
	    md->exact_match = choose_thing(md->exact_match, first, md);
	} else if (muck_strmatch(GetName(first), md->match_name)) {
	    md->last_match = first;
	    (md->match_count)++;
	}
    }
}

void match_possession(struct match_data *md)
{
    match_list(GetContents(md->match_who), md);
}

void match_neighbor(struct match_data *md)
{
    dbref loc;
    
    if ((loc = GetLoc(md->match_who)) != NOTHING) {
	match_list(GetContents(loc), md);
    }
}

/*
 * match_exits matches a list of exits, starting with 'first'.
 * It is will match exits of players, rooms, or things.
 */
void match_exits(dbref first, struct match_data *md)
{
    dbref exit, absolute;
    const char *exitname, *p;
    int i,exitprog;
    dbref *temp;
    int ndest;
    
    if (first == NOTHING) return;		/* Easy fail match */
    if ((GetLoc(md->match_who)) == NOTHING) return;
    
    absolute = absolute_name(md);		/* parse #nnn entries */
    if (!controls(md->match_who, absolute, Wizard(md->match_who)))
	absolute = NOTHING;
    
    DOLIST(exit, first) {
	if (exit == absolute) {
	    md->exact_match = exit;
	    continue;
	}
	exitprog = 0;
	ndest = GetNDest(exit);
	temp = GetDest(exit);
	if (temp) {
	    for (i = 0; i < ndest; i++, temp++)
		if (Typeof(*temp) == TYPE_PROGRAM)
		    exitprog = 1;
	}
	exitname = GetName(exit);
	while (*exitname) {			/* for all exit aliases */
	    for (p = md->match_name;		/* check out 1 alias */
		 *p
		 && tolower(*p) == tolower(*exitname)
		 && *exitname != EXIT_DELIMITER;
		 p++, exitname++);
	    /* did we get a match on this alias? */
	    if (*p == '\0' || (*p == ' ' && exitprog)) {
		/* make sure there's nothing afterwards */
		while (isspace(*exitname)) exitname++;
		if (*exitname == '\0' || *exitname == EXIT_DELIMITER) {
		    /* we got a match on this alias */
		    if (strlen(md->match_name)-strlen(p) > md->longest_match) {
			md->exact_match = exit;
			md->longest_match = strlen(md->match_name)-strlen(p);
			if (*p == ' ')
			    Bufcpy(&match_args, p+1);
			else
			    Bufcpy(&match_args, "");
		    }
		    else if (strlen(md->match_name)-strlen(p) == md->longest_match) {
			md->exact_match =
			    choose_thing(md->exact_match, exit, md);
			if (md->exact_match == exit)
			    if (*p == ' ')
				Bufcpy(&match_args, p+1);
			    else
				Bufcpy(&match_args, "");
		    }
		    goto next_exit;
		}
	    }
	    /* we didn't get it, go on to next alias */
	    while (*exitname && *exitname++ != EXIT_DELIMITER);
	    while (isspace(*exitname)) exitname++;
	} /* end of while alias string matches */
    next_exit:
	;
    }
}

/*
 * match_room_exits
 * Matches exits and actions attached to player's current room.
 * Formerly 'match_exit'.
 */
void match_room_exits(dbref loc, struct match_data *md)
{
    if (Typeof(loc) != TYPE_ROOM) return;
    if (GetActions(loc) != NOTHING)
	match_exits(GetActions(loc), md);
}

/*
 * match_invobj_actions
 * matches actions attached to objects in inventory
 */
void match_invobj_actions(struct match_data *md)
{
    dbref thing;
    
    if (GetContents(md->match_who) == NOTHING) return;

    DOLIST(thing, GetContents(md->match_who)) {
	if (Typeof(thing) == TYPE_THING
	    && GetActions(thing) != NOTHING) {
	    match_exits(GetActions(thing), md);
	}
    }
}

/*
 * match_roomobj_actions
 * matches actions attached to objects in the room
 */
void match_roomobj_actions(struct match_data *md)
{
    dbref thing, loc;
    
    if ((loc = GetLoc(md->match_who)) == NOTHING) return;
    if (GetContents(loc) == NOTHING) return;
    DOLIST(thing, GetContents(loc)) {
	if (Typeof(thing) == TYPE_THING
	    && GetActions(thing) != NOTHING) {
	    match_exits(GetActions(thing), md);
	}
    }
}

/*
 * match_player_actions
 * matches actions attached to player
 */
void match_player_actions(struct match_data *md)
{
    if (GetActions(md->match_who) == NOTHING) return;
    match_exits(GetActions(md->match_who), md);
}

/*
 * match_all_exits
 * Matches actions on player, objects in room, objects in inventory,
 * and room actions/exits (in reverse order of priority order).
 */
void match_all_exits(struct match_data *md)
{
    dbref loc;
    
    Bufcpy(&match_args, "");
    if (md->exact_match == NOTHING) {
	if ((loc = GetLoc(md->match_who)) != NOTHING)
	    match_room_exits(loc, md);
    }
    if (md->exact_match == NOTHING)
	match_invobj_actions(md);
    if (md->exact_match == NOTHING)
	match_roomobj_actions(md);
    if (md->exact_match == NOTHING)
	match_player_actions(md);
    while (md->exact_match == NOTHING
	   && (loc = GetLoc(loc)) != NOTHING) {
	match_room_exits(loc, md);
    }
}


void match_everything(struct match_data *md, int wizzed)
{
    match_me(md);
    match_here(md);
    match_absolute(md);
    if (wizzed)
	match_player(md);
    if (md->exact_match == NOTHING) {
        match_all_exits(md);
	match_neighbor(md);
	match_possession(md);
    }
}


dbref match_result(struct match_data *md)
{
    if (md->exact_match != NOTHING) {
	return (md->exact_match);
    } else {
	switch (md->match_count) {
	case 0:
	    return NOTHING;
	case 1:
	    return (md->last_match);
	default:
	    return AMBIGUOUS;
	}
    }
}

/* use this if you don't care about ambiguity */
dbref last_match_result(struct match_data *md)
{
    if (md->exact_match != NOTHING) {
	return (md->exact_match);
    } else {
	return (md->last_match);
    }
}

dbref noisy_match_result(struct match_data *md)
{
    dbref match;
    
    switch (match = match_result(md)) {
    case NOTHING:
	notify(md->match_who, NOMATCH_MESSAGE);
	return NOTHING;
    case AMBIGUOUS:
	notify(md->match_who, AMBIGUOUS_MESSAGE);
	return NOTHING;
    default:
	return match;
    }
}

void match_rmatch(dbref arg1, struct match_data *md)
{
    if (arg1 == NOTHING) return;
    switch (Typeof(arg1)) {
    case TYPE_PLAYER:
    case TYPE_ROOM:
	match_list(GetContents(arg1), md);
	match_exits(GetActions(arg1), md);
	break;
    case TYPE_THING:
	match_exits(GetActions(arg1), md);
	break;
    }
}


dbref do_match_boolexp(const dbref player, const char *what)
{
    struct match_data md;

    init_match(player, what, TYPE_THING, &md);
    match_me(&md);
    match_here(&md);
    match_absolute(&md);
    match_player(&md);
    if (md.exact_match == NOTHING) {
        match_neighbor(&md);
	match_possession(&md);
    }
    return match_result(&md);
}



dbref match_controlled(const dbref player, const char *name)
{
    dbref match;
    struct match_data md;
    
    init_match(player, name, NOTYPE, &md);
    match_everything(&md, Wizard(player));
    
    match = noisy_match_result(&md);
    if (match != NOTHING && !controls(player, match, Wizard(player))) {
	notify(player, "Permission denied.");
	return NOTHING;
    } else {
	return match;
    }
}
