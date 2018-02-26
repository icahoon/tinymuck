/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* macro.c,v 2.12 1997/08/18 19:11:35 dmoore Exp */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "db.h"
#include "externs.h"
#include "params.h"

/* Entry stored in hashtable (and linked list) storing all of the pertinent
   information about macro entries.
   NOTE: name _must_ be the first field of the struct.  This is because it
   uses the generic compare and key routines which expect it there. */
struct macro_entry {
    const char *name;		/* Name of macro. */
    const char *definition;	/* Expansion of the macro. */
    dbref player;		/* Player who created this entry. */
    struct macro_entry *next;	/* In order linked list. */
};

static struct macro_entry *macro_head = NULL; /* Linked list of macros. */
static hash_tab *macro_table = NULL; /* Fast access hash table. */


/* Free up the memory used by all the macros. */
void clear_macros(void)
{
    clear_hash_table(macro_table);
    macro_table = NULL;
    macro_head = NULL;
}


/* Routine called when a macro entry is removed. */
void delete_macro_entry(void *entry)
{
    struct macro_entry *macro, *prev;

    macro = (struct macro_entry *) entry;

    /* Snip it out of the linked list. */
    if (macro == macro_head) {
	macro_head = macro_head->next;
    } else {
	/* Find it in list. */
	for (prev = macro_head; prev; prev = prev->next)
	    if (macro == prev->next) {
		prev->next = macro->next;
		break;
	    }
    }

    FREE_STRING(macro->name);
    FREE_STRING(macro->definition);
    FREE(macro);
}


/* Add a new macro definition.  Return 0 if already present.  Otherwise
   add the definition and return 1. */
int new_macro(const char *name, const char *definition, const dbref player)
{
    struct macro_entry *macro, *prev;
    struct macro_entry dummy;
    Buffer buf;
    char *p;

    /* Make the name all lowercase. */
    Bufcpy(&buf, name);
    for (p = Buftext(&buf); *p; p++) *p = tolower(*p);
    name = Buftext(&buf);

    dummy.name = name;
    if (find_hash_entry(macro_table, &dummy))
	return 0;

    /* Find where it goes in the list. */
    dummy.next = macro_head;
    for (prev = &dummy; prev->next; prev = prev->next) {
	if (strcmp(name, prev->next->name) < 0)
	    break;
    }

    /* Make the new entry. */
    MALLOC(macro, struct macro_entry, 1);
    macro->name = ALLOC_STRING(name);
    if (definition && *definition) {
	macro->definition = ALLOC_STRING(definition);
    } else {
	macro->definition = ALLOC_STRING(" ");
    }
    macro->player = player;

    /* Chain it in. */
    if (prev == &dummy) {
	macro->next = prev->next;
	macro_head = macro;
    } else {
	macro->next = prev->next;
	prev->next = macro;
    }

    /* Add it to the hash table. */
    add_hash_entry(macro_table, macro);

    return 1;
}


/* Removes the named macro, returns 0 if fails, otherwise 1. */
int kill_macro(const dbref player, const char *name)
{
    struct macro_entry match;
    const struct macro_entry *result;

    match.name = name;
    result = find_hash_entry(macro_table, &match);

    if (!result) return 0;

    if ((player != result->player) && !Wizard(player))
	return 0;

    clear_hash_entry(macro_table, &match);

    return 1;
}


/* Chown macros from own player to another (for toading). */
void chown_macros(const dbref from, const dbref to)
{
    struct macro_entry *temp;

    for (temp = macro_head; temp; temp = temp->next) {
	if (temp->player == from) {
	    temp->player = to;
	}
    }
}


/* Return the macro definition for the given macro, or NULL if not found. */
const char *macro_expansion(const char *name)
{
    struct macro_entry match;
    const struct macro_entry *result;

    match.name = name;
    result = find_hash_entry(macro_table, &match);
    
    if (!result) return NULL;
    else return result->definition;
}


/* Read macros out of the already open file. */
void macroload(FILE *f)
{
    const char *line;
    Buffer buf;
    
    while ((line = getstring(f))) {
	Bufcpy(&buf, line);	/* Since getstring uses a static buffer. */
	line = getstring(f);
	new_macro(Buftext(&buf), line, getref(f));
    }
}


/* Dump the macros into the already opened file. */
int macrodump(const char *name)
{
    FILE *f;
    struct macro_entry *temp;

    f = fopen(name, "w");
    if (!f) return 0;

    for (temp = macro_head; temp; temp = temp->next) {
	putstring(f, temp->name);
	putstring(f, temp->definition);
	putref(f, temp->player);
	fflush(f);
	if (feof(f) || ferror(f)) return 0;
    }

    fclose(f);
    
    return 1;
}


/* Dump a list of macros in the specified range of dictionary. */
void show_macros(const char *first, const char *last, const int full, const dbref player)
{
    /* This should really be made to use a Buffer.  But it should be ok,
       until bufsprint gets field widths.  20 is to provide enough
       space for owner's name in displayed version.  FIX FIX FIX. */
    static char  buf[BUFFER_LEN+20];
    struct macro_entry *macro;
    int last_len, first_len;
    const char *player_name;

    last_len = strlen(last);
    first_len = strlen(first);

    buf[0] = '\0';		/* Used for abbreviated listing. */

    for (macro = macro_head; macro; macro = macro->next) {
	if (strncmp(macro->name, first, first_len) < 0)
	    continue;
	if (strncmp(macro->name, last, last_len) > 0)
	    break;
	    
	/* In the correct range. */
	if (full) {
	    if (Garbage(macro->player) 
		|| (Typeof(macro->player) != TYPE_PLAYER)) {
		log_status("BAD MACRO: macro owner is not a player: %s",
			   macro->name);
		player_name = "*ILLEGAL*";
	    } else {
		player_name = GetName(macro->player);
	    }

	    sprintf(buf, "%-16s %-16s  %s", macro->name,
		    player_name,
		    macro->definition);
	    notify(player, "%s", buf);
	} else {
	    /* Was originally 16. */
	    sprintf(buf + strlen(buf), "%-15s", macro->name);
	    if (strlen(buf) > 70) {
		notify(player, "%s", buf);
		buf[0] = '\0';
	    }
	}
    }

    /* flush last line if doing short mode. */
    if (!full && *buf)
	notify(player, "%s", buf);
}


void init_macros(void)
{
    /* Initialize hash table if needed. */
    if (macro_table) return;

    macro_table = init_hash_table("Macro table", compare_generic_hash,
				  make_key_generic_hash, delete_macro_entry,
				  MACRO_HASH_SIZE);
}
