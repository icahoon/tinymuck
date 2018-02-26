/* Copyright (c) 1992 by David Moore and Ben Jackson.  All rights reserved. */
/* dump_db.c,v 2.12 1997/08/10 04:30:41 dmoore Exp */
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "db.h"
#include "params.h"
#include "buffer.h"
#include "externs.h"

/* Return the dbref for the specified string #x (x >= 0), NOTHING,
   AMBIGUOUS, or HOME.  NOTHING if it isn't valid. */
/* NOTE:  for now, this returns for any dbref ( x < 0 or x >= 0).
   NOTHING only for invalid string. */
dbref parse_dbref(register const char *s)
{
    long x;
 
    if (!s) return NOTHING;

    x = atol(s);
    if (x != 0) return (dbref) x;

    /* atol was 0.  Check validity. */
    while (isspace(*s)) s++;

    if (*s == '0') return (dbref) 0;
    else return NOTHING;
}


void putref(FILE *f, const dbref ref)
{
    fprintf(f, "%ld\n", ref);
}


void putstring(FILE *f, const char *s)
{
    if (s) fputs(s, f);
    putc('\n', f);
}


/* This gives us one line of push back. */
static int use_last_string = 0;
static Buffer getstring_buf;

const char *getstring(FILE *f)
{
    int eof;

    if (use_last_string) {
	use_last_string = 0;
	return Buftext(&getstring_buf);
    } 

    return Bufgets(&getstring_buf, f, &eof);
}


void putbackstring(void)
{
    use_last_string = 1;
}


/* getref MUST use it's own buffer rather than calling getstring. */
dbref getref(FILE *f)
{
    Buffer buf;
    int eof;

    if (use_last_string) {
	use_last_string = 0;
        return (dbref) atol(Buftext(&getstring_buf));
    } 

    Bufgets(&buf, f, &eof);
    return (dbref) atol(Buftext(&buf));
}


/* tmuck2.3 dump format */
static int db_write_object(FILE *f, const dbref i)
{
    int  ndest, j;
    dbref *temp;

    /* unparse_long_flags: 1 -> dump format */
    putstring(f, unparse_long_flags(i, 1));
    putstring(f, GetName(i));
    putref(f, GetLoc(i));
    putref(f, GetOwner(i));
    putref(f, GetContents(i));
    putref(f, GetActions(i));
    putref(f, GetNext(i));

    ndest = GetNDest(i);
    temp = GetDest(i);
    putref(f, ndest);
    for (j = 0; j < ndest; j++, temp++)
	putref(f, *temp);

    putproperties(f, GetProps(i));

    return 0;
}


int db_write(const char *name)
{
    FILE *f;
    dbref i;

    f = fopen(name, "w");
    if (!f) return 0;

    fputs("***Firiss TinyMUCK 2.3 DUMP Format v1***\n", f);

    fprintf(f, "DB top: %ld\n", db_top);
    fprintf(f, "Growth: %d\n", 200);
    fprintf(f, "Properties: %ld\n", total_properties());
    fprintf(f, "Interned Strings: %ld\n", total_intern_strings());
    fprintf(f, "Hash Buckets: %ld\n", total_hash_entries());
    fprintf(f, "Boolexp Nodes: %ld\n", total_boolexp_nodes());
    fprintf(f, "Global environment: %ld\n", global_environment);
    fprintf(f, "Player start: %ld\n", player_start);
    fprintf(f, "Head player: %ld\n", head_player);

    if (feof(f) || ferror(f)) return 0;

    for (i = 0; i < db_top; i++) {
	if (Typeof(i) != TYPE_GARBAGE) {
	    fprintf(f, "#%ld\n", i);
	    db_write_object(f, i);
	    if (!(i % 100)) {
	        fflush(f);
		if (feof(f) || ferror(f)) return 0;
	    }
	}
    }

    fputs("***END OF DUMP***\n", f);
    fflush(f);
    if (feof(f) || ferror(f)) return 0;

    fclose(f);
    if (feof(f) || ferror(f)) return 0;

    return 1;
}

