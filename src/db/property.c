/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* property.c,v 2.12 1997/08/29 20:57:48 dmoore Exp */
#include "config.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "db.h"
#include "params.h"
#include "buffer.h"
#include "intern.h"
#include "externs.h"

#ifdef COMPRESS
extern const char *compress(const char *);
extern const char *uncompress(const char *);
#else
#define compress(x) (x)
#define uncompress(x) (x)
#endif /* COMPRESS */


/* The type of property this is.  Junked means that it was removed earlier,
   but for memory conservation (no prev ptr) and efficiency (no list scan),
   we don't remove it right away, rather wait until an opportune moment
   later to clean them up.  Such times are when we access a node and that
   node's next field is set to junked, or when we loop over all of the
   properties for an object. */
#define PROPERTY_JUNKED		0 /* To be removed prop entry. */
#define PROPERTY_INTEGER	1 /* Integer property. */
#define PROPERTY_STRING	        2 /* String property. */
#define PROPERTY_BOOLEXP	3 /* Boolexp property. */
#define PROPERTY_MACRO		4 /* Macro property. */
#define PROPERTY_DBREF		5 /* Dbref. */

struct prop_node
{
    unsigned int type:3;	/* Is it int, string, etc. */
    unsigned int internal:1;	/* Is this an internal property? */
    unsigned int temporary:1;	/* Is this a temporary property? */
    dbref      object;		/* What object is it on. */
    struct interned_string *name; /* Name of the property. */
    union {
	long value;
	dbref obj;
	const char *string;
	struct boolexp *bool;
    } un;
    struct prop_node *next;	/* Next item on property list. */
};


static long total_props = 0;
static hash_tab *property_table = NULL;
static struct prop_node *available_prop_nodes = NULL;


/* Used mostly by the db code to init the prop_list structure. */
void clear_prop_list(struct prop_list *plist)
{
    plist->head = NULL;
    plist->tail = NULL;
}


/* Free up the memory associated with a string or boolexp.  If they
   existed. */
static void clear_prop_union(struct prop_node *prop)
{
    if (!prop) return;

    if (prop->type == PROPERTY_STRING) {
	if (prop->un.string) FREE_STRING(prop->un.string);
    }

    if (prop->type == PROPERTY_BOOLEXP) {
	if (prop->un.bool) free_boolexp(prop->un.bool);
    }
}


/* Convert an object, name pair into a decent hash value, quickly. */
static unsigned long make_key_prop_entry(const void *entry)
{
    const struct prop_node *prop = entry;
    unsigned long obj_hash = dbref2long(prop->object);
    unsigned long name_hash = (unsigned long) prop->name;

    return obj_hash + name_hash;
}


/* Compare to property entries. */
static int compare_prop_entries(const void *entry1, const void *entry2)
{
    const struct prop_node *prop1 = entry1;
    const struct prop_node *prop2 = entry2;

    /* Must be same object. */
    if (prop1->object != prop2->object)
	return 1;

    /* Must agree on being internal or not. */
    if (prop1->internal != prop2->internal)
	return 1;

    /* Names must be the same. */
    if (prop1->name != prop2->name)
	return 1;

    return 0;
}


/* Allocate a hunk of prop_node's in one swoop to save malloc overhead.
   Stick them on a free list of prop_node's. */
static void generate_prop_nodes(long count)
{
    struct prop_node *hunk;
    int num_4k;
    int i, j;

    /* Try allocating them in 4K sets.  The -4 is for some mallocs
       which want to stick some overhead in the space.  This keeps
       things on a single page (hopefully). */
    num_4k = (4096 - 4) / sizeof(struct prop_node);

    count = (count / num_4k) + 1;

    for (i = 0; i < count; i++) {
	MALLOC(hunk, struct prop_node, num_4k);
	for (j = 0; j < num_4k; j++) {
	    hunk->next = available_prop_nodes;
	    available_prop_nodes = hunk;
	    hunk++;
	}
    }
}


/* Create a new property node, adding it to the linked list, and to the
   hash table. */
static struct prop_node *make_new_prop(const dbref object, const char *name, const int internal, const int temporary)
{
    struct prop_node *prop;
    struct prop_list *plist;

    plist = GetProps(object);

