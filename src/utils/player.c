/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* player.c,v 2.5 1994/02/20 23:04:17 dmoore Exp */
#include "config.h"

#include <stdio.h>		/* NULL */

#include "db.h"
#include "params.h"
#include "externs.h"

/* Entry for the player name -> dbref hashtable.
   NOTE: name _must_ be first, because the generic hashtable key generator
   and comparision function need it to be there. */
struct player_entry {
    const char *name;
    dbref object;
};

static hash_tab *player_list = NULL;

#ifdef DRUID_MUCK
/* Augh, at one point druid muck got some encrypted passwords in the
   database.  Allow these people to log in, but fix their passwords
   as they log in.  Once all of the old encrypted passwords are gone
   this code goes away. */
int check_crypt_password(const char *password, const char *guess)
{
    char salt[]="xx";
    char *crypted;

    salt[0] = password[0];
    salt[1] = password[1];
    crypted = crypt(guess, salt);

    return strcmp(crypted, password);
}
#endif /* DRUID_MUCK */


dbref connect_player(const char *name, const char *password)
{
    dbref player;
    const char *pass;
    
    player = lookup_player(name);

    if (player == NOTHING) return NOTHING;
    
    pass = GetPass(player);

    /* Any password works if none set. */
#ifdef DRUID_MUCK
    if (pass && *pass && strcmp(pass, password) && check_crypt_password(pass, password))
#else
    if (pass && *pass && strcmp(pass, password))
#endif
	return NOTHING;

#ifdef DRUID_MUCK
    SetPass(player, password);
#endif
    
    return player;
}


dbref create_player(const char *name, const char *password)
{
    dbref player;
    
    if(!ok_player_name(name) || !ok_password(password)) return NOTHING;
    
    /* else he doesn't already exist, create him */
    player = make_object(TYPE_PLAYER, name, NOTHING,
			 player_start, player_start);
    PushContents(player_start, player);
    
    /* initialize everything */
    SetPennies(player, 0);
    SetPass(player, password);
    
    add_player(player);
    
    return player;
}


void clear_players(void)
{
    clear_hash_table(player_list);
    player_list = NULL;
}


void delete_player(const dbref who)
{
    struct player_entry match;

    match.name = GetName(who);
    clear_hash_entry(player_list, &match);
}


void add_player(const dbref who)
{
    struct player_entry *entry;
    
    /* Initialize the hash table if needed. */
    if (player_list == NULL) {
	player_list = init_hash_table("Player table",
				      compare_generic_hash,
				      make_key_generic_hash,
				      delete_generic_hash,
				      PLAYER_HASH_SIZE);
    }

    MALLOC(entry, struct player_entry, 1);
    entry->name = GetName(who);
    entry->object = who;

    add_hash_entry(player_list, entry);
}


dbref lookup_player(const char *name)
{
    struct player_entry match;
    const struct player_entry *result;

    match.name = name;
    result = find_hash_entry(player_list, &match);

    if (result == NULL)
	return NOTHING;
    else
	return result->object;
}
