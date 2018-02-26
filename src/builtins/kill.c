/* Copyright (c) 1992 by David Moore.  All rights reserved. */
#include "copyright.h"
/* kill.c,v 2.6 1997/06/28 01:06:09 dmoore Exp */

#include "config.h"

#include <stdlib.h>		/* atoi */

#include "ansify.h"		/* random for hpux */
#include "db.h"
#include "params.h"
#include "match.h"
#include "externs.h"


void do_kill(const dbref player, const char *what, const char *scost, const char *ignore)
{
    dbref victim;
    int cost, pennies;
    struct match_data md;
    
    cost = atoi(scost);
    init_match(player, what, TYPE_PLAYER, &md);
    match_neighbor(&md);
    match_me(&md);
    if (Wizard(player)) {
	match_player(&md);
	match_absolute(&md);
    }
    victim = match_result(&md);
    
    switch (victim) {
    case NOTHING:
	notify(player, "I don't see that player here.");
	break;
    case AMBIGUOUS:
	notify(player, "I don't know who you mean!");
	break;
    default:
	if (Typeof(victim) != TYPE_PLAYER) {
	    notify(player, "Sorry, you can only kill other players.");
	} else {
	    /* go for it */
	    /* set cost */
	    if (cost < KILL_MIN_COST) cost = KILL_MIN_COST;
	    
	    /* Changed 2/29/92 by dmoore, to check for haven in the
	       victim's location, not the player's location.  Makes
	       wizard far kills work right. */
	    if (HasFlag(GetLoc(victim), HAVEN)) {
		notify(player,
                       "You can't kill anyone %s!",
                       (GetLoc(victim) == GetLoc(player)) ? "here" : "there");
		break;
	    }
	    
	    /* see if it works */
	    if (!payfor(player, cost)) {
		notify(player, "You don't have enough pennies.");
	    } else if ((random() % KILL_BASE_COST) < cost
		      && !Wizard(victim)) {
		/* you killed him */
		if (GetDrop(victim))
		    /* give him the drop message */
		    notify(player, "%s", GetDrop(victim));
		else {
		    notify(player, "You killed %n!", victim);
		}
		
		/* now notify everybody else */
		if (GetODrop(victim)) {
		    notify_except(GetLoc(player), player,
				  "%n killed %n!  %p", player, victim,
				  player, GetODrop(victim));
		} else {
		    notify_except(GetLoc(player), player,
				  "%n killed %n!", player, victim);
		}

		/* in case of remote kill, also notify at the destination */
		if (GetLoc(player) != GetLoc(victim)) {
		    if (GetODrop(victim)) {
		        notify_except(GetLoc(victim), NOTHING,
				      "%n killed %n!  %p", player, victim,
				      player, GetODrop(victim));
		    } else {
		        notify_except(GetLoc(victim), NOTHING,
				      "%n killed %n!", player, victim);
		    }
		}

		/* maybe pay off the bonus */
		pennies = GetPennies(victim);
		if (pennies < MAX_PENNIES) {
		    notify(victim, "Your insurance policy pays %d pennies.",
			   KILL_BONUS);
		    IncPennies(victim, KILL_BONUS);
		} else {
		    notify(victim, "Your insurance policy has been revoked.");
		}
		/* send him home */
		send_home(victim);
		
	    } else {
		/* notify player and victim only */
		notify(player, "Your murder attempt failed.");
		notify(victim, "%n tried to kill you!", player);
	    }
	    break;
	}
    }
}