/* tmuck 2.3 db format */
static void db_read_object(FILE *f, const dbref objno)
{
    dbref *temp;
    int ndest, i;

    make_object_internal(objno, parse_flags(getstring(f)));

    SetName(objno, getstring(f));
    SetLoc(objno, getref(f));
    SetOwner(objno, getref(f));
    SetContents(objno, getref(f));
    SetActions(objno, getref(f));
    SetNext(objno, getref(f));

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

    getproperties(objno, f);

    /* Not really needed if programs all have proper homes. */
    if (Typeof(objno) == TYPE_PROGRAM) SetLink(objno, GetOwner(objno));
}


static int db_read_header(FILE *f)
{
    dbref old_dbtop;
    int old_growth;
    long old_properties;
    long old_interned;
    long old_hash;
    long old_bool;
    const char *line;

    /* Pre init things. */
    old_dbtop = 10000;
    old_growth = 200;
    old_properties = 1;
    old_interned = 1;
    old_hash = 1;
    old_bool = 1;
    global_environment = 0;
    player_start = 0;
    head_player = 1;

    /* Loop checking all lines of header until we hit the first object. */
    while (1) {
        line = getstring(f);

	if (*line == '#') break;

	if (muck_strprefix(line, "DB top: ")) {
	    old_dbtop = parse_dbref(strchr(line, ':')+2);
            continue;
	}
	if (muck_strprefix(line, "Growth: ")) {
	    old_growth = atoi(strchr(line, ':')+2);
	    continue;
	}
	if (muck_strprefix(line, "Properties: ")) {
	    old_properties = atol(strchr(line, ':')+2);
	    continue;
	} 
	if (muck_strprefix(line, "Interned Strings: ")) {
	    old_interned = atol(strchr(line, ':')+2);
	    continue;
	} 
	if (muck_strprefix(line, "Hash Buckets: ")) {
	    old_hash = atol(strchr(line, ':')+2);
	    continue;
	}
	if (muck_strprefix(line, "Boolexp Nodes: ")) {
	    old_bool = atol(strchr(line, ':')+2);
	    continue;
	}
	if (muck_strprefix(line, "Global environment: "))  {
	    global_environment = parse_dbref(strchr(line, ':')+2);
	    continue;
	}
	if (muck_strprefix(line, "Player start: ")) {
	    player_start = parse_dbref(strchr(line, ':')+2);
	    continue;
	}
	if (muck_strprefix(line, "Head player: ")) {
	    head_player = parse_dbref(strchr(line, ':')+2);
	    continue;
	}

	/* Only get here if this header is unknown. */
	log_status("DB Header: ignoring unknown field: %s", line);
    }

    putbackstring();			/* last line was start of object. */

    init_hash_entries(old_hash);
    init_properties(old_properties);
    init_intern_strings(old_interned);
    init_db(old_dbtop, old_growth);

    return 1;
}

dbref db_read_firiss(FILE *f)
{
    dbref expect, objno, j;
    const char *line;

    if (!db_read_header(f)) {
	fprintf(stderr, "Problem in database header.\n");
	return NOTHING;
    }

    expect = 0;
    while (1) {
	line = getstring(f);
	switch (*line) {
	case '#':
	    objno = parse_dbref(line+1);
	    db_grow(objno+1);
	    if (expect != objno) {
		for (j = expect; j < objno; j++) {
		    make_object_internal(j, TYPE_GARBAGE);
		}
	    }
	    db_read_object(f, objno);
	    if (Typeof(objno) == TYPE_PLAYER) {
		add_player(objno);
	    }
	    expect = objno+1;	/* Get ready to read next one. */
	    break;
	case '*':
	    if (strcmp(line, "***END OF DUMP***")) {
		return NOTHING;
	    } else {
		return db_top;
	    }
	default:
	    return NOTHING;
	    /* break; */
	}
    }				
}				


dbref db_read(FILE *f)
{
    const char *header;
    dbref read_result;
    dbref i;

    db_FREE();

    header = getstring(f);
    if (strcmp(header, "***Firiss TinyMUCK 2.3 DUMP Format v1***")) {
	fprintf(stderr, "Unsupported db format.\n");
	return NOTHING;
    }

    read_result = db_read_firiss(f);
    if (read_result == NOTHING) return NOTHING;

    if (!sanity_check()) return NOTHING;

    /* Update ownership	lists for the whole db.  Loop backwards
       over the db so that the head insertion of the list will
       stick them in sorted order. */
    for (i = db_top-1; i >= 0 ; i--) {
	if (Garbage(i)) continue;

	chown_object(i, GetOwner(i));
    }

    /* Stick db warning's here. */

    return read_result;
}



