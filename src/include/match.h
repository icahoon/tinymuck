/* match.h,v 2.4 1997/08/10 03:12:35 dmoore Exp */
#ifndef MUCK_MATCH_H
#define MUCK_MATCH_H

#include "buffer.h"
extern Buffer match_args;

struct match_data {
    dbref exact_match;	/* holds result of exact match */
    int check_keys;	        /* if non-zero, check for keys */
    dbref last_match;	/* holds result of last match */
    int match_count;	/* holds total number of inexact matches */
    dbref match_who;	/* player who is being matched around */
    const char *match_name;	/* name to match */
    int preferred_type;     /* preferred type */
    int longest_match;	/* longest matched string */
};


/* match functions */
/* Usage: init_match(player, name, type); match_this(); match_that(); ... */
/* Then get value from match_result() */

/* initialize matcher */
extern void init_match(dbref player, const char *name, int type, struct match_data *md);
extern void init_match_check_keys(dbref player, const char *name, int type, struct match_data *md);

/* match (LOOKUP_TOKEN)player */
extern void match_player(struct match_data *md);

/* match (NUMBER_TOKEN)number */
extern void match_absolute(struct match_data *md);

extern void match_me(struct match_data *md);

extern void match_here(struct match_data *md);

extern void match_home(struct match_data *md);

/* match something player is carrying */
extern void match_possession(struct match_data *md);

/* match something in the same room as player */
extern void match_neighbor(struct match_data *md);

/* match an exit from player's room */
extern void match_room_exits(dbref loc, struct match_data *md);

/* match an action attached to a room object */
extern void match_roomobj_actions(struct match_data *md);

/* match an action attached to an object in inventory */
extern void match_invobj_actions(struct match_data *md);

/* match an action attached to a player */
extern void match_player_actions(struct match_data *md);

/* match 4 above */
extern void match_all_exits(struct match_data *md);

/* only used for rmatch */
extern void match_rmatch(dbref, struct match_data *md);

/* all of the above, except only Wizards do match_player */
extern void match_everything(struct match_data *md, int wizzed);

/* return match results */
extern dbref match_result(struct match_data *md); /* returns AMBIGUOUS for
						     multiple inexacts */
extern dbref last_match_result(struct match_data *md); /* returns last
							  result */

#define NOMATCH_MESSAGE "I don't see that here."
#define AMBIGUOUS_MESSAGE "I don't know which one you mean!"

extern dbref noisy_match_result(struct match_data *md);
/* wrapper for match_result */
/* noisily notifies player */
/* returns matched object or NOTHING */

#endif /* MUCK_MATCH_H */