    /* Create the node. */
    if (plist->tail && (plist->tail->type == PROPERTY_JUNKED)) {
        /* this reuse of the tail node helps slightly with things that
	   add/remove a single property off of something. */
        /* #### a better more general solution is desirable */
        prop = plist->tail;
    } else {
        if (!available_prop_nodes) {
	    generate_prop_nodes(1);
	}
	prop = available_prop_nodes;
	available_prop_nodes = prop->next;
    }
    total_props++;

    prop->type = PROPERTY_INTEGER;	/* Has no other data associated. */
    prop->internal = internal;
    prop->temporary = temporary;
    prop->object = object;
    prop->name = make_interned_string(name);
    prop->un.value = 0;

    /* Add to hash table. */
    add_hash_entry(property_table, prop);

    /* Chain it in.  Adding to tail of list. */
    if (plist->tail && (plist->tail != prop)) {
	plist->tail->next = prop;
    }
    prop->next = NULL;
    plist->tail = prop;
    if (!plist->head) {
	/* First property on this object. */
	plist->head = prop;
    }

    return prop;
}


/* ''Free'' a property node by sticking it on a list of free properties.
   It might be smart to keep a counter of # of such properties and if
   it exceeds some limit to actually free things. */
static void free_prop_node(struct prop_node *prop)
{
    prop->next = available_prop_nodes;
    available_prop_nodes = prop;
}


/* Remove any trailing deleted nodes off of this one. */
static void delete_trailing_nodes(struct prop_node *prop)
{
    struct prop_node *temp, *next;
    struct prop_list *plist;

    if (!prop)
	return;

    temp = prop->next;
    if (!temp || (temp->type != PROPERTY_JUNKED))
        return;

    plist = GetProps(prop->object);

    /* Eat any trailing junked properties while we are here. */
    while (temp && (temp->type == PROPERTY_JUNKED)) {
	if (temp == plist->tail) plist->tail = prop;
	next = temp->next;
	free_prop_node(temp);
	temp = next;
    }
    prop->next = temp;
}


/* Try to find the property associated with the given object and name.
   If can't be found, return NULL. */
static struct prop_node *find_hash_prop(const dbref object, const char *name, const int internal)
{
    struct prop_node match;
    struct prop_node *result;

    match.name = find_interned_string(name);
    if (!match.name) return NULL;	/* Can't exist if not interned. */

    match.object = object;
    match.internal = internal;

    result = (struct prop_node *) find_hash_entry(property_table, &match);
    if (result) delete_trailing_nodes(result);

    return result;
}


/* Called by hash table when a prop_node is deleted from it. */
/* What we do here is check if it's the head node, in which case we
   change the head to point past it, and release it.  Otherwise we
   mark it as junked for later cleanup.  In any case we free up any strings,
   etc. */
static void delete_prop_entry(void *entry)
{
    struct prop_node *prop = entry;
    struct prop_list *plist;

    if (!prop) return;

    total_props--;

    delete_trailing_nodes(prop);
    clear_interned_string(prop->name);
    prop->name = NULL;
    clear_prop_union(prop);

    prop->type = PROPERTY_JUNKED;

    /* If prop is the head, skip the head down. */
    plist = GetProps(prop->object);
    if (prop == plist->head) {
	if (prop == plist->tail) {
	    /* Only node present. */
	    plist->head = NULL;
	    plist->tail = NULL;
	} else {
	    /* Move the head down. */
	    plist->head = prop->next;
	}
	free_prop_node(prop);
    }
}	

/* Set the property associated with with a given object and name to the
   specified integer value. */
void add_int_prop(const dbref object, const char *name, const long value, const int internal, const int temporary)
{
    struct prop_node *prop;

    if (!value) {
	/* Missing property acts like 0 value. */
	remove_prop(object, name, internal);
	return;
    }

    prop = find_hash_prop(object, name, internal);

    if (!prop) prop = make_new_prop(object, name, internal, temporary);

    clear_prop_union(prop);
    prop->type = PROPERTY_INTEGER;
    prop->un.value = value;
}


/* Set the property associated with with a given object and name to the
   specified dbref value. */
void add_dbref_prop(const dbref object, const char *name, const dbref value, const int internal, const int temporary)
{
    struct prop_node *prop;

    if (value == NOTHING) {
	/* Missing property acts like #-1 value. */
	remove_prop(object, name, internal);
	return;
    }

    prop = find_hash_prop(object, name, internal);

    if (!prop) prop = make_new_prop(object, name, internal, temporary);

    clear_prop_union(prop);
    prop->type = PROPERTY_DBREF;
    prop->un.obj = value;
}


