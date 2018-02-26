/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* set.c,v 2.5 1993/12/21 20:23:23 dmoore Exp */
#include "config.h"

#include <string.h>
#include <ctype.h>

#include "db.h"
#include "params.h"
#include "match.h"
#include "flags.h"
#include "externs.h"


void do_set(const dbref player, const char *objname, const char *text, const char *ignore)
{
    dbref object;
    Buffer name, string;
    int bang = 0;
    object_flag_type flag;
    
    /* find thing */
    object = match_controlled(player, objname);
    if (object == NOTHING) return;

    if (!strchr(text, PROP_DELIMITER)) {
	/* It's a flag, not a property. */

	if (*text == '!') {
	    bang = 1;
	    text++;
	}

	if (!*text) {
	    notify(player, "You must specify a flag to set.");
	    return;
	}

	flag = which_flag(Typeof(object), text);

	if (flag == BAD_FLAG) {
	    notify(player, "I don't recognize that flag.");
	    return;
	}
    
	/* check for restricted flag */
	if (restricted(player, object, flag, Wizard(player), God(player))) {
	    notify(player, "Permission denied.");
	    return;
	}

	if (restricted(player, object, flag, 0, 0)) {
	    log_status("FLAG SET: %s %s on %u by %u.",
		       unparse_flag(object, flag, 1),
		       (bang ? "reset" : "set"),
		       GetOwner(object), object, player, player);
	}
    
	/* check for stupid wizard */
	if ((flag == WIZARD) && (object == player) && bang) {
	    notify(player, "You cannot make yourself mortal.");
	    return;
	}
    
	/* else everything is ok, do the set */
	if (bang) {
	    SetOffFlag(object, flag);
	    notify(player, "Flag reset.");
	} else {
	    SetOnFlag(object, flag);
	    notify(player, "Flag set.");
	}

	return;
    }

    /* Only get here if a property. */

    /* Copy off the name. */
    Bufcpy(&name, "");
    while (isspace(*text)) text++;
    while (*text && (*text != PROP_DELIMITER)) {
	Bufcat_char(&name, *text++);
    }
    Bufstw(&name);

    text++;			/* Skip prop delimiter. */

    /* Copy off the body. */
    while (isspace(*text)) text++;
    Bufcpy(&string, text);
    Bufstw(&string);

    if (!Buflen(&name)) {
	/* No name.  Either error or clear all props. */
	if (Buflen(&string)) {
	    notify(player, "No property name specified.");
	} else {
	    clear_nonhidden_props(object);
	    notify(player, "All properties removed.");
	}
    } else {
	if (Buflen(&string)) {
	    add_string_prop(object, Buftext(&name), Buftext(&string),
			    NORMAL_PROP, PERM_PROP);
	    notify(player, "Property set.");
	} else {
	    remove_prop(object, Buftext(&name), NORMAL_PROP);
	    notify(player, "Property removed.");
	}
    }
}

