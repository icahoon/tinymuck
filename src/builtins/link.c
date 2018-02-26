#include "copyright.h"
/* link.c,v 2.6 1997/08/10 03:12:20 dmoore Exp */

#include "config.h"

#include <ctype.h>

#include "db.h"
#include "params.h"
#include "match.h"
#include "buffer.h"
#include "externs.h"

/* The code in this file is essentially the 2.2 code w/ bug fixes. -- David */


/* Should really be in a header file. */
int link_exit(const dbref player, const dbref exit, const char *dest_name, dbref *dest_list);


/* do_link
 *
 * Use this to link to a room that you own.  It also sets home for
 * objects and things, and drop-to's for rooms.
 * It seizes ownership of an unlinked exit, and costs 1 penny
 * plus a penny transferred to the exit owner if they aren't you
 *
 * All destinations must either be owned by you, or be LINK_OK.
 */
void do_link(const dbref player, const char *thing_name, const char *dest_name, const char *ignore)
{
    dbref thing;
    dbref dest;
    dbref good_dest[MAX_LINKS];
    dbref *temp;
    struct match_data md;
  
    int  ndest, i;
  
    init_match(player, thing_name, TYPE_EXIT, &md);
    match_everything(&md, Wizard(player));
  
    if ((thing = noisy_match_result(&md)) == NOTHING) return;
  
    switch (Typeof(thing)) {
    case TYPE_EXIT:
	/* we're ok, check the usual stuff */
	if (GetNDest(thing) > 0) {
	    if (controls(player, thing, Wizard(player))) {
		notify(player, "That exit is already linked.");
		return;
	    } else {
		notify(player, "Permission denied.");
		return;
	    }
	}
	/* handle costs */
	if (GetOwner(thing) == player) {
	    if (!payfor(player, LINK_COST)) {
		notify(player,
		       "It costs a penny to link this exit.");
		return;
	    }
	} else {
	    if (!payfor(player, LINK_COST + EXIT_COST)) {
		notify(player,
		       "It costs two pennies to link this exit.");
		return;
	    } else if (!Builder(player)) {
		notify(player,
		       "Only authorized builders may seize exits.");
		return;
	    } else {
		/* pay the owner for his loss */
		IncPennies(GetOwner(thing), EXIT_COST);
	    }
	}
    
	/* link has been validated and paid for; do it */
	chown_object(thing, player);
	ndest = link_exit(player, thing, (char *) dest_name, good_dest);
	if (ndest == 0) {
	    /* refund */
	    notify(player, "No destinations linked.");
	    IncPennies(player, LINK_COST);
	    break;
	}
	SetNDest(thing, ndest);
	MALLOC(temp, dbref, ndest);
	SetDest(thing, temp);
	for (i = 0; i < ndest; i++, temp++) {
	    *temp = good_dest[i];
	}
	break;
    case TYPE_THING:
    case TYPE_PLAYER:
	init_match(player, dest_name, TYPE_ROOM, &md);
	match_me(&md);
	match_neighbor(&md);
	match_absolute(&md);
	match_here(&md);
	if ((dest = noisy_match_result(&md)) == NOTHING)
	    return;
	if (!controls(player, thing, Wizard(player))
	    || !can_link_to(player, Typeof(thing), dest)) {
	    notify(player, "Permission denied.");
	    return;
	}
	/* do the link */
	SetLink(thing, dest);
	notify(player, "Home set.");
	break;
    case TYPE_ROOM:		/* room dropto's */
	init_match(player, dest_name, TYPE_ROOM, &md);
	match_neighbor(&md);
	match_absolute(&md);
	match_home(&md);
	if ((dest = noisy_match_result(&md)) == NOTHING)
	    break;
	if (!controls(player, thing, Wizard(player))
	    || !can_link_to(player, Typeof(thing), dest)
	    || (thing == dest)) {
	    notify(player, "Permission denied.");
	} else {
	    SetLink(thing, dest); /* Set a dropto. */
	    notify(player, "Dropto set.");
	}
	break;
    case TYPE_PROGRAM:
	notify(player, "You can't link programs to things!");
	break;
    default:
	notify(player, "Internal error: weird object type.");
	log_status("PANIC: weird object: Typeof(%d) = %d",
		   thing, Typeof(thing));
	break;
    }
    return;
}


/* parse_linkable_dest()
 *
 * A utility for open and link which checks whether a given destination
 * string is valid.  It returns a parsed dbref on success, and NOTHING
 * on failure.
 */

