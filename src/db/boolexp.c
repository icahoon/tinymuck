/* Copyright (c) 1992 by David Moore.  All rights reserved. */
#include "copyright.h"
/* boolexp.c,v 2.8 1993/12/22 01:51:00 dmoore Exp */

#include "config.h"

#include <ctype.h>
#include <string.h>

#include "db.h"
#include "externs.h"
#include "params.h"
#include "buffer.h"

typedef char boolexp_type;

#define BOOLEXP_AND	0
#define BOOLEXP_OR	1
#define BOOLEXP_NOT	2
#define BOOLEXP_CONST	3
#define BOOLEXP_PROP	4

struct boolexp {
    boolexp_type type;
    union {
	struct {
	    struct boolexp *sub1;
	    struct boolexp *sub2;
	} bool;
	struct {
	    const char *name;
	    const char *string;
	} prop;
	dbref thing;
	struct boolexp *next;
    } un;
};


static struct boolexp *available_bools = NULL;
static long total_bools = 0;


static void generate_bools(long count)
{
    struct boolexp *hunk;
    int num_4k;
    int i, j;
 
    /* Try allocating them in 4K sets.  The -4 is for some mallocs
       which want to stick some overhead in the space.  This keeps
       things on a single page (hopefully). */
    num_4k = (4096 - 4) / sizeof(struct boolexp);
 
    count = (count / num_4k) + 1;
 
    for (i = 0; i < count; i++) {
        MALLOC(hunk, struct boolexp, num_4k);
        for (j = 0; j < num_4k; j++) {
            hunk->un.next = available_bools;
            available_bools = hunk;
            hunk++;
        }
    }
}


static struct boolexp *make_new_bool(void)
{
    struct boolexp *result;

    if (!available_bools) {
	generate_bools(1);
    }
    result = available_bools;
    available_bools = result->un.next;

    total_bools++;

    return result;
}


static void free_bool(struct boolexp *entry)
{
    entry->un.next = available_bools;
    available_bools = entry;

    total_bools--;
}

/* Lachesis note on the routines in this package:
 *   eval_booexp does just evaluation.
 *
 *   parse_boolexp makes potentially recursive calls to several different
 *   subroutines ---
 *          parse_boolexp_F
 *	      This routine does the leaf level parsing and the NOT.
 *	  parse_boolexp_E
 *	      This routine does the ORs.
 *	  pasre_boolexp_T
 *	      This routine does the ANDs.
 *
 *   Because property expressions are leaf level expressions, I have only
 *   touched eval_boolexp_F, asking it to call my additional parse_boolprop()
 *   routine.
 */

static struct boolexp *parse_boolexp_E(void); /* defined below */
static struct boolexp *parse_boolprop(const char *buf); /* defined below */

int eval_boolexp(const dbref player, const struct boolexp *b, const dbref thing)
{
    if (b == TRUE_BOOLEXP)
	return 1;

    switch (b->type) {
    case BOOLEXP_AND:
	return (eval_boolexp(player, b->un.bool.sub1, thing)
		&& eval_boolexp(player, b->un.bool.sub2, thing));
    case BOOLEXP_OR:
	return (eval_boolexp(player, b->un.bool.sub1, thing)
		|| eval_boolexp(player, b->un.bool.sub2, thing));
    case BOOLEXP_NOT:
	return !eval_boolexp(player, b->un.bool.sub1, thing);
    case BOOLEXP_CONST:
	if (Typeof(b->un.thing) == TYPE_PROGRAM) {
	    /* Feh on match_args. */
	    return interp(player, b->un.thing, thing,
			  INTERP_NO_THREAD, Buftext(&match_args));
	}
	return (b->un.thing == player
		|| member(b->un.thing, GetContents(player))
		|| b->un.thing == GetLoc(player));
    case BOOLEXP_PROP:
	return has_prop_contents(player, b->un.prop.name, b->un.prop.string);
    default:
	log_status("BOOLEXP: eval_boolexp called with bad type %i",
		   b->type);
	return 0;
    }
}


struct boolexp *copy_bool(const struct boolexp *old)
{
    struct boolexp *o;

    /* Changed 1/9/92 by dmoore, to check if old is NULL before mallocing. */
    if (!old)
	return 0;

    o = make_new_bool();
  
    if (!o)
	return 0;
  
    o->type = old->type;
  
    switch (old->type) {
    case BOOLEXP_AND:
    case BOOLEXP_OR:
	o->un.bool.sub1 = copy_bool(old->un.bool.sub1);
	o->un.bool.sub2 = copy_bool(old->un.bool.sub2);
	break;
    case BOOLEXP_NOT:
	o->un.bool.sub1 = copy_bool(old->un.bool.sub1);
	break;
    case BOOLEXP_CONST:
	o->un.thing = old->un.thing;
	break;
    case BOOLEXP_PROP:
	o->un.prop.name = ALLOC_STRING(old->un.prop.name);
	o->un.prop.string = ALLOC_STRING(old->un.prop.string);
	break;
    default:
	log_status("BOOLEXP: copy_boolexp: Error in boolexp!");
    }
    return o;
}