/* Set the property associated with with a given object and name to the
   specified string value. */
void add_string_prop(const dbref object, const char *name, const char *string, const int internal, const int temporary)
{
    struct prop_node *prop;

    if (!string || !*string) {
	/* Missing property acts like empty string. */
	remove_prop(object, name, internal);
	return;
    }

    prop = find_hash_prop(object, name, internal);

    if (!prop) prop = make_new_prop(object, name, internal, temporary);

    clear_prop_union(prop);
    prop->type = PROPERTY_STRING;
    prop->un.string = ALLOC_STRING(compress(string));
}


/* Set the property associated with a given object and name to the
   specified boolexp value. */
void add_boolexp_prop(const dbref object, const char *name, struct boolexp *bool, const int internal, const int temporary)
{
    struct prop_node *prop;

    if (bool == TRUE_BOOLEXP) {
	/* Missing property acts like true boolexp. */
	remove_prop(object, name, internal);
	return;
    }

    prop = find_hash_prop(object, name, internal);

    if (!prop) prop = make_new_prop(object, name, internal, temporary);

    clear_prop_union(prop);
    prop->type = PROPERTY_BOOLEXP;
    prop->un.bool = bool;
}


/* Set the property associated with with a given object and name to be the
   specified string if not empty, otherwise the integer value. */
void add_property(const dbref object, const char *name, const char *string, const int value)
{
    if (string && *string) {
	add_string_prop(object, name, string, NORMAL_PROP, PERM_PROP);
    } else {
	add_int_prop(object, name, value, NORMAL_PROP, PERM_PROP);
    }
}


/* Remove the named property from the given object. */
void remove_prop(const dbref object, const char *name, const int internal)
{
    struct prop_node match;

    match.name = find_interned_string(name);
    if (!match.name) return;

    match.object = object;
    match.internal = internal;

    clear_hash_entry(property_table, &match);
}


/* Check to see if a player (or any of the player's contents) has a property
   with the name and string value given. */
int has_prop_contents(const dbref player, const char *name, const char *string)
{
    dbref obj;

    /* Check the player. */
    if (has_prop(player, name, string))
	return 1;

    /* Check player contents. */
    DOLIST(obj, GetContents(player)) {
	if (has_prop(obj, name, string))
	    return 1;
    }

    return 0;
}


/* Check to see if the given object has a property with the given name and
   string value. */
int has_prop(const dbref object, const char *name, const char *string)
{
    const char *p;

    p = get_string_prop(object, name, 0);
   
    if (!p) return 0;

    return (!muck_stricmp(string, p));
}


/* Return the property string associated with the given object and name.
   This string is uncompressed already, and is a pointer into a static
   buffer.  Don't call something else which uses uncompress or reads in
   a property.  Null if not found. */
const char *get_string_prop(const dbref object, const char *name, const int internal)
{
    struct prop_node *prop;

    prop = find_hash_prop(object, name, internal);

    if (!prop || (prop->type != PROPERTY_STRING))
	return NULL;

    return (uncompress(prop->un.string));
}


/* Return the integer property value associated with the given object and
   name.  0 if not found. */
int get_int_prop(const dbref object, const char *name, const int internal)
{
    struct prop_node *prop;

    prop = find_hash_prop(object, name, internal);

    if (!prop || (prop->type != PROPERTY_INTEGER))
	return 0;

    return prop->un.value;
}


/* Return the dbref property value associated with the given object and
   name.  #-1 if not found. */
dbref get_dbref_prop(const dbref object, const char *name, const int internal)
{
    struct prop_node *prop;

    prop = find_hash_prop(object, name, internal);

    if (!prop || (prop->type != PROPERTY_DBREF))
	return NOTHING;

    return prop->un.obj;
}


/* Return the boolexp property value associated with the given object and
   name.  TRUE_BOOLEXP if not found. */
struct boolexp *get_boolexp_prop(const dbref object, const char *name, const int internal)
{
    struct prop_node *prop;

    prop = find_hash_prop(object, name, internal);

    if (!prop || (prop->type != PROPERTY_BOOLEXP))
	return TRUE_BOOLEXP;

    return prop->un.bool;
}