/* 2/2/92 by dmoore to not overflow static buffers. */
static dbref parse_linkable_dest(const dbref player, const dbref exit, const char *dest_name)
{
    dbref  dobj;                /* destination room/player/thing/link */
    struct match_data md;

    init_match(player, dest_name, NOTYPE, &md);
    match_home(&md);
    match_everything(&md, Wizard(player));

    if ((dobj = match_result(&md)) == NOTHING || dobj == AMBIGUOUS) {
	notify(player, "I couldn't find '%s'.", dest_name);
	return NOTHING;
#ifndef TELEPORT_TO_PLAYER
    }
    if (Typeof(dobj) == TYPE_PLAYER) {
	notify(player, "You can't link to players.  Destination %u ignored.",
	       player, dobj);
	return NOTHING;
#endif				/* TELEPORT_TO_PLAYER */
    }
    if (!can_link(player, exit)) {
	notify(player, "You can't link that.");
	return NOTHING;
    }
    if (!can_link_to(player, Typeof(exit), dobj)) {
	notify(player, "You can't link to %u.", player, dobj);
	return NOTHING;
    } else {
	return dobj;
    }
}

/* exit_loop_check()
 *
 * Recursive check for loops in destinations of exits.  Checks to see
 * if any circular references are present in the destination chain.
 * Returns 1 if circular reference found, 0 if not.
 */
static int exit_loop_check(const dbref source, const dbref dest)
{
    int ndest, i;
    dbref *temp;

    if (source == dest) return 1; /* That's an easy one! */
    if (Typeof(dest) != TYPE_EXIT) return 0;
    
    ndest = GetNDest(dest);
    temp = GetDest(dest);
    for (i = 0; i < ndest; i++, temp++) {
	if (*temp == source) return 1; /* Found a loop. */
	if (Typeof(*temp) == TYPE_EXIT) {
	    /* Check recursively. */
	    if (exit_loop_check(source, *temp)) return 1;
	}
    }
    return 0;			/* No loops found */
}


/*
 * link_exit()
 *
 * This routine connects an exit to a bunch of destinations.
 *
 * 'player' contains the player's name.
 * 'exit' is the the exit whose destinations are to be linked.
 * 'dest_name' is a character string containing the list of exits.
 *
 * 'dest_list' is an array of dbref's where the valid destinations are
 * stored.
 *
 */
int link_exit(const dbref player, const dbref exit, const char *dest_name, dbref *dest_list)
{
    int  prdest;
    dbref  dest;
    int  ndest;
    static Buffer buf;
  
    prdest = 0;
    ndest = 0;
  
    while (*dest_name) {
	/* Skip white space. */
	while (*dest_name && isspace(*dest_name)) dest_name++;
	/* Reset buffer to hold exit name. */
	Bufcpy(&buf, "");
	/* Stuff characters of the exit name into the buffer. */
	while (*dest_name && (*dest_name != EXIT_DELIMITER)) {
	    Bufcat_char(&buf, *dest_name);
	    dest_name++;
	}
	/* If the last char was the delimiter, skip over it. */
	if (*dest_name) dest_name++;
    
	dest = parse_linkable_dest(player, exit, Buftext(&buf));
	if (dest == NOTHING) continue;
    
	switch (Typeof(dest)) {
	case TYPE_PLAYER:
	case TYPE_ROOM:
	case TYPE_PROGRAM:
	    if (prdest) {
		notify(player, "Only one player, room, or program destination allowed. Destination %u ignored.", player, dest);
		continue;
	    }
	    dest_list[ndest++] = dest;
	    prdest = 1;
	    break;
	case TYPE_THING:
	    dest_list[ndest++] = dest;
	    break;
	case TYPE_EXIT:
	    if (exit_loop_check(exit, dest)) {
		notify(player, "Destination %u would create a loop, ignored.",
		       player, dest);
		continue;
	    }
	    dest_list[ndest++] = dest;
	    break;
	default:
	    notify(player, "Internal error: weird object type.");
	    log_status("PANIC: weird object: Typeof(%d) = %d",
		       dest, Typeof(dest));
	    break;
	}
	if (dest == HOME) {
	    notify(player, "Linked to HOME.");
	} else {
	    notify(player, "Linked to %u.", player, dest);
	}
	if (ndest >= MAX_LINKS) {
	    notify(player, "Too many destinations, rest ignored.");
	    break;
	}
    }
    return ndest;
}
