/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* stringutil.c,v 2.10 1997/08/16 22:34:13 dmoore Exp */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "db.h"
#include "buffer.h"
#include "externs.h"

#define tolower_unsigned(x) ((unsigned char) tolower(x))


unsigned long default_hash(register const char *s)
{
    unsigned long hashval;
    
    if (!s) return 0;

    for (hashval = 0; *s != '\0'; s++)
	hashval = (((unsigned int) *s) | 0x20) + 31 * hashval;
    
    return hashval;
}


int muck_stricmp(register const char *str1, register const char *str2)
{
    if (str1 == str2) return 0;	/* Same string. */

    if (!str1) str1 = "";
    if (!str2) str2 = "";

    while (*str1 && *str2 && (tolower(*str1) == tolower(*str2))) {
	str1++;
	str2++;
    }

    return tolower_unsigned(*str1) - tolower_unsigned(*str2);
}


int muck_strnicmp(register const char *str1, register const char *str2, register int n)
{
    if (str1 == str2) return 0;

    if (!str1) str1 = "";
    if (!str2) str2 = "";

    while ((n > 0) && *str1 && *str2 && (tolower(*str1) == tolower(*str2))) {
	str1++;
	str2++;
	n--;
    }
  
    return (n <= 0) ? 0 : tolower_unsigned(*str1) - tolower_unsigned(*str2);
}


int muck_instr(const char *str, const int lenstr, const char *pat, const int lenpat)
{
    const char *pos;

    if (!str || !pat) return 0;
    if (lenpat > lenstr) return 0;
    if (str == pat) return 1;

    pos = strstr(str, pat);

    if (!pos) return 0;
    else return (pos - str) + 1;
}


int muck_rinstr(register const char *str, int lenstr, register const char *pat, const int lenpat)
{
    const char *pos;

    if (!str || !pat) return 0;
    if (lenpat > lenstr) return 0;
    if (str == pat) return 1;

    pos = str + lenstr - 1;
    do {
	if (!strncmp(pos, pat, lenpat))
	    return pos - str + 1;
        pos -= 1;
    } while (pos >= str && *pos);
    return 0;
}


int muck_strprefix(register const char *string, register const char *prefix)
{
    if (!string) string = "";
    if (!prefix) prefix = "";

    while (*string && *prefix && (tolower(*string) == tolower(*prefix))) {
	string++;
	prefix++;
    }

    return (*prefix == '\0');
}


/* accepts only nonempty matches starting at the beginning of a word */
const char *muck_strmatch(register const char *src, register const char *sub)
{
    if (!src) src = "";
    if (!sub) sub = "";

    if (*sub) {
	while (*src) {
	    if (muck_strprefix(src, sub)) return src;

	    /* else scan to beginning of next word */
	    while (*src && isalnum(*src)) src++;
	    while (*src && !isalnum(*src)) src++;
	}
    }
    
    return NULL;
}


/*
 * pronoun_substitute()
 *
 * %-type substitutions for pronouns
 *
 * %a/%A for absolute possessive (his/hers/its, His/Hers/Its)
 * %s/%S for subjective pronouns (he/she/it, He/She/It)
 * %o/%O for objective pronouns (him/her/it, Him/Her/It)
 * %p/%P for possessive pronouns (his/her/its, His/Her/Its)
 * %r/%R for reflexive pronouns (himself/herself/itself,
 *                                Himself/Herself/Itself)
 * %n    for the player's name.
 */
#define GENDER_UNASSIGNED	0x0	/* unassigned - the default */
#define GENDER_NEUTER	0x1	/* neuter */
#define GENDER_FEMALE	0x2	/* for women */
#define GENDER_MALE	0x3	/* for men */

