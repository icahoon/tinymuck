#include "copyright.h"
/* predicates.c,v 2.8 1994/02/20 23:14:50 dmoore Exp */

#include "config.h"

#include <ctype.h>
#include <string.h>

#include "db.h"
#include "params.h"
#include "externs.h"


int can_link_to(const dbref who, const object_flag_type what_type, const dbref where)
{
    if (where == HOME) return 1;
    if (Garbage(where)) return 0;

    switch (what_type) {
    case TYPE_EXIT:
	return (controls(who, where, Wizard(who)) || HasFlag(where, LINK_OK));
	/*NOTREACHED*/
	break;
    case TYPE_PLAYER:
    case TYPE_ROOM:
	return ((Typeof(where) == TYPE_ROOM)
	        && (controls(who, where, Wizard(who))
		    || Linkable(where)));
	/*NOTREACHED*/
	break;
    case TYPE_THING:
	return (((Typeof(where) == TYPE_ROOM)
                 || (Typeof(where) == TYPE_PLAYER))
                && (controls(who, where, Wizard(who))
                    || Linkable(where)));
	/*NOTREACHED*/
	break;
    case NOTYPE:
	return (controls(who, where, Wizard(who))
		|| HasFlag(where, (ABODE|LINK_OK)));
	/*NOTREACHED*/
	break;
    }
    return 0;
}


/* If who is allowed to see the flags of what, in unparse_object. */
int can_see_flags(const dbref who, const dbref what)
{
    if (Garbage(what)) return 0;

    if (can_link_to(who, NOTYPE, what)) return 1;

    if (Typeof(what) == TYPE_PLAYER) return 0;

    return HasFlag(what, CHOWN_OK);
}


int can_link(const dbref who, const dbref what)
{
    if (Garbage(what)) return 0;

    if ((Typeof(what) == TYPE_EXIT)
	&& (GetNDest(what) == 0))
	return 1;

    return controls(who, what, Wizard(who));
}


int could_doit(const dbref player, const dbref thing)
{
    if (Garbage(thing)) return 0;

    if ((Typeof(thing) == TYPE_EXIT) && (GetNDest(thing) == 0)) { 
	return 0;
    }

    return eval_boolexp(player, GetKey(thing), thing);
}


int can_doit(const dbref player, const dbref thing, const char *default_fail_msg)
{
    dbref loc = GetLoc(player);

    if (Garbage(thing)) return 0; /* Should have never been called. */
  
    if (!could_doit(player, thing)) {
	/* can't do it */
	if (GetFail(thing)) {
	    exec_or_notify(player, thing, GetFail(thing), 0);
	} else if (default_fail_msg) {
	    notify(player, default_fail_msg);
	}
    
	if (GetOFail(thing) && !Dark(player)) {
	    notify_except(loc, player, "%n %p",
			  player, player, GetOFail(thing));
	}
    
	return 0;
    } else {
	/* can do it */
	if (GetSucc(thing)) {
	    exec_or_notify(player, thing, GetSucc(thing), 0);
	}
    
	if (GetOSucc(thing) && !Dark(player)) {
	    notify_except(loc, player, "%n %p",
			  player, player, GetOSucc(thing));
	}
    
	return 1;
    }
}


int can_see(const dbref player, const dbref thing, const int wizard, const int can_see_loc)
{
    if (Garbage(thing)) return 0; /* Should have never been called. */

    if (player == thing || Typeof(thing) == TYPE_EXIT
	|| Typeof(thing) == TYPE_ROOM) return 0;

    if (can_see_loc) {
	switch (Typeof(thing)) {
	case TYPE_PROGRAM:
	    return (HasFlag(thing, LINK_OK)
		    || controls(player, thing, wizard));
	default:
	    return (!Dark(thing) || controls(player, thing, wizard));
	}
    } else {
	/* can't see loc */
	return (controls(player, thing, wizard));
    }
}


/* No one controls garbage.  Wizards control everything.  Players control
   things they own. */
int controls(const dbref who, const dbref what, const int wizard)
{
    if (Garbage(what)) return 0;
    if (wizard) return 1;
    if (who == GetOwner(what)) return 1;

    return 0;
}


/* Are we allowed to set a certain kind of flag? */
int restricted(const dbref player, const dbref thing, const object_flag_type flag, const int wizplayer, const int godplayer)
{
    int result;

    if (Garbage(thing)) return 1; /* Shouldn't have been called. */

    switch (flag) {
    case DARK:
	result = (!wizplayer && (Typeof(thing) != TYPE_ROOM));
	break;
    case MUCKER:
    case BUILDER:
	result = !wizplayer;
	break;
    case WIZARD:
	if (wizplayer) {
	    result = ((Typeof(thing) == TYPE_PLAYER) && !godplayer);
	} else {
	    result = 1;
	}
	break;
#ifdef GOD_FLAGS
    case GOD:
	if (godplayer) {
	    result = ((Typeof(thing) == TYPE_PLAYER) && !godplayer);
	} else {
	    result = 1;
	}
	if (thing == head_player) {
	    result = 1;
	}
	break;
#endif
    default:
	result = 0;
	break;
    }
    return result;
}


int payfor(const dbref who, const int cost)
{
    int pennies;

    pennies = GetPennies(who);
    if (Wizard(who)) {
	return 1;
    } else if (pennies >= cost) {
	SetPennies(who, pennies - cost);
	return 1;
    } else {
	return 0;
    }
}


int word_start (const char *str, const char let)
{
    int chk;
  
    for (chk = 1; *str; str++) {
	if (chk && *str == let) return 1;
	chk = *str == ' ';
    }
    return 0;
}


int ok_name(const char *name)
{
    return (name
	    && *name
	    && *name != LOOKUP_TOKEN
	    && *name != NUMBER_TOKEN
	    && !strchr(name, ARG_DELIMITER)
	    && !strchr(name, AND_TOKEN)
	    && !strchr(name, OR_TOKEN)
	    && !word_start(name, NOT_TOKEN)
	    && muck_stricmp(name, "me")
	    && muck_stricmp(name, "home")
	    && muck_stricmp(name, "here"));
}


int ok_player_name(const char *name)
{
    const char *scan;
  
    if (!ok_name(name) || strlen(name) > PLAYER_NAME_LIMIT) return 0;
  
    for (scan = name; *scan; scan++) {
	if (!(isprint(*scan) && !isspace(*scan))) { /* was isgraph(*scan) */
	    return 0;
	}
    }
  
    /* lookup name to avoid conflicts */
    return (lookup_player(name) == NOTHING);
}


int ok_password(const char *password)
{
    const char *scan;
  
    if (*password == '\0') return 0;
  
    for (scan = password; *scan; scan++) {
	if (!(isprint(*scan) && !isspace(*scan))) {
	    return 0;
	}
    }
  
    return 1;
}
