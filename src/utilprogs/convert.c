/* Copyright (c) 1992 by David Moore and Ian McCloghrie. All rights reserved. */
/* convert.c,v 2.9 1994/02/26 17:12:46 dmoore Exp */
#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "db.h"
#include "params.h"
#include "buffer.h"
#include "externs.h"

#define CheckFlag(x, y)		((x) & (y))
#define RemoveFlag(x, y)	((x) = ((x) & ~(y)))
#define AddFlag(x, y)		((x) = ((x) | (y)))

#if 0
/* This little section here will printout all lines as they are read in.
   This makes it easier to debug why it dies sometimes. */
const char *kludge;
long kludge2;
#define getstring(f) \
    (kludge = getstring(f), fprintf(stderr, "str: %s\n", kludge), kludge)
#define getref(f) \
    (kludge2 = getref(f), fprintf(stderr, "ref: %d\n", kludge2), kludge2)
#endif

/* All the old muck 2.2 values for the various flags. */
#define TYPE_ROOM2_2 	0x0
#define TYPE_THING2_2 	0x1
#define TYPE_EXIT2_2 	0x2
#define TYPE_PLAYER2_2 	0x3
#define TYPE_PROGRAM2_2	0x4 
#define TYPE_DAEMON2_2  0x5 /* not really used in 2.2, only in daemon */
#define TYPE_GARBAGE2_2	0x6
#define TYPE_MASK2_2 	0x7
#define WIZARD2_2	0x10
#define LINK_OK2_2	0x20
#define DARK2_2		0x40
#define INTERNAL2_2	0x80
#define STICKY2_2	0x100
#define BUILDER2_2	0x200
#define	CHOWN_OK2_2	0x400
#define JUMP_OK2_2	0x800
#define ENTER_OK_DAEMON 0x1000
#define NOSPOOF_DAEMON  0x2000
#define HAVEN2_2        0x10000
#define ABODE2_2        0x20000
#define MUCKER2_2       0x40000
#define INTERACTIVE2_2	0x200000
#define GOD_DAEMON      0x1000000
#define QUELL_DAEMON    0x2000000
#define VISUAL_DAEMON   0x4000000

#define GENDER_MASK	0x3000	/* 2 bits of gender */
#define GENDER_SHIFT	12	/* 0x1000 is 12 bits over (for shifting) */
#define GENDER_UNASSIGNED	0x0	/* unassigned - the default */
#define GENDER_NEUTER	0x1	/* neuter */
#define GENDER_FEMALE	0x2	/* for women */
#define GENDER_MALE	0x3	/* for men */

#define OLD_HAVEN	CHOWN_OK2_2
#define OLD_ABODE	JUMP_OK2_2
#define ANTILOCK	0x8

#define DB_OLD            0
#define DB_MUCK           1
#define DB_LACHESIS       2
#define DB_DORAN          3
#define DB_DAEMON         4
#define DB_DAEMON_PERM    5
#define DB_MULCH          6
#define DB_SHADOWS        7
#define DB_FB             8
#define DB_FIRISS         9

static int clean_antilock(long *inflags)
{
    int anti = 0;
    long flags = *inflags;

    if (CheckFlag(flags, ANTILOCK)) {
	RemoveFlag(flags, ANTILOCK);
	anti = 1;
    }

    *inflags = flags;
    return anti;
}


static void clean_gender(const dbref objno, long flags)
{
    switch (CheckFlag(flags, GENDER_MASK) >> GENDER_SHIFT) {
    case GENDER_NEUTER:
	add_string_prop(objno, "sex", "neuter", NORMAL_PROP, PERM_PROP);
	break;
    case GENDER_FEMALE:
	add_string_prop(objno, "sex", "female", NORMAL_PROP, PERM_PROP);
	break;
    case GENDER_MALE:
	add_string_prop(objno, "sex", "male", NORMAL_PROP, PERM_PROP);
	break;
    default:
	/* add_string_prop(objno, "sex", "unassigned"); */
	/* Except that puts sex properties on rooms and everything. */
	/* And having no sex set is identical to this. */
	break;
    }
}