/* Free all non hidden properties associated with an object. */
void clear_nonhidden_props(const dbref object)
{
    struct prop_list *plist;
    struct prop_node *prop, *next;

    plist = GetProps(object);

    if (!plist)
	return;

    for (prop = plist->head; prop; prop = next) {
	delete_trailing_nodes(prop); /* Insure no trailing properties. */
	next = prop->next;
	if (!prop->internal)
	    clear_hash_entry(property_table, prop);
    }
}


/* Free all properties associated with an object. */
void free_all_props(const dbref object)
{
    struct prop_node *prop, *next;
    struct prop_list *plist;

    plist = GetProps(object);

    if (!plist)
	return;

    /* Free all of the properties. */
    for (prop = plist->head; prop; prop = next) {
	delete_trailing_nodes(prop);
	next = prop->next;
	clear_hash_entry(property_table, prop);
    }
}


void show_properties(const dbref player, const dbref thing)
{
    struct prop_node *prop;
    struct prop_list *plist;
    int seen_one = 0;
    Buffer name;

    plist = GetProps(thing);

    if (!plist) return;

    for (prop = plist->head; prop; prop = prop->next) {
	delete_trailing_nodes(prop);

	/* Skip over internal properties. */
	if (prop->internal)
	    continue;

	/* Visible property. */
	if (!seen_one) {
	    notify(player, "Properties:");
	    seen_one++;
	}

	if (prop->type == PROPERTY_STRING) {
	    Bufcpy(&name, istring_text(prop->name));
	    notify(player, "%s: %s", Buftext(&name),
		   uncompress(prop->un.string));
        }

	if (prop->type == PROPERTY_INTEGER) {
#ifdef SHOWINTPROP
	    notify(player, "%s: %i", istring_text(prop->name), prop->un.value);
#else
	    notify(player, "%s: ", istring_text(prop->name));
#endif
	}

	if (prop->type == PROPERTY_DBREF) {
	    notify(player, "%s: #%d", istring_text(prop->name), prop->un.obj);
	}

	if (prop->type == PROPERTY_BOOLEXP) {
#ifdef SHOWINTPROP
	    notify(player, "%s: ", istring_text(prop->name));
#else
	    notify(player, "%s: ", istring_text(prop->name));
#endif
	}
    }
}


/* Make a copy of a property list. */
void copy_props(const dbref old, const dbref new)
{
    struct prop_list *plist;
    struct prop_node *prop;

    FreeProps(new);

    plist = GetProps(old);
    if (!plist) return;

    for (prop = plist->head; prop; prop = prop->next) {
	delete_trailing_nodes(prop);
	switch (prop->type) {
	case PROPERTY_INTEGER:
	    add_int_prop(new, istring_text(prop->name), prop->un.value,
			 prop->internal, prop->temporary);
	    break;
	case PROPERTY_STRING:
	    add_string_prop(new, istring_text(prop->name), prop->un.string,
			    prop->internal, prop->temporary);
	    break;
	case PROPERTY_DBREF:
	    add_dbref_prop(new, istring_text(prop->name), prop->un.obj,
			   prop->internal, prop->temporary);
	    break;
	case PROPERTY_BOOLEXP:
	    add_boolexp_prop(new, istring_text(prop->name),
			     copy_bool(prop->un.bool),
			     prop->internal, prop->temporary);
	    break;
	default:
	    log_status("PROPERTY: bad type in property list = %d\n",
		       prop->type);
	    break;
	}
    }
}


/* Write out a single property to a file. */
/* We compress the name on dump, and uncompress it on load. */
static void putprop(FILE *f, const struct prop_node *prop)
{
    const char *name;
    Buffer type_info, data;

    if (!prop) return;
    if (prop->temporary) return;	/* temp props don't write. */

    /* Store up what the type flags look like for this object. */
    Bufcpy(&type_info, ":");
    if (prop->internal) Bufcat_char(&type_info, 'i');
    if (prop->type == PROPERTY_INTEGER) Bufcat_char(&type_info, '^');
    if (prop->type == PROPERTY_DBREF) Bufcat_char(&type_info, '#');
    if (prop->type == PROPERTY_BOOLEXP) Bufcat_char(&type_info, '&');
    Bufcat_char(&type_info, ':');

    /* Figure out the data. */
    Bufcpy(&data, "");
    switch (prop->type) {
    case PROPERTY_INTEGER:
	Bufcat_int(&data, prop->un.value);
	break;
    case PROPERTY_STRING:
	Bufcat(&data, prop->un.string); 
	break;
    case PROPERTY_DBREF:
	Bufcat_dbref(&data, prop->un.obj);
	break;
    case PROPERTY_BOOLEXP:
	/* 1 means dump format. */
	Bufcat_boolexp(&data, NOTHING, prop->un.bool, 1);
	break;
    default:
	break;
    }

    name = istring_text(prop->name);

    /* Decide whether to print property on one line or to split it up. */
    if ((strlen(name) + Buflen(&type_info) + Buflen(&data)) >= BUFFER_LEN) {
	fprintf(f, "%s\n%s\n%s\n", name, Buftext(&type_info),
		Buftext(&data));
    } else {
	fprintf(f, "%s%s%s\n", name, Buftext(&type_info),
		Buftext(&data));
    }
}


