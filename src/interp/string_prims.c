/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* string_prims.c,v 2.5 1997/08/29 21:02:08 dmoore Exp */
#include "config.h"

#include <string.h>

#include "db.h"
#include "code.h"
#include "prim_offsets.h"
#include "buffer.h"
#include "externs.h"


/* Use this to get shared strings in a nice format. */
#define SSText(x)	((x)->un.string ? (x)->un.string->data : "")
#define SSLen(x)	((x)->un.string ? (x)->un.string->length : 0)

/* All of your basic string primitives (including notifies):
   pronoun_sub, explode, subst, instr, rinstr, number, stringcmp,
   strcmp, strncmp, strcut, strlen, strcat, atoi, intostr
   notify, notify_except */

static Buffer buf;		/* Global buffer for all routines to use. */


MUF_PRIM(prim_pronoun)
{
    inst format;
    inst who;
    inst result;

    pop_stack(curr_data_st, &format);
    pop_stack(curr_data_st, &who);

    Bufsprint(&buf, "%p", who.un.object, SSText(&format));

    result.type = INST_STRING;
    result.un.string = make_shared_string(Buftext(&buf), Buflen(&buf));

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_explode)
{
    inst str1, str2;
    inst temp;
    const char *pat;
    char *orig;
    int origlen, patlen;
    int pos;
    int count;

    pop_stack(curr_data_st, &str2);
    pop_stack(curr_data_st, &str1);

    count = 0;
    temp.type = INST_STRING;

    Bufcpy(&buf, SSText(&str1));
    orig = Buftext(&buf);
    origlen = Buflen(&buf);
    pat = SSText(&str2);
    patlen = SSLen(&str2);

    while ((pos = muck_rinstr(orig, origlen, pat, patlen))) {
	pos--;

	temp.un.string = make_shared_string(orig + pos + patlen,
					    origlen - pos - patlen);
	
	orig[pos] = '\0';
	origlen = pos;

	safe_push_data_st(curr_frame, curr_data_st, &temp, PRIM_explode);
	if (curr_frame->error) return;

	count++;
    }

    temp.un.string = make_shared_string(orig, origlen);
	    
    safe_push_data_st(curr_frame, curr_data_st, &temp, PRIM_explode);
    if (curr_frame->error) return;
	    
    count++;

    temp.type = INST_INTEGER;
    temp.un.integer = count;
    
    safe_push_data_st(curr_frame, curr_data_st, &temp, PRIM_explode);
}



MUF_PRIM(prim_subst)
{
    inst str, orig, repl;
    inst result;
    const char *subbed;
    int len;

    pop_stack(curr_data_st, &orig);
    pop_stack(curr_data_st, &repl);
    pop_stack(curr_data_st, &str);

    subbed = muck_subst(SSText(&str), SSText(&orig), SSLen(&orig),
			SSText(&repl), &len);

    result.type = INST_STRING;
    result.un.string = make_shared_string(subbed, len);

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_instr)
{
    inst str, pattern;
    inst result;

    pop_stack(curr_data_st, &pattern);
    pop_stack(curr_data_st, &str);
   
    result.type = INST_INTEGER;
    result.un.integer = muck_instr(SSText(&str), SSLen(&str),
				   SSText(&pattern), SSLen(&pattern));

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_rinstr)
{
    inst str, pattern;
    inst result;

    pop_stack(curr_data_st, &pattern);
    pop_stack(curr_data_st, &str);

    result.type = INST_INTEGER;
    result.un.integer = muck_rinstr(SSText(&str), SSLen(&str),
				    SSText(&pattern), SSLen(&pattern));
   
    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_numberp)
{
    inst str;
    inst result;

    pop_stack(curr_data_st, &str);

    result.type = INST_INTEGER;
    result.un.integer = number(SSText(&str));

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_stringcmp)
{
    inst arg1, arg2;
    inst result;

    pop_stack(curr_data_st, &arg2);
    pop_stack(curr_data_st, &arg1);

    result.type = INST_INTEGER;
    result.un.integer = muck_stricmp(SSText(&arg1), SSText(&arg2));

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_strcmp)
{
    inst arg1, arg2;
    inst result;

    pop_stack(curr_data_st, &arg2);
    pop_stack(curr_data_st, &arg1);

    result.type = INST_INTEGER;
    result.un.integer = strcmp(SSText(&arg1), SSText(&arg2));

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_strncmp)
{
    inst str1, str2, size;
    inst result;

    pop_stack(curr_data_st, &size);
    pop_stack(curr_data_st, &str2);
    pop_stack(curr_data_st, &str1);

    result.type = INST_INTEGER;
    result.un.integer = strncmp(SSText(&str1), SSText(&str2), size.un.integer);

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_stringncmp)
{
    inst str1, str2, size;
    inst result;

    pop_stack(curr_data_st, &size);
    pop_stack(curr_data_st, &str2);
    pop_stack(curr_data_st, &str1);

    result.type = INST_INTEGER;
    result.un.integer = muck_strnicmp(SSText(&str1), SSText(&str2),
				      size.un.integer);

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_strcut)
{
    inst str, n;
    inst result1, result2;

    pop_stack(curr_data_st, &n);
    pop_stack(curr_data_st, &str);

    if (n.un.integer < 0) {
	interp_error(curr_frame, PRIM_strcut, "Non-negative arg needed");
	return;
    }

    result1.type = INST_STRING;
    result2.type = INST_STRING;

    if (!str.un.string) {
	result1.un.string = NULL;
	result2.un.string = NULL;
    } else if (n.un.integer > SSLen(&str)) {
	result1.un.string = dup_shared_string(str.un.string);
	result2.un.string = NULL;
    } else {
	Bufncpy(&buf, SSText(&str), n.un.integer);
	result1.un.string = make_shared_string(Buftext(&buf), Buflen(&buf));
	
	result2.un.string = make_shared_string(SSText(&str) + n.un.integer,
					       SSLen(&str) - n.un.integer);
    }

    push_stack(curr_data_st, &result1);
    push_stack(curr_data_st, &result2);
}