static void clean_haven(long *inflags)
{
    long flags = *inflags;

    if (CheckFlag(flags, OLD_HAVEN)) {
	RemoveFlag(flags, OLD_HAVEN);
	AddFlag(flags, HAVEN2_2);
    }

    if (CheckFlag(flags, OLD_ABODE)) {
	RemoveFlag(flags, OLD_ABODE);
	AddFlag(flags, ABODE2_2);
    }
}


/* This should convert from 2.2 -> 2.3 flag rep. */
#define DoConvType(old, new) \
    do { if (oldtype == old) type = new; } while (0)
#define DoConvFlag(old, new) \
    do { if (CheckFlag(oldflags, old)) \
	     flags |= which_flag(type, #new); } while (0)

static object_flag_type convert_flags(long oldflags)
{
    long oldtype;
    object_flag_type flags;
    object_flag_type type;

    /* Get the type info. */
    type = NOTYPE;
    oldtype = CheckFlag(oldflags, TYPE_MASK2_2);
    DoConvType(TYPE_ROOM2_2, TYPE_ROOM);
    DoConvType(TYPE_THING2_2, TYPE_THING);
    DoConvType(TYPE_EXIT2_2, TYPE_EXIT);
    DoConvType(TYPE_PLAYER2_2, TYPE_PLAYER);
    DoConvType(TYPE_PROGRAM2_2, TYPE_PROGRAM);
    DoConvType(TYPE_GARBAGE2_2, TYPE_GARBAGE);
    DoConvType(TYPE_DAEMON2_2, TYPE_THING);

    /* if it's still NOTYPE should freak out, I guess... */
    if (type == NOTYPE) {
	abort();
    }

    /* Do the flags. */
    flags = type;
    if (flags != TYPE_GARBAGE) {
	RemoveFlag(oldflags, INTERACTIVE2_2);
	RemoveFlag(oldflags, INTERNAL2_2);
	DoConvFlag(WIZARD, WIZARD);
	DoConvFlag(LINK_OK, LINK_OK);
	if (type == TYPE_PROGRAM) DoConvFlag(DARK, DEBUG);
	else DoConvFlag(DARK, DARK);
	if (type == TYPE_PROGRAM) DoConvFlag(STICKY, SETUID);
	else DoConvFlag(STICKY, STICKY);
	DoConvFlag(BUILDER, BUILDER);
	DoConvFlag(CHOWN_OK, CHOWN_OK);
	DoConvFlag(JUMP_OK, JUMP_OK);
	DoConvFlag(HAVEN, HAVEN);
	DoConvFlag(ABODE, ABODE);
	DoConvFlag(MUCKER, MUCKER);
    }

    return flags;
}


static struct boolexp *clean_old_boolexp(const char *old, const int antilock)
{
    static Buffer buf;
    const char *p;
    int in_prop;		/* True if we are inside a prop dump [...]. */

    /* Empty string or old nothing key (-), return true boolexp. */
    if (!old || !*old || (*old == '-' && !*(old+1))) {
	if (antilock)
	    return parse_boolexp(NOTHING, "0&!0", 1);	/* Always false. */
	else
	    return TRUE_BOOLEXP;		/* Always true. */
    }

    Bufcpy(&buf, "");

    if (antilock) Bufcat(&buf, "!(");

    in_prop = 0;
    for (p = old; *p; p++) {
	if ((*p == '[') && !in_prop) {
	    in_prop = 1;
	} else if ((*p == ']') && in_prop) {
	    in_prop = 0;
	} else {
	    Bufcat_char(&buf, *p);
	}
    }

    if (antilock) Bufcat(&buf, ")");

    return parse_boolexp(NOTHING, Buftext(&buf), 1);
}


static void db_read_object_old(FILE *f, dbref objno)
{
    /* These are due to old db formats sticking the freaking flags down
       to far. */
    Buffer name, desc, fail, succ, ofail, osucc, lock, pass;
    dbref loc, contents, link, exits, next, owner;
    int value;
    int antilock;
    long oldflags;

    Bufcpy(&name, getstring(f));
    Bufcpy(&desc, getstring(f));
    loc = getref(f);
    contents = getref(f);
    link = getref(f);
    next = getnext(f);
    Bufcpy(&lock, getstring(f));
    Bufcpy(&fail, getstring(f));
    Bufcpy(&succ, getstring(f));
    Bufcpy(&ofail, getstring(f));
    Bufcpy(&osucc, getstring(f));
    owner = getref(f);
    value = atol(getstring(f));
    oldflags = atol(getstring(f));
    Bufcpy(&pass, getstring(f));

    clean_haven(objno);
    antilock = clean_antilock(&oldflags);
    clean_gender(objno, oldflags);
    update_flags(objno);

    SetKey(objno, clean_old_boolexp(Buftext(&lock), antilock));

    switch (Typeof(objno)) {
    case TYPE_ROOM:
	/* Dropto was in location, and action list was in link. */
	SetActions(objno, GetLink(objno));
	SetLink(objno, GetLoc(objno));
	SetLoc(objno, NOTHING);
	moveto(objno, global_environment);
	break;
    case TYPE_EXIT:
	/* Destination was in location. */
	SetLink(objno, GetLoc(objno));
	SetLoc(objno, NOTHING);
	/* Don't know where it belongs yet.  Let sanity checker fix it. */
	break;
    }
}


static void db_read_object_muck(FILE *f, dbref objno)
{
#if 0
    int antilock;
    Buffer lock;
    int ndest, i;
    dbref *temp;

    SetName(objno, getstring(f));
    SetDesc(objno, getstring(f));
    SetLoc(objno, getref(f));
    SetContents(objno, getref(f));
    SetNext(objno, getref(f));
    Bufcpy(&lock, getstring(f));
    SetFail(objno, getstring(f));
    SetSucc(objno, getstring(f));
    SetOFail(objno, getstring(f));
    SetOSucc(objno, getstring(f));
    SetFlags(objno, getref(f));

    clean_haven(objno);
    antilock = clean_antilock(objno);
    clean_gender(objno);
    update_flags(objno);

    SetKey(objno, clean_old_boolexp(Buftext(&lock), antilock));

    switch (Typeof(objno)) {
    case TYPE_THING:
	SetLink(objno, getref(f));
	SetActions(objno, getref(f));
	SetOwner(objno, getref(f));
	SetPennies(objno, getref(f));
	break;
    case TYPE_ROOM:
	SetLink(objno, getref(f));
	SetActions(objno, getref(f));
	SetOwner(objno, getref(f));
	break;
    case TYPE_EXIT:
	ndest = getref(f);
	SetNDest(objno, ndest);
	if (ndest) {
	    MALLOC(temp, dbref, ndest);
	    SetDest(objno, temp);
	    for (i = 0; i < ndest; i++, temp++)
		*temp = getref(f);
	} else {
	    SetDest(objno, NULL);
	}
	SetOwner(objno, getref(f));
	break;
    case TYPE_PLAYER:
	SetLink(objno, getref(f));
	SetActions(objno, getref(f));
	SetPennies(objno, getref(f));
	SetPass(objno, getstring(f));
	SetOwner(objno, objno);	/* Players own themselves. */
	break;
    }
#endif
}

static void getproperties22(const dbref objno, FILE *f, int dbtype)
{
    Buffer buf;
    char *p, *q;
    int internal;
    int ign;
    
    Bufgets(&buf, f, &ign);
    if (!strcmp(Buftext(&buf), "***Property list start ***"))
        Bufgets(&buf, f, &ign); /* Skip optional line. */
    
    while (strchr(Buftext(&buf), PROP_DELIMITER)) {
        /* Skip to the prop seperator. */
        for (p = Buftext(&buf); *p && (*p != PROP_DELIMITER); p++);
        *p++ = '\0';            /* Replace the delimiter with NUL. */
	/* if there are permissions in the prop, skip over them */
	if ((dbtype == DB_DAEMON_PERM) || (dbtype == DB_MULCH)) {
	    for (p = p; *p && (*p != PROP_DELIMITER); p++);
	    *p++ = '\0';
	}
	q = Buftext(&buf);
        if (*q == '\t') {
            q++;
            internal = INTERNAL_PROP;
        } else internal = NORMAL_PROP;
        if (!(*p && (*p == '^') && number(p + 1)))
            add_string_prop(objno, q, p, internal, PERM_PROP);
        else
            add_int_prop(objno, q, atol(p+1), internal, PERM_PROP);
        
        Bufgets(&buf, f, &ign);
    }
}

static void eat_times_and_backs(FILE *f, int dbtype)
{
  int i;
  char c;

  /* throw out the create, modify, and last used times in daemon */
    if ((dbtype == DB_DORAN) || (dbtype == DB_DAEMON) ||
        (dbtype == DB_DAEMON_PERM) || (dbtype == DB_MULCH) ||
        (dbtype == DB_SHADOWS)) {
	for (i=0; i<3; i++) {
	    getstring(f);
	}
    }
    /* throw out the backlinks/locks stored in later daemonmuck dbs */
    if ((dbtype == DB_DAEMON) || (dbtype == DB_DAEMON_PERM) ||
	(dbtype == DB_MULCH)) {
	getstring(f);       /* eat "*backlinks*" */
	while ((c = getc(f)) != '*') {
	    getstring(f);   /* eat all of the backlink lines */
	}
	getstring(f);       /* eat "*end backlinks*" */
	getstring(f);       /* eat "*backlocks*" */
	while ((c = getc(f)) != '*') {
	    getstring(f);   /* eat all of the backlock lines */
	}
    getstring(f);       /* eat "*end backlocks*" */
    }
}
/* 
 * handles most 2.2-ish dbs.
 *    lachesis, doran, daemon, daemon p, mulch, shadows
 */
static void db_read_object_twotwoish(FILE *f, dbref objno, int dbtype)
{
    int antilock;
    dbref *temp;
    int ndest, i;
    int c, daemon_flag = 0;
    /* These are due to old db formats sticking the freaking flags down
       to far. */
    Buffer name, desc, fail, succ, drop, ofail, osucc, odrop, lock;
    dbref loc, contents, link, exits, next, owner;
    int value;
    long oldflags;
    object_flag_type flags;

    Bufcpy(&name, getstring(f));
    Bufcpy(&desc, getstring(f));
    loc = getref(f);
    contents = getref(f);
    if (dbtype == DB_MULCH) {
	link = getref(f);
	exits = getref(f);
	value = getref(f);
    }
    next = getref(f);
    if ((dbtype == DB_DAEMON) || (dbtype == DB_DAEMON_PERM) ||
	(dbtype == DB_MULCH))
    {
	getref(f); /* throw away ownership list */
	owner = getref(f);
    }
    Bufcpy(&lock, getstring(f));
    Bufcpy(&fail, getstring(f));
    Bufcpy(&succ, getstring(f));
    Bufcpy(&drop, getstring(f));
    Bufcpy(&ofail, getstring(f));
    Bufcpy(&osucc, getstring(f));
    Bufcpy(&odrop, getstring(f));
    oldflags = atol(getstring(f));

    antilock = clean_antilock(&oldflags);

    /* if there's a daemon type floating around, make it a player. */
    if ((oldflags & TYPE_MASK2_2) == TYPE_DAEMON2_2) {
	oldflags &= ~TYPE_MASK2_2;
	oldflags |= TYPE_PLAYER2_2;
	daemon_flag = 1;  /* set a flag so we know to eat the data on it */
    }
    
    flags = convert_flags(oldflags);
    make_object_internal(objno, flags);
    if(!Garbage(objno)) {
	SetName(objno, Buftext(&name));
	SetLoc(objno, loc);
	SetContents(objno, contents);
	SetNext(objno, next);
	SetDesc(objno, Buftext(&desc));
	SetFail(objno, Buftext(&fail));
	SetSucc(objno, Buftext(&succ));
	SetDrop(objno, Buftext(&drop));
	SetOFail(objno, Buftext(&ofail));
	SetOSucc(objno, Buftext(&osucc));
	SetODrop(objno, Buftext(&odrop));
	SetKey(objno, clean_old_boolexp(Buftext(&lock), antilock));
	
	eat_times_and_backs(f,dbtype); /* eat the daemon-style useless info */

	c = getc(f);
	ungetc(c, f);		/* Stuff that character back. */
	if (c == '*') {
	    getproperties22(objno, f, dbtype);
	} else {
	    clean_gender(objno, oldflags);
	}
    } else { /* it is garbage */
	eat_times_and_backs(f,dbtype); /* eat the daemon-style useless info */
	c = getc(f);
	if(c == '*') {
	    while((c = getc(f)) != 'n')
		/* flush the line */;
	    while((c = getc(f)) != '\n')
		/* flush next line */;
	}
    }

    switch (Typeof(objno)) {
    case TYPE_THING:
	if (dbtype != DB_MULCH) {
	    link = getref(f);
	    exits = getref(f);
	    if ((dbtype == DB_LACHESIS) || (dbtype == DB_DORAN) ||
	        (dbtype == DB_SHADOWS))
	        owner = getref(f);
	    value = getref(f);
	}
	SetLink(objno, link);
	SetActions(objno, exits);
	SetOwner(objno, owner);
	SetPennies(objno, value);
	break;
    case TYPE_ROOM:
	if (dbtype != DB_MULCH) {
	    link = getref(f);
	    exits = getref(f);
	    if ((dbtype == DB_LACHESIS) || (dbtype == DB_DORAN) ||
		(dbtype == DB_SHADOWS))
		owner = getref(f);
	}
	SetOwner(objno, owner);
	SetLink(objno, link);
	SetActions(objno, exits);
	break;
    case TYPE_EXIT:
	ndest = getref(f);
	SetNDest(objno, ndest);
	if (ndest) {
	    MALLOC(temp, dbref, ndest);
	    SetDest(objno, temp);
	    for (i = 0; i < ndest; i++, temp++) {
		*temp = getref(f);
	    }
	} else {
	    SetDest(objno, NULL);
	}
	if ((dbtype == DB_LACHESIS) || (dbtype == DB_DORAN) ||
	    (dbtype == DB_SHADOWS)) {
	    owner = getref(f);
	}
	SetOwner(objno, owner);
	break;
    case TYPE_PLAYER:
	if (!daemon_flag) { /* daemons were converted into players */
	    if (dbtype != DB_MULCH) {
		link = getref(f);
		exits = getref(f);
		value = getref(f);
	    }
	    SetLink(objno, link);
	    SetActions(objno, exits);
	    SetPennies(objno, value);
/*
 * DaemonMUCK dbs have encrypt()ed passwords, 2.3 doesn't.  For now, let's
 * leave 'em like this.  OJ may add support for crypt()ed passes to 2.3,
 * dunno.
 */
	    SetPass(objno, getstring(f));
	    SetOwner(objno, objno);	/* Players own themselves. */
	} else {
/*
 * Note, this handling of daemons, currently, won't work right.  The
 * problem lies in the sanity checker, which ends up introducing cycles
 * into the db when it tries to fix the inconsistancies caused by the
 * fact that the converter doesn't know enough about the db to fix things
 * in a sane way.  The production sanity checker will do this right.
 * Until tile, if you've got daemons in your db, @recycle them all before
 * doing the conversion.  They're useless in 2.3 anyway.
 */
	    if (dbtype == DB_SHADOWS) {
		getref(f);                    /* throw away owner */
		SetOwner(objno, objno);       /* 'player' so owns itself */
		SetLoc(objno, 0);	      /* move it into #0 */
		SetLink(objno, getref(f));    /* get the home, sanity will fix*/
		SetActions(objno, getref(f)); /* we need the action list */
		SetPennies(objno, getref(f)); /* keep pennies, why not? */
		getstring(f);                 /* throw away password */
		SetPass(objno, "*&^%NOTAPASS"); /* impossible password */
	    } else {
		for (i=0; i<4; i++) {
		    getstring(f);
		}
	    }
	}
	break;
    case TYPE_PROGRAM:
	if ((dbtype == DB_LACHESIS) || (dbtype == DB_DORAN) ||
	    (dbtype == DB_SHADOWS))
	    owner = getref(f);
	SetOwner(objno, owner);
	SetLink(objno, GetOwner(objno));
	break;
    case TYPE_GARBAGE:
	break;
    }
}


static dbref db_read_driver(FILE *f, int dbtype)
{
    dbref objno;
    const char *line;
    int c;

    while (1) {
	c = getc(f);
	switch (c) {
	case '#':
	    objno = getref(f);
	    db_grow(objno+1);
/*
 * run all types through this driver eventually?  maybe.  do the old ones
 * have the same format with # and *s?
 */
	    switch (dbtype) {
	    case DB_OLD:
		return NOTHING;
		/* db_read_object_old(f, objno); */
		break;
	    case DB_MUCK:
		return NOTHING;
		/* db_read_object_muck(f, objno); */
		break;
	    case DB_LACHESIS:
	    case DB_DORAN:
	    case DB_DAEMON:
	    case DB_DAEMON_PERM:
	    case DB_MULCH:
	    case DB_SHADOWS:
		db_read_object_twotwoish(f, objno, dbtype);
		break;
/*
 * not implemented yet.  is there any point?  fuzzball has hundreds of db 
 * formats.
 */
	    case DB_FB:
		return NOTHING;
		/* db_read_object_fb(f, objno); */
		break;
	    case DB_FIRISS:
		return NOTHING;
		/* db_read_object_firiss(f, objno); */
		break;
	    }
	    break;
	case '*':
	    line = getstring(f);
	    if (strcmp(line, "**END OF DUMP***")) {
		return NOTHING;
	    } else {
		return db_top;
	    }
	    break;
	default:
	    return NOTHING;
	    /* break; */
	}
    }				
}


static dbref db_read_muck(FILE *f)
{
    return NOTHING;
}


static dbref db_read_old(FILE *f)
{
    return NOTHING;
}

static dbref db_read_any(FILE *f)
{
    const char *header;

    header = getstring(f);
    if (!strcmp(header, "***Firiss TinyMUCK 2.3 DUMP Format***")) {
	return db_read_firiss(f);
    } else if (!strcmp(header, "***Lachesis TinyMUCK DUMP Format***")) {
	return db_read_driver(f, DB_LACHESIS);
    } else if (!strcmp(header, "***TinyMUCK DUMP Format***")) {
	return db_read_muck(f);
    } else if (!strcmp(header, "***TinyMUCK Doran DUMP Format***")) {
	return db_read_driver(f, DB_DORAN);
    } else if (!strcmp(header, "***DaemonMUCK DUMP Format***")) {
	return db_read_driver(f, DB_DAEMON);
    } else if (!strcmp(header, "***DaemonMUCK P DUMP Format***")) {
	return db_read_driver(f, DB_DAEMON_PERM);
    } else if (!strcmp(header, "***MULCH***")) {
	return db_read_driver(f, DB_MULCH);
    } else if (!strcmp(header, "***Shadows TinyMUCK DUMP Format***")) {
	return db_read_driver(f, DB_SHADOWS);
    } else {
	return db_read_old(f);
    }
}

int main(int argc, const char * const *argv)
{
    int i;
    char *usage = "[-splayer_start] [-eglobal_environment] [-oconverted.db]";
    char *outputfile = NULL;

    global_environment = 0;
    player_start = 0;

    init_properties(100);
    init_intern_strings(100);

    for (i=1; i<argc; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	    case 'o':
		MALLOC(outputfile, char, strlen(&(argv[i][2]) + 1));
		strcpy(outputfile, &(argv[i][2]));
		break;
	    case 's':
		player_start = atoi(&(argv[i][2]));
		break;
	    case 'e':
		global_environment = atoi(&(argv[i][2]));
		break;
	    default:
		fprintf(stderr, "Usage:  %s %s\n", argv[0], usage);
		return 1;
	    }
	} else {
	    fprintf(stderr, "Usage:  %s %s\n", argv[0], usage);
	    return 1;
	}
    }
		    
    if (db_read_any(stdin) != NOTHING) {
	if (outputfile == (char *) NULL) {
	    db_write("convert.out");
	} else {
	    db_write(outputfile);
	}
	exit(0);
    } else {
	fprintf(stderr, "Couldn't load database.\n");
	exit(1);
    }
}