/* Write out all the properties to a file.  An empty line signifies the end
   of the list in a dump. */
void putproperties(FILE *f, const struct prop_list *plist)
{
    struct prop_node *prop;

    if (plist) {
	for (prop = plist->head; prop; prop = prop->next) {
	    delete_trailing_nodes(prop);
	    putprop(f, prop);
	}
    }

    putc('\n', f);		/* Trailing blank line. */
}


/* Read a property list out of a file. */
void getproperties(const dbref objno, FILE *f)
{
    Buffer buf, name;
    char *p;
    const char *prop_name;
    int internal;
    int type;
    int multi_line;

    Bufgets(&buf, f, NULL);
    while (*Buftext(&buf)) {
	/* Skip to the prop seperator. */
	Bufcpy(&name, "");
	for (p = Buftext(&buf); *p && (*p != PROP_DELIMITER); p++)
	    Bufcat_char(&name, *p);
	if (*p) {
	    /* Skip seperator. */
	    p++;
	    multi_line = 0;
	} else {
	    /* Read in next line. */
	    Bufgets(&buf, f, NULL);
	    p = Buftext(&buf);
	    multi_line = 1;
	}

	/* Check the type. */
	internal = NORMAL_PROP;
	type = PROPERTY_STRING;
	for (; *p && (*p != PROP_DELIMITER); p++) {
	    if (*p == 'i') internal = INTERNAL_PROP;
	    if (*p == '^') type = PROPERTY_INTEGER;
	    if (*p == '#') type = PROPERTY_DBREF;
	    if (*p == '&') type = PROPERTY_BOOLEXP;
	}
	p++;
	if (multi_line) {
	    Bufgets(&buf, f, NULL);
	    p = Buftext(&buf);
	}

	/* Some db's had it compressed. */
	prop_name = uncompress(Buftext(&name));

	if (!prop_name || !*prop_name) {
	    /* Properties without names are very illegal nasty things. */
	    log_status("PROPERTY: property with no name on object %d.", objno);
	    panic("Bad property.");
	}

	if (type == PROPERTY_INTEGER)
	    add_int_prop(objno, prop_name, atol(p), internal, PERM_PROP);

	if (type == PROPERTY_STRING)
	    add_string_prop(objno, prop_name, p, internal, PERM_PROP);

	if (type == PROPERTY_DBREF) {
	    add_dbref_prop(objno, prop_name, parse_dbref(p), internal,
			   PERM_PROP);
	}

	if (type == PROPERTY_BOOLEXP) {
	    /* 1 means use dump format for parsing. */
	    add_boolexp_prop(objno, prop_name,
			     parse_boolexp(NOTHING, p, 1),
			     internal, PERM_PROP);
	}

	/* Read the next line. */
	Bufgets(&buf, f, NULL);
    }
}


/* Apply the given function to every property of an object. */
void map_string_props(const dbref object, void (*func)(const dbref obj, const char *name, const char *string))
{
    struct prop_list *plist;
    struct prop_node *prop;

    plist = GetProps(object);
    if (!plist)
	return;

    for (prop = plist->head; prop; prop = prop->next) {
	delete_trailing_nodes(prop);
	if (prop->type == PROPERTY_STRING) {
            func(prop->object, istring_text(prop->name),
		 uncompress(prop->un.string));
	}
    }
}


long total_properties(void)
{
    return total_props;
}


void init_properties(const long count)
{
    if (property_table) return;

    total_props = 0;
    property_table = init_hash_table("Property table", compare_prop_entries,
				     make_key_prop_entry, delete_prop_entry,
				     PROP_HASH_SIZE);

    generate_prop_nodes(count);
}
				   
				  
				   
