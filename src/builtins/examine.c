/* Copyright (c) 1992 by David Moore.  All rights reserved. */
#include "copyright.h"
/* examine.c,v 2.5 1993/12/21 20:23:22 dmoore Exp */

#include "config.h"

#include <stdio.h>		/* If muf program text is shown. Ick. */

#include "db.h"
#include "params.h"
#include "match.h"
#include "externs.h"

/* prints owner of something */
static void print_owner(const dbref player, const dbref thing)
{
    switch (Typeof(thing)) {
    case TYPE_PLAYER:
	notify(player, "%n is a player.", thing);
	break;
    case TYPE_ROOM:
    case TYPE_THING:
    case TYPE_EXIT:
    case TYPE_PROGRAM:
	notify(player, "Owner: %n", GetOwner(thing));
	break;
    case TYPE_GARBAGE:
	notify(player, "<garbage> is garbage.");
	break;
    }
}


/* Actually generates the examine output for an object and shows it to
   the specified player.  This routine is used both by @examine and
   @extract.  If show_muf is true (in @extract) then the text of muf
   programs will be included in the output. */
void show_examine(const dbref player, const dbref thing, const int show_muf)
{
    dbref content;
    dbref exit;
    dbref *temp;
    int  i, ndest;
    const char *str;
    
    switch (Typeof(thing)) {
    case TYPE_ROOM:
	notify(player, "%u  Owner: %n  Parent: %u", player, thing,
	       GetOwner(thing), player, GetLoc(thing));
	break;
    case TYPE_THING:
	notify(player, "%u  Owner: %n  Value: %d", player, thing,
	       GetOwner(thing), GetPennies(thing));
	break;
    case TYPE_PLAYER:
	notify(player, "%u  Pennies: %d", player, thing, GetPennies(thing));
	break;
    case TYPE_EXIT:
    case TYPE_PROGRAM:
	notify(player, "%u  Owner: %n", player, thing, GetOwner(thing));
	break;
    default:
	notify(player, "Strange object type: %d", Typeof(thing));
	log_status("BAD OBJECT: %d, type: %d", thing, Typeof(thing));
	break;
    }
    /* unparse_long_flags: 0 -> not dump format */
    notify(player, "%s", unparse_long_flags(thing, 0));
    
    str = GetDesc(thing);
    if (str) notify(player, "%s", str);
    
    notify(player, "Key: %b", player, GetKey(thing));
    
    str = GetFail(thing);
    if (str) notify(player, "Fail: %s", str);
    
    str = GetSucc(thing);
    if (str) notify(player, "Success: %s", str);
    
    str = GetDrop(thing);
    if (str) notify(player, "Drop: %s", str);
    
    str = GetOFail(thing);
    if (str) notify(player, "Ofail: %s", str);
    
    str = GetOSucc(thing);
    if (str) notify(player, "Osuccess: %s", str);
    
    str = GetODrop(thing);
    if (str) notify(player, "Odrop: %s", str);
    
    
    /* show him the properties */
    show_properties(player, thing);
    
    /* show him the contents */
    if (GetContents(thing) != NOTHING) {
	if (Typeof(thing) == TYPE_PLAYER)
	    notify(player, "Carrying:");
	else
	    notify(player, "Contents:");
	DOLIST(content, GetContents(thing)) {
	    notify(player, "%u", player, content);
	}
    }
    
    switch (Typeof(thing)) {
    case TYPE_ROOM:
	/* tell him about exits */
	if (GetActions(thing) != NOTHING) {
	    notify(player, "Exits:");
	    DOLIST(exit, GetActions(thing)) {
		notify(player, "%u", player, exit);
	    }
	} else {
	    notify(player, "No exits.");
	}
	
	/* print dropto if present */
	if (GetLink(thing) != NOTHING) {
	    notify(player, "Dropped objects go to: %u",
		   player, GetLink(thing));
	}
	break;
    case TYPE_THING:
	/* print home */
	notify(player, "Home: %u", player, GetLink(thing));
	/* print location if player can link to it */
	if (GetLoc(thing) != NOTHING
	    && (controls(player, GetLoc(thing), Wizard(player))
		|| can_link_to(player, NOTYPE, GetLoc(thing)))) {
	    notify(player, "Location: %u", player, GetLoc(thing));
	}
	/* print thing's actions, if any */
	if (GetActions(thing) != NOTHING) {
	    notify(player, "Actions/exits:");
	    DOLIST(exit, GetActions(thing)) {
		notify(player, "%u", player, exit);
	    }
	} else {
	    notify(player, "No actions attached.");
	}
	break;
    case TYPE_PLAYER:
	/* print home */
	notify(player, "Home: %u", player, GetLink(thing));
	
	/* print location if player can link to it */
	if (GetLoc(thing) != NOTHING &&
	    (controls(player, GetLoc(thing), Wizard(player)) ||
	     can_link_to(player, NOTYPE, GetLoc(thing)))) {
	    notify(player, "Location: %u", player, GetLoc(thing));
	}
	
	/* print player's actions, if any */
	if (GetActions(thing) != NOTHING) {
	    notify(player, "Actions/exits:");
	    DOLIST(exit, GetActions(thing)) {
		notify(player, "%u", player, exit);
	    }
	} else {
	    notify(player, "No actions attached.");
	}
	break;
    case TYPE_EXIT:
	if (GetLoc(thing) != NOTHING) {
	    notify(player, "Source: %u", player, GetLoc(thing));
	}
	/* print destinations */
	ndest = GetNDest(thing);
	temp = GetDest(thing);
	for (i = 0; i < ndest; i++, temp++) {
	    switch (*temp) {
	    case NOTHING:
		break;
	    case HOME:
		notify(player, "Destination: *HOME*");
		break;
	    default:
		notify(player, "Destination: %u", player, *temp);
		break;
	    }
	}
	break;
    case TYPE_PROGRAM:
	if (get_code(thing)) {
	    int num_funcs, num_insts;
	    
	    get_code_size(get_code(thing), &num_funcs, &num_insts);
	    notify(player,
                   "Program compiled size: %i.  Number of functions: %i",
		   num_insts, num_funcs);
	} else {
            notify(player, "Program not compiled.");
	}
	
	/* print location if player can link to it */
	if (GetLoc(thing) != NOTHING &&
	    (controls(player, GetLoc(thing), Wizard(player))
	     || can_link_to(player, NOTYPE, GetLoc(thing)))) {
	    notify(player, "Location: %u", player, GetLoc(thing));
	}
	
	if (show_muf) {
	    /* FIX FIX FIX */
	    /* This really should use the editor list routine. */
	    FILE *prg;
	    Buffer buf;
	    
	    prg = fopen(make_progname(thing), "r");
	    if (!prg) break;
	    
	    notify(player, "--8<---cut---8<---");
	    while (Bufgets(&buf, prg, NULL)) {
		notify(player, "%s", Buftext(&buf));
	    }
	    fclose(prg);
	    notify(player, "--8<---cut---8<---");
	}
	break;
    default:
	/* do nothing */
	break;
    }
}


void do_examine(const dbref player, const char *name, const char *ignore1, const char *ignore2)
{
    dbref thing;
    struct match_data md;
    
    if (*name == '\0') {
	if ((thing = GetLoc(player)) == NOTHING) return;
    } else {
	/* look it up */
	init_match(player, name, NOTYPE, &md);
	match_all_exits(&md);
	match_neighbor(&md);
	match_possession(&md);
	match_absolute(&md);
	/* only Wizards can examine other players */
	if (Wizard(player)) match_player(&md);
	match_here(&md);
	match_me(&md);
	
	/* get result */
	if ((thing = noisy_match_result(&md)) == NOTHING) return;
    }
    
    if (can_link(player, thing)) {
	show_examine(player, thing, 0);
    } else {
	print_owner(player, thing);
    }
    
}
