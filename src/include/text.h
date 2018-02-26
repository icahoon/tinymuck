/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* text.h,v 2.3 1993/04/08 20:24:46 dmoore Exp */
#ifndef MUCK_TEXT_H
#define MUCK_TEXT_H

#define TEXT_INSERT_END -1
#define TEXT_INSERT_FRONT 0
#define TEXT_FIRST_LINE 1

struct text;

void free_text(struct text *);
void write_text(const char *, struct text *);
struct text *make_new_text(void);
struct text *read_text(const char *);
void insert_text(struct text *, const char *, const int, int);
void insert2_text(struct text *, const char *, const int, const char *, const int, int);
void flush_text(struct text *, int, int);
void delete_text(struct text *, int, int);
const char *text_line(struct text *, int, int *);
int text_total_lines(struct text *);
int text_total_bytes(struct text *);

#endif /* MUCK_TEXT_H */