/* return old gender values for pronoun substitution code */
static int genderof(const dbref player)
{
    const char *sex;

    sex = get_string_prop(player, "sex", NORMAL_PROP);
    if (sex == NULL)
	return GENDER_UNASSIGNED;
    else if (!muck_stricmp(sex, "male"))
	return GENDER_MALE;
    else if (!muck_stricmp(sex, "female"))
	return GENDER_FEMALE;
    else if (!muck_stricmp(sex, "neuter"))
	return GENDER_NEUTER;
    else
	return GENDER_UNASSIGNED;
}


/* Created 2/1/92 by dmoore, to simplify the pronoun_substitute routine. */
/* This routine uses a static buffer to hold the string it's returning.
   So copy it away before you call this routine again. */
char *percent_sub(const dbref player, const char sub)
{
    char prn[3];
    const char *try_prop;
    char *result;
    static Buffer buf;
    static const char *subjective[4] = { "", "it", "she", "he" };
    static const char *possessive[4] = { "", "its", "her", "his" };
    static const char *objective[4] = { "", "it", "her", "him" };
    static const char *reflexive[4] = { "", "itself", "herself", "himself" };
    static const char *absolute[4] = { "", "its", "hers", "his" };
    
    /* Check if it's the % special case. */
    if (sub == '%') return "%";
    
    /* Try to get it from a property. */
    prn[0] = '%';
    prn[1] = sub;
    prn[2] = '\0';
    
    try_prop = get_string_prop(player, prn, 0);
    
    if (try_prop) {
	/* If it was a prop, store it away. */
	Bufcpy(&buf, try_prop);
    } else {
	/* Do the right thing based on gender of the player. */
	if (genderof(player) == GENDER_UNASSIGNED) {
	    if (strchr("NnSsOoRr", sub)) Bufsprint(&buf, "%n", player);
	    else if (strchr("PpAa", sub)) Bufsprint(&buf, "%n's", player);
	    else Bufcpy(&buf, "");
	} else {
	    switch (tolower(sub)) {
		/* The player's gender is known, lookup the right thing. */
	    case 'n': Bufsprint(&buf, "%n", player);
		break;
	    case 's': Bufcpy(&buf, subjective[genderof(player)]);
		break;
	    case 'o': Bufcpy(&buf, objective[genderof(player)]);
		break;
	    case 'p': Bufcpy(&buf, possessive[genderof(player)]);
		break;
	    case 'r': Bufcpy(&buf, reflexive[genderof(player)]);
		break;
	    case 'a': Bufcpy(&buf, absolute[genderof(player)]);
		break;
	    default:
		Bufcpy(&buf, "");
		break;
	    }
	}
    }
    
    result = Buftext(&buf);
    if (isupper(sub) && *result) *result = toupper(*result);
    return result;
}


/* Fixed 12/31/91 by dmoore, so that calling uncompress on properties
   doesn't nuke the uncompressed message string, since they both would
   use the static uncompress buffer. */
/* Changed 2/1/92 by dmoore, to use a seperate routine to collect the
   percent subs, and to prevent buffer overflow. */
char *pronoun_substitute(const dbref player, const char *str_in)
{
    static Buffer buf;		/* Storage for result. */
    Buffer input;		/* Copy of input string. */
    char *str;			/* Pointer into input or str_in. */
    
    Bufcpy(&input, str_in);
    str = Buftext(&input);
    
    Bufcpy(&buf, "");
    while (*str) {
	if (*str == '%') {
	    str++;
	    if (*str) Bufcat(&buf, percent_sub(player, *str++));
	} else {
	    Bufcat_char(&buf, *str++);
	}
    }
    
    return Buftext(&buf);
} 


struct shared_string *make_shared_string(const char *str, const unsigned int len)
{
    struct shared_string *result;

    if (!str || !*str) return NULL;

    /* The size field below does not need a +1, since the structure
       already has 1 itself. */
    MALLOCEXACT(result, struct shared_string,
		sizeof(struct shared_string) + len);

    result->links = 1;
    result->length = len;
    strcpy(result->data, str);

    return result;
}