/* If the parser returns TRUE_BOOLEXP, you lose */
/* TRUE_BOOLEXP cannot be typed in by the user; use @unlock instead */
static const char *parsebuf;
static dbref parse_player;
static int parse_dump_format;

static void skip_whitespace(void)
{
    while(*parsebuf && isspace(*parsebuf)) parsebuf++;
}

static int boolexp_depth;	/* Check to see if they nest too much. */

/* F->(E); F->!F; F->object identifier */
static struct boolexp *parse_boolexp_F(void)
{
    struct boolexp *b;
    Buffer buf;

    if (++boolexp_depth > MAX_BOOLEXP_NEST) {
	notify(parse_player, "Nesting in boolean expression too deep.");
	return TRUE_BOOLEXP;
    }
    skip_whitespace();
    switch (*parsebuf) {
    case '\0':
	return TRUE_BOOLEXP;
	break;
    case '(':
	parsebuf++;
	b = parse_boolexp_E();
	skip_whitespace();
	if (b == TRUE_BOOLEXP || *parsebuf++ != ')') {
	    free_boolexp(b);
	    return TRUE_BOOLEXP;
	} else {
	    return b;
	}
	break;
    case NOT_TOKEN:
	parsebuf++;
	b = make_new_bool();
	b->type = BOOLEXP_NOT;
	b->un.bool.sub1 = parse_boolexp_F();
	if (b->un.bool.sub1 == TRUE_BOOLEXP) {
	    free_bool(b);
	    return TRUE_BOOLEXP;
	} else {
	    return b;
	}
	break;
    default:
	/* Either an object or property. */
	Bufcpy(&buf, "");
	while(*parsebuf
	      && *parsebuf != AND_TOKEN
	      && *parsebuf != OR_TOKEN
	      && *parsebuf != ')') {
	    Bufcat_char(&buf, *parsebuf++);
	}
	/* strip trailing whitespace */
	Bufstw(&buf);

	/* check to see if this is a property expression */
	if (strchr(Buftext(&buf), PROP_DELIMITER)) {
	    return parse_boolprop(Buftext(&buf));
	}
    
	b = make_new_bool();
	b->type = BOOLEXP_CONST;

	if (parse_dump_format) {
	    /* dbref is simple #. */
	    b->un.thing = parse_dbref(Buftext(&buf));
	    if (b->un.thing == NOTHING) {
		/* Couldn't figure it out. */
		free_bool(b);
		return TRUE_BOOLEXP;
	    }
	    return b;
	}

	/* Not dump format, do a match. */
	b->un.thing = do_match_boolexp(parse_player, Buftext(&buf));
    
	if (b->un.thing == NOTHING) {
	    notify(parse_player, "I don't see %s here.", Buftext(&buf));
	    free_bool(b);
	    return TRUE_BOOLEXP;
	} else if (b->un.thing == AMBIGUOUS) {
	    notify(parse_player, "I don't know which %s you mean!",
		   Buftext(&buf));
	    free_bool(b);
	    return TRUE_BOOLEXP;
	} else {
	    return b;
	}
	break;
    }
}

/* T->F; T->F & T */
static struct boolexp *parse_boolexp_T(void)
{
    struct boolexp *b;
    struct boolexp *b2;
  
    if ((b = parse_boolexp_F()) == TRUE_BOOLEXP) {
	return b;
    } else {
	skip_whitespace();
	if (*parsebuf == AND_TOKEN) {
	    parsebuf++;
      
	    b2 = make_new_bool();
	    b2->type = BOOLEXP_AND;
	    b2->un.bool.sub1 = b;
	    if ((b2->un.bool.sub2 = parse_boolexp_T()) == TRUE_BOOLEXP) {
		free_boolexp(b2);
		return TRUE_BOOLEXP;
	    } else {
		return b2;
	    }
	} else {
	    return b;
	}
    }
}

/* E->T; E->T | E */
static struct boolexp *parse_boolexp_E(void)
{
    struct boolexp *b;
    struct boolexp *b2;

    if ((b = parse_boolexp_T()) == TRUE_BOOLEXP) {
	return b;
    } else {
	skip_whitespace();
	if (*parsebuf == OR_TOKEN) {
	    parsebuf++;
      
	    b2 = make_new_bool();
	    b2->type = BOOLEXP_OR;
	    b2->un.bool.sub1 = b;
	    if ((b2->un.bool.sub2 = parse_boolexp_E()) == TRUE_BOOLEXP) {
		free_boolexp(b2);
		return TRUE_BOOLEXP;
	    } else {
		return b2;
	    }
	} else {
	    return b;
	}
    }
}

struct boolexp *parse_boolexp(const dbref player, const char *buf, const int dump_format)
{
    if (!buf || !*buf) return TRUE_BOOLEXP;

