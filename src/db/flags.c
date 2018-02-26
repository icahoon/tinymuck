/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* flags.c,v 2.8 1994/02/21 08:32:48 dmoore Exp */
#include "config.h"

#include "db.h"
#include "params.h"
#include "flags.h"
#include "buffer.h"
#include "externs.h"

struct object_type_name {
    const char *long_name;
    const char *short_name;
    object_flag_type kind;
};

static struct object_type_name type_names [] = {
    { "ROOM",           "R" },
    { "THING",          ""  },
    { "EXIT/ACTION",    "E" },
    { "PLAYER",         "P" },
    { "FORTH",          "F" },
    { "---",            ""  },
    { "GARBAGE",        ""  },
    { "NOTYPE",         ""  },
    { NULL, NULL }
};

struct object_flag_name {
    const char *long_name;
    const char *short_name;
    object_flag_type flag;
    object_flag_type kind;
};

static struct object_flag_name flag_names[] = {
#ifdef DRUID_MUCK
    { "INVISIBLE","I", INVISIBLE, TYPE_PLAYER },
#endif
#ifdef GOD_FLAGS
    { "GOD",	  "G", GOD,	 TYPE_ANY },
#endif
    { "WIZARD",   "W", WIZARD,   TYPE_ANY },
    { "LINK_OK",  "L", LINK_OK,  TYPE_ANY },
    { "DARK",     "D", DARK,     TYPE_ROOM },
    { "DARK",     "D", DARK,     TYPE_PLAYER },
    { "DARK",     "D", DARK,     TYPE_THING },
    { "DARK",     "D", DARK,     TYPE_EXIT },
    { "DEBUG",    "D", DEBUG,    TYPE_PROGRAM },
    { "STICKY",   "S", STICKY,   TYPE_ROOM },
    { "STICKY",   "S", STICKY,   TYPE_PLAYER },
    { "STICKY",   "S", STICKY,   TYPE_THING },
    { "STICKY",   "S", STICKY,   TYPE_EXIT },
    { "SETUID",   "S", SETUID,   TYPE_PROGRAM },
    { "BUILDER",  "B", BUILDER,  TYPE_ANY },
    { "CHOWN_OK", "C", CHOWN_OK, TYPE_ANY },
    { "JUMP_OK",  "J", JUMP_OK,  TYPE_ANY },
    { "HAVEN",    "H", HAVEN,    TYPE_ANY },
    { "ABODE",    "A", ABODE,    TYPE_ANY },
    { "MURKY",    "M", MURKY,    TYPE_ROOM},
    { "MUCKER",   "M", MUCKER,   TYPE_PLAYER },
    { "MUCKER",   "M", MUCKER,   TYPE_THING },
    { "MUCKER",   "M", MUCKER,   TYPE_EXIT },
    { "MUCKER",   "M", MUCKER,   TYPE_PROGRAM },
    { "QUELL",    "Q", QUELL,    TYPE_ANY },
    { "LINE_NUMBERS",  "",  LINE_NUMBERS, TYPE_PLAYER },
    { NULL, NULL, 0, 0 }
};


const char *unparse_flag(const dbref thing, object_flag_type flag, const int long_name)
{
    static Buffer buf;
    struct object_flag_name *temp;

    Bufcpy(&buf, "");
    for (temp = flag_names; temp->long_name; temp++) {
	if ((flag & temp->flag)
            && ((temp->kind == TYPE_ANY) || (Typeof(thing) == temp->kind))) {
	    if (long_name) Bufcat(&buf, temp->long_name);
	    else Bufcat(&buf, temp->short_name);
	}
    }

    return Buftext(&buf);
}


const char *unparse_flags(const dbref thing)
{
    static Buffer buf;
    struct object_flag_name *temp;

    /* Stick in the type, P, F, E, etc. */
    Bufcpy(&buf, type_names[Typeof(thing)].short_name);

    /* Loop building up the other flags. */
    for (temp = flag_names; temp->long_name; temp++) {
	if (HasFlag(thing, temp->flag)
	    && ((temp->kind == TYPE_ANY)
		|| (Typeof(thing) == temp->kind))) {
	    /* This object has both this flag set, and is the right type. */
	    Bufcat(&buf, temp->short_name);
	}
    }

    return Buftext(&buf);
}


const char *unparse_long_flags(const dbref thing, const int dump_format)
{
    struct object_flag_name *temp;
    int saw_a_flag = 0;
    static Buffer buf;

    /* Stick in the type. */
    if (!dump_format) {
	Bufcpy(&buf, "Type: ");
    } else {
	Bufcpy(&buf, "");
    }
    Bufcat(&buf, type_names[Typeof(thing)].long_name);

    /* Loop building up the other flags. */
    for (temp = flag_names; temp->long_name; temp++) {
	if (HasFlag(thing, temp->flag)
	    && ((temp->kind == TYPE_ANY)
		|| (Typeof(thing) == temp->kind))) {
	    /* This object has both this flag set, and is the right type. */
	    if (!dump_format && !saw_a_flag) {
		Bufcat(&buf, "  Flags:");
		saw_a_flag++;
	    }
	    Bufcatlist(&buf, " ", temp->long_name, NULL);
	}
    }

    return Buftext(&buf);
}


object_flag_type parse_flags(const char *flagstr)
{
    object_flag_type objtype, result;
    const char *p;
    struct object_flag_name *tflag;
    struct object_type_name *ttype;
    Buffer buf;

    result = 0;
    
    if (!flagstr) return 0;

    /* Copy out the type of the object. */
    Bufcpy(&buf, "");
    for (p = flagstr; *p && (*p != ' '); p++)
	Bufcat_char(&buf, *p);
   
    /* Move up flagstr to be just after found type. */
    if (*p) flagstr = p + 1;
    else flagstr = p;

    /* Lookup the type. */
    p = Buftext(&buf);
    for (ttype = type_names; ttype->long_name; ttype++) {
	if (!muck_stricmp(p, ttype->long_name)) {
	    objtype = result = ttype - type_names;
	    break;
	}
    }

    /* Loop collecting all of the other flags. */
    while (*flagstr) {
	Bufcpy(&buf, "");
	for (p = flagstr; *p && (*p != ' '); p++)
	    Bufcat_char(&buf, *p);

	/* Move up flagstr to just after found flag. */
	if (*p) flagstr = p + 1;
	else flagstr = p;

	/* Lookup the flag. */
	p = Buftext(&buf);
	for (tflag = flag_names; tflag->long_name; tflag++) {
	    if ((!muck_stricmp(p, tflag->long_name))
		&& ((tflag->kind == TYPE_ANY)
		    || (objtype == tflag->kind))) {
		result |= tflag->flag;
		break;
	    }
	}
    }

    return result;
}


/* Lookup the flag for a given object and string version of flag name.
   If no such flag return BAD_FLAG (-1). */
int which_flag(object_flag_type type, const char *flagstr)
{
    struct object_flag_name *temp;    

    /* Loop building up the other flags. */
    for (temp = flag_names; temp->long_name; temp++) {
	if ((temp->kind != TYPE_ANY) && (temp->kind != type))
	    continue;

	if (muck_strprefix(flagstr, temp->short_name)) {
	    if (muck_strprefix(temp->long_name, flagstr)) {
		return temp->flag;
	    } else {
		/* Matched beginning of a valid flag, but remainder is bad. */
		return BAD_FLAG;
	    }
	}
    }

    return BAD_FLAG;		/* Didn't end up matching anything. */
}