struct shared_string *make2_shared_string(const char *str1, const unsigned int len1, const char *str2, const unsigned int len2)
{
    struct shared_string *result;
    int len = len1 + len2;

    if (!len) return NULL;

    if (len > BUFFER_LEN - 1)
        len = BUFFER_LEN - 1;

    /* The size field below does not need a +1, since the structure
       already has 1 itself. */
    MALLOCEXACT(result, struct shared_string,
		sizeof(struct shared_string) + len);

    result->links = 1;
    result->length = len;
    if (len1) strcpy(result->data, str1);
    if (len > len1) strncpy(result->data+len1, str2, len-len1+1);

    return result;
}


struct shared_string *dup_shared_string(struct shared_string *s)
{
    if (s) s->links++;
    return s;
}


void clear_shared_string(struct shared_string *s)
{
    if (!s) return;

    if (--s->links > 0) return;

    FREE(s);
}


/* Added 2/1/92 by dmoore, to pull subst into a nice seperate function. */
/* This returns it's result in a static buffer, so copy it out before
   calling the routine again.  Does not allow buffer overflow. */
const char *muck_subst(const char *str, const char *orig, const int origlen, const char *repl, int *resultlen)
{
    static Buffer buf;
    
    if (!str || !*str) {
	*resultlen = 0;
	return "";
    }

    /* Must have something to replace. */
    if (!orig || !*orig) {
	Bufcpy(&buf, str);

	*resultlen = Buflen(&buf);
	return Buftext(&buf);
    }
    
    if (!repl) repl = "";

    Bufcpy(&buf, "");
    while (*str) {
	if (!strncmp(str, orig, origlen)) {
	    Bufcat(&buf, repl);
	    str += origlen;	/* Skip past the match. */
	} else {
	    /* Doesn't match the original. */
	    Bufcat_char(&buf, *str++);
	}
    }
    
    *resultlen = Buflen(&buf);
    return Buftext(&buf);
}


const char *make_progname(const dbref prog)
{
    static Buffer buf;

    Bufsprint(&buf, "muf/%d.m", prog);
    return Buftext(&buf);
}


int notify(const dbref player, const char *message, ...)
{
    Buffer buf;
    va_list ap;
    
    if (player == NOTHING) return 1;

    va_start(ap, message);
    Bufvsprint(&buf, message, ap);
    va_end(ap);
    
    return send_player(player, Buftext(&buf), Buflen(&buf));
}


void notify_all(const char *message, ...)
{
    Buffer buf;
    va_list ap;

    va_start(ap, message);
    Bufvsprint(&buf, message, ap);
    va_end(ap);

    send_all(Buftext(&buf), Buflen(&buf), 0); /* 0 == not important */
}


void notify_except(const dbref where, const dbref except, const char *msg, ...)
{
    Buffer buf;
    char *ptr;
    int len;
    va_list ap;

    if (where == NOTHING) return;

    va_start(ap, msg);
    Bufvsprint(&buf, msg, ap);
    va_end(ap);
    ptr = Buftext(&buf);
    len = Buflen(&buf);

#if 0
    /* This version may be faster when a hashtable lookup of
       dbref->descriptors exists, and the number of objects in a room
       is much smaller compared to the number of connected players.
       For now, it's a lose. */
    dbref who;
    DOLIST(who, GetContents(where)) {
	if ((Typeof(who) == TYPE_PLAYER) && (who != except)) {
	    send_player(who, ptr, len);
	}
    }
#endif

    send_except(where, except, ptr, len);
}


/* returns true for numbers of form [ + | - ] <series of digits> */
int number(const char *s)
{
    if (!s) return 0;

    while (*s && isspace(*s)) s++;

    if (*s == '+' || *s == '-') s++;

    if (!*s || !isdigit(*s)) return 0;

    while (*s && isdigit(*s)) s++;
    while (*s && isspace(*s)) s++;

    if (*s) return 0;
    else return 1;
}