MUF_PRIM(prim_strlen)
{
    inst str;
    inst result;

    pop_stack(curr_data_st, &str);

    result.type = INST_INTEGER;
    result.un.integer = SSLen(&str);

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_strcat)
{
    inst str1, str2;
    inst result;

    pop_stack(curr_data_st, &str2);
    pop_stack(curr_data_st, &str1);
    

    result.type = INST_STRING;
    if (!str1.un.string) {
	result.un.string = dup_shared_string(str2.un.string);
    } else if (!str2.un.string) {
	result.un.string = dup_shared_string(str1.un.string);
    } else {
	result.un.string = make2_shared_string(SSText(&str1), SSLen(&str1),
					       SSText(&str2), SSLen(&str2));
    }

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_atoi)
{
    inst str;
    inst result;

    pop_stack(curr_data_st, &str);

    result.type = INST_INTEGER;
    result.un.integer = atoi(SSText(&str));

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_intostr)
{
    inst arg, result;
    Buffer buf;

    pop_stack(curr_data_st, &arg);

    switch (arg.type) {
    case INST_INTEGER:
	Bufsprint(&buf, "%i", arg.un.integer);
	break;
    case INST_OBJECT:
	Bufsprint(&buf, "%d", arg.un.object);
	break;
    default:
	interp_error(curr_frame, PRIM_intostr, "Requires integer or object");
	return;
	break;
    }

    result.type = INST_STRING;
    result.un.string = make_shared_string(Buftext(&buf), Buflen(&buf));

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_strftime)
{
    inst when, format;
    inst result;

    pop_stack(curr_data_st, &format);
    pop_stack(curr_data_st, &when);

    result.type = INST_STRING;
    if (format.un.string) {
	Bufsprint(&buf, "%t", SSText(&format), when.un.integer);
	result.un.string = make_shared_string(Buftext(&buf), Buflen(&buf));
    } else {
	result.un.string = NULL;
    }

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_notify)
{
    inst who, message;

    /* Pops are guaranteed safe before calling.  And types checked. */
    pop_stack(curr_data_st, &message);
    pop_stack(curr_data_st, &who);

    if (message.un.string) {
	notify(who.un.object, "%s", message.un.string->data);
    }
}


MUF_PRIM(prim_notify_except)
{
    inst loc;
    inst who_not;
    inst message;

    pop_stack(curr_data_st, &message);
    pop_stack(curr_data_st, &who_not);
    pop_stack(curr_data_st, &loc);

    if (message.un.string) {
	notify_except(loc.un.object, who_not.un.object, "%s",
		      message.un.string->data);
    }
}

