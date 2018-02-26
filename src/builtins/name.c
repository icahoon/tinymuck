/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* name.c,v 2.3 1993/04/08 20:24:36 dmoore Exp */
#include "config.h"

#include <ctype.h>
#include <string.h>

#include "db.h"
#include "match.h"
#include "buffer.h"
#include "externs.h"


void do_name(const dbref player, const char *name, const char *newname, const char *ignore)
{
    Buffer buf;
    dbref thing;
    char *password;
    const char *pass;
    
    thing = match_controlled(player, name);
    if (thing == NOTHING) return;

    /* check for bad name */
    if (*newname == '\0') {
        notify(player, "Give it what new name?");
        return;
    }

    /* check for renaming a player */
    if (Typeof(thing) == TYPE_PLAYER) {
        Bufcpy(&buf, newname);
        newname = password = Buftext(&buf);
    
        /* split off password */
        while (*password && !isspace(*password)) password++;

        /* eat whitespace */
        if (*password) {
            *password++ = '\0'; /* terminate name */
            while(*password && isspace(*password)) password++;
        }

        pass = GetPass(thing);
            
        /* check for null password */
        if (!*password) {
            notify(player,
                   "You must specify a password to change a player name.");
            notify(player, "E.g.: name player = newname password");
            return;
        } else if (!pass || strcmp(password, pass))  {
            notify(player, "Incorrect password.");
            return;
        } else if (muck_stricmp(newname, GetName(thing))
                   && !ok_player_name(newname)) {
            notify(player, "You can't give a player that name.");
            return;
        }
        /* everything ok, notify */
        log_status("NAME CHANGE: %u to %s.", thing, thing, newname);
        delete_player(thing);
        SetName(thing, newname);
        add_player(thing);
        notify(player, "Name set.");
        return;
    } else {
        if (!ok_name(newname)) {
            notify(player, "That is not a reasonable name.");
            return;
        }
    }
    
    /* everything ok, change the name */
    SetName(thing, newname);
    notify(player, "Name set.");
}

