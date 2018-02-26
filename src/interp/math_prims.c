/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* math_prims.c,v 2.4 1997/08/29 21:02:05 dmoore Exp */
#include "config.h"

#include "ansify.h"
#include "db.h"
#include "code.h"
#include "prim_offsets.h"
#include "muck_time.h"
#include "externs.h"

/* Primitives contained in this file: +, -, *, /, %, <, >, <=, >=, =,
   random, systime, time */

/* Useful macro to generate the functions for these primitives, since
   they all take the same form, essentially.  Good for all except / and %. */

#define MATH_PRIM(NAME, OP) \
    MUF_PRIM(prim_##NAME) \
    { inst arg1, arg2; \
      pop_stack(curr_data_st, &arg2); pop_stack(curr_data_st, &arg1); \
      arg1.un.integer = arg1.un.integer OP arg2.un.integer; \
      push_stack(curr_data_st, &arg1); }

MATH_PRIM(add, +)
MATH_PRIM(sub, -)
MATH_PRIM(mul, *)
MATH_PRIM(lessthan, <)
MATH_PRIM(gtrthan, >)
MATH_PRIM(lesseq, <=)
MATH_PRIM(gtreq, >=)
MATH_PRIM(equal, ==)


/* This version checks the second arg to be 0 first. */
#define MATH0_PRIM(NAME, OP) \
    MUF_PRIM(prim_##NAME) \
    { inst arg1, arg2; \
      pop_stack(curr_data_st, &arg2); pop_stack(curr_data_st, &arg1); \
      if (arg2.un.integer) \
	  arg1.un.integer = arg1.un.integer OP arg2.un.integer; \
      else arg1.un.integer = 0; \
      push_stack(curr_data_st, &arg1); }

MATH0_PRIM(div, /)
MATH0_PRIM(mod, %)


MUF_PRIM(prim_random)
{
    inst result;

    result.type = INST_INTEGER;
    result.un.integer = random();

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_systime)
{
    inst result;

    result.type = INST_INTEGER;
    result.un.integer = muck_time();

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_time)
{
    inst result;
    time_t tim;
    struct tm *tm;

    tim = time(NULL);
    tm = localtime(&tim);

    result.type = INST_INTEGER;

    result.un.integer = tm->tm_sec;
    push_stack(curr_data_st, &result);

    result.un.integer = tm->tm_min;
    push_stack(curr_data_st, &result);

    result.un.integer = tm->tm_hour;
    push_stack(curr_data_st, &result);
}

