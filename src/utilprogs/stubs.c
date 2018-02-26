/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* stubs.c,v 2.3 1997/08/09 23:51:58 dmoore Exp */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#include "db.h"
#include "buffer.h"
#include "externs.h"

/* This file has some stub routines which aren't included if you
   don't link in the whole server, such as convert and sanity. */

Buffer match_args;

void panic(const char *s)
{
    fprintf(stderr, "%s", s);
    abort();
}

void compile_text(const dbref i, const dbref j, struct text *t) {}
void free_prog(const dbref i) {}
void clear_code(struct code *x) {}
int interp(const dbref player, const dbref program, const dbref trigger, const int nothread, const char *arg_text) { return 0; }
int running_program(const dbref player) { return 0; }
void add_player(const dbref i) {}
int send_player(const dbref i, const char *x, const int n) { return 0; }
void send_except(const dbref i, const dbref j, const char *x, const int n) {}
void send_all(const char *x, const int n, const int y) {}
dbref do_match_boolexp(const dbref player, const char *what) { return NOTHING; }
void move_object(const dbref player, dbref loc, const dbref exit) {}
int can_see_flags(const dbref i, const dbref j) { return 1; }
void edit_quit_recycle(const dbref i) {}
void delete_code(const dbref i) {}
void delete_frame(const dbref i) {}