    boolexp_depth = 0;
    parsebuf = buf;
    parse_player = player;
    parse_dump_format = dump_format;
    return parse_boolexp_E();
}

/* parse a property expression
   If this gets changed, please also remember to modify set.c       */
static struct boolexp *parse_boolprop(const char *text)
{
    Buffer name;		/* Name of property to lock against. */
    Buffer string;		/* Text that needs to match. */
    struct boolexp *b;
  
    /* Copy off the name. */
    Bufcpy(&name, "");
    while (isspace(*text)) text++;
    while (*text && (*text != PROP_DELIMITER)) {
	Bufcat_char(&name, *text++);
    }
    Bufstw(&name);

    /* Check that the name really exists. */
    if (!Buflen(&name)) return TRUE_BOOLEXP;

    text++;			/* Skip prop delimiter. */

    /* Copy off the body. */
    Bufcpy(&string, "");
    while (isspace(*text)) text++;
    while (*text && (*text != PROP_DELIMITER)) {
	Bufcat_char(&string, *text++);
    }
    Bufstw(&string);

    if (!Buflen(&string)) return TRUE_BOOLEXP;

    b = make_new_bool();
    b->type = BOOLEXP_PROP;
    b->un.prop.name = ALLOC_CNT_STRING(Buftext(&name), Buflen(&name));
    b->un.prop.string = ALLOC_CNT_STRING(Buftext(&string), Buflen(&string));
    return b;
}



static Buffer boolexp_buf;
static dbref unparse_who;
static int unparse_dump_format;

static void unparse_boolexp1(const struct boolexp *b, const boolexp_type outer_type)
{
    if (b == TRUE_BOOLEXP) {
	if (!unparse_dump_format) {
	    Bufcat(&boolexp_buf, "*UNLOCKED*");
	}
	/* dump format is just empty string for this, nothing to do. */
    } else {
	switch (b->type) {
	case BOOLEXP_AND:
	    if (outer_type == BOOLEXP_NOT) Bufcat_char(&boolexp_buf, '(');
	    unparse_boolexp1(b->un.bool.sub1, b->type);
	    Bufcat_char(&boolexp_buf, AND_TOKEN);
	    unparse_boolexp1(b->un.bool.sub2, b->type);
	    if (outer_type == BOOLEXP_NOT) Bufcat_char(&boolexp_buf, ')');
	    break;
	case BOOLEXP_OR:
	    if (outer_type == BOOLEXP_NOT || outer_type == BOOLEXP_AND)
		Bufcat_char(&boolexp_buf, '(');
	    unparse_boolexp1(b->un.bool.sub1, b->type);
	    Bufcat_char(&boolexp_buf, OR_TOKEN);
	    unparse_boolexp1(b->un.bool.sub2, b->type);
	    if (outer_type == BOOLEXP_NOT || outer_type == BOOLEXP_AND)
		Bufcat_char(&boolexp_buf, ')');
	    break;
	case BOOLEXP_NOT:
	    Bufcat_char(&boolexp_buf, '!');
	    unparse_boolexp1(b->un.bool.sub1, b->type);
	    break;
	case BOOLEXP_CONST:
	    if (unparse_dump_format) {
		Bufcat_dbref(&boolexp_buf, b->un.thing);
	    } else {
		Bufcat_unparse(&boolexp_buf, unparse_who, b->un.thing);
	    }
	    break;
	case BOOLEXP_PROP:
	    Bufcat(&boolexp_buf, b->un.prop.name);
	    Bufcat_char(&boolexp_buf, ':');
	    if (b->un.prop.string)
		Bufcat(&boolexp_buf, b->un.prop.string);
	    break;
	default:
	    log_status("BOOLEXP: unexpected type (%i) in boolexp.",
		       b->type);
	    break;
	}
    }
}

const char *unparse_boolexp(const dbref player, const struct boolexp *b, const int dump_format)
{
    Bufcpy(&boolexp_buf, "");
    unparse_who = player;
    unparse_dump_format = dump_format;
    unparse_boolexp1(b, BOOLEXP_CONST);
    return Buftext(&boolexp_buf);
}


void free_boolexp(struct boolexp *b)
{
    if (b == TRUE_BOOLEXP)
	return;

    switch (b->type) {
    case BOOLEXP_AND:
    case BOOLEXP_OR:
	free_boolexp(b->un.bool.sub1);
	free_boolexp(b->un.bool.sub2);
	break;
    case BOOLEXP_NOT:
	free_boolexp(b->un.bool.sub1);
	break;
    case BOOLEXP_CONST:
	break;
    case BOOLEXP_PROP:
	FREE_STRING(b->un.prop.name);
	if (b->un.prop.string)
	    FREE_STRING(b->un.prop.string);
	break;
    default:
	log_status("BOOLEXP: bad type in boolexp, %i", b->type);
    }
    free_bool(b);
}


long total_boolexp_nodes(void)
{
    return total_bools;
}


void init_boolexp_nodes(const long count)
{
    generate_bools(count);
}
