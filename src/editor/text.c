/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* text.c,v 2.7 1996/01/26 01:01:39 dmoore Exp */
#include "config.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "db.h"
#include "buffer.h"
#include "text.h"
#include "externs.h"

struct line {
    int length;			/* Length of line. */
    const char *data;		/* Body of line. */
    struct line *next;
    struct line *prev;
};

struct text {
    struct line *first;		/* First line of text. */
    struct line *last;		/* Quicker tail insert. */
    struct line *curr;		/* Compiler and editor for quick insert. */
    int total_lines;		/* Quicker tail insert. */
    int curr_line;		/* Compiler and editor for quick insert. */
    int nbytes;			/* Total number of bytes contained. */
};


static void free_line(struct line *line)
{
    if (line->data)
	FREE_CNT_STRING(line->data, line->length);
    FREE(line);
}


void free_text(struct text *text)
{
    struct line *temp;
    struct line *next;
    
    if (!text) return;

    for (temp = text->first; temp; temp = next) {
	next = temp->next;
	free_line(temp);
    }
   
    FREE(text);
}


void write_text(const char *fname, struct text *text)
{
    const struct line *temp;
    FILE *f;
    
    f = fopen(fname, "w");
    if (!f) {
	log_status("Couldn't open file (%s) for write_text.", fname);
	return;
    }
    
    temp = text ? text->first : NULL;
    for ( ; temp; temp = temp->next) {
	if (temp->data) {
	    fputs(temp->data, f);
	}
	fputc('\n', f);
    }
    fclose(f);
}


struct text *make_new_text(void)
{
    struct text *text;

    MALLOC(text, struct text, 1);
    text->first = NULL;
    text->last = NULL;
    text->curr = NULL;
    text->total_lines = 0;
    text->curr_line = 0;
    text->nbytes = 0;
    
    return text;
}


static struct line *make_new_line(const char *data, const int len)
{
    struct line *new;
    
    MALLOC(new, struct line, 1);
    new->length = len;
    new->data = ALLOC_CNT_STRING(data, len);
    new->next = NULL;
    new->prev = NULL;
    return new;
}


static struct line *make2_new_line(const char *data1, const int len1, const char *data2, const int len2)
{
    struct line *new;
    
    MALLOC(new, struct line, 1);
    new->length = len1 + len2;
    new->data = ALLOC_CNT2_STRING(data1, len1, data2, len2);
    new->next = NULL;
    new->prev = NULL;
    return new;
}


struct text *read_text(const char *fname)
{
    struct text *text;
    struct line *new;
    FILE *fp;
    static Buffer buf;

    text = make_new_text();

    if (fname && *fname) {
	/* If there is a file name, try to read in the associated file. */
	fp = fopen(fname, "r");
	if (fp) {
	    while (Bufgets(&buf, fp, NULL)) {
		text->total_lines++;
		new = make_new_line(Buftext(&buf), Buflen(&buf));
		text->nbytes += new->length;
		if (!text->first) {
		    text->first = text->curr = text->last = new;
		    text->curr_line = 1;
		} else {
		    text->last->next = new;
		    new->prev = text->last;
		    text->last = new;

		    /* Nice idea, every other line move curr up a spot. */
		    /* This will leave it in the middle when we are done. */
		    if (text->total_lines % 2) {
			text->curr = text->curr->next;
			text->curr_line++;
		    }
		}
	    }
	    fclose(fp);
	}
    }

    return text;
}


/* Text lines are numbered like so:
   1 - All little chupchup's must learn to
   2 - not go out and play in Storm.
   3 - Whee.
   Ranging from 1 to N. */
/* Additionally, negative entries range backwards from the end:
   -3 - All little chupchup's must learn to
   -2 - not go out and play in Storm.
   -1 - Tomato. */
static int translate_line(struct text *text, int where)
{

    if (!text) return 0;

    if (where < 0)
	where = text->total_lines + where + 1;

    return where;
}


/* This routine has a side effect of updating the current line pointer
   for the text.  If the text has no lines, NULL is returned. */
static struct line *find_line(struct text *text, int where)
{
    register int dist;
    register struct line *result;

    if (!text) return NULL;

    if (!text->curr) {
	/* Current not initialized yet. */
	text->curr = text->first;
	text->curr_line = 1;
    }

    if (where < 1 || where > text->total_lines)
	return NULL;

    if (where == text->curr_line + 1) {
	/* Immediate next line.  Most common case for editting and
	   compiling. */
	result = text->curr->next;
	where = text->curr_line + 1;
    } else if (where == 1) {
	/* First line. */
	result = text->first;
	where = 1;
    } else if (where == text->total_lines) {
	/* Last line. */
	result = text->last;
	where = text->total_lines;
    } else if (where == text->curr_line) {
	/* Current line. */
	result = text->curr;
    } else {
	if (where < text->curr_line) {
	    /* Between 1st and current line. */
	    if ((where - 1) <= (text->curr_line - where)) {
		/* Closer to 1st line. */
		result = text->first;
		for (dist = (where - 1); dist; dist--)
		    result = result->next;
	    } else {
		/* Closer to current line. */
		result = text->curr;
		for (dist = (text->curr_line - where); dist; dist--)
		    result = result->prev;
	    }
	} else {
	    /* Between current and last line. */
	    if ((where - text->curr_line) <= (text->total_lines - where)) {
		/* Closer to current line. */
		result = text->curr;
		for (dist = (where - text->curr_line); dist; dist--)
		    result = result->next;
	    } else {
		/* Closer to last line. */
		result = text->last;
		for (dist = (text->total_lines - where); dist; dist--)
		    result = result->prev;
	    }
	}
    }

    text->curr = result;
    text->curr_line = where;

    return result;
}


static int insert_line_internal(struct text *text, struct line *line, int where)
{
    struct line *pos;

    if (!text || !line) return 0;

    where = translate_line(text, where); /* Check for -N notation. */

    if (!text->first) {
	/* First line in text. */
	text->first = text->last = line;
	where = 1;
    } else if (where == 0) {
	/* Prepend to the front. */
	line->next = text->first;
	text->first->prev = line;
	text->first = line;
	where = 1;
    } else {
	pos = find_line(text, where);
	if (!pos) return 0;	/* Bad line to insert at. */

	/* Insert after this line, and check if real end. */
	line->next = pos->next;
	line->prev = pos;
	pos->next = line;
	if (line->next)
	    line->next->prev = line;
	else text->last = line;

	where = text->curr_line + 1;
    }

    /* Increment the # of lines, and make the current line be this one. */
    text->nbytes += line->length;
    text->total_lines++;
    text->curr_line = where;
    text->curr = line;

    return 1;			/* All ok. */
}


void insert_text(struct text *text, const char *data, const int len, int where)
{
    struct line *line;

    if (!text) return;

    line = make_new_line(data, len);

    if (!insert_line_internal(text, line, where)) {
	/* Unable to insert, must have been bad line number. */
	free_line(line);
    }
}


void insert2_text(struct text *text, const char *data1, const int len1, const char *data2, const int len2, int where)
{
    struct line *line;

    if (!text) return;

    line = make2_new_line(data1, len1, data2, len2);

    if (!insert_line_internal(text, line, where)) {
	/* Unable to insert, must have been bad line number. */
	free_line(line);
    }
}


/* Delete out from given line to line. */
static void delete_lines_internal(struct text *text, const int start_pos, struct line *start, struct line *end)
{
    struct line *prev;
    struct line *next;
    struct line *temp;

    if (!start || !end) return;

    /* Just in front of where we started deleting, and just after. */
    prev = start->prev;
    next = end->next;

    /* Now clean up the first and last pointers. */
    if (prev == NULL) text->first = next;
    else prev->next = next;
    if (next == NULL) text->last = prev;
    else next->prev = prev;

    /* Setup the current line pointer to just after deleted lines.  If
       deletion went to end of text, just leave current at NULL and let
       it get moved on next access. */
    text->curr = next;
    text->curr_line = next ? start_pos : 0;

    /* Free up the lines. */
    end->next = NULL;
    for (temp = start; temp; temp = next) {
	next = temp->next;
	text->nbytes -= temp->length; /* Subtract off bytes from total. */
	text->total_lines--;
	free_line(temp);
    }
}

/* Flush lines until at least amount bytes have been flushed, or the end has
   been reached. */
void flush_text(struct text *text, int where, int amount)
{
    struct line *start, *end;

    if (amount <= 0) return;

    where = translate_line(text, where);

    start = find_line(text, where);

    for (end = start; end && (amount > 0); end = end->next) {
	amount -= end->length;
    }

    delete_lines_internal(text, where, start, end);
}


/* Delete lines from start to end inclusive. */
void delete_text(struct text *text, int start_pos, int end_pos)
{
    struct line *start, *end;

    if (!text) return;

    start_pos = translate_line(text, start_pos); /* Check for -N notation. */
    end_pos = translate_line(text, end_pos); /* Check for -N notation. */

    if (start_pos > end_pos) return;

    /* Get pointers to the first and last lines to delete. */
    start = find_line(text, start_pos);
    end = find_line(text, end_pos);

    delete_lines_internal(text, start_pos, start, end);
}


const char *text_line(struct text *text, int where, int *length)
{
    struct line *line;

    if (!text) return NULL;

    where = translate_line(text, where); /* Check for -N. */
    line = find_line(text, where);

    if (line) {
	if (length) *length = line->length;
	return line->data;
    } else {
	if (length) *length = 0;
	return NULL;
    }
}


int text_total_lines(struct text *text)
{
    return text ? text->total_lines : 0;
}


int text_total_bytes(struct text *text)
{
    return text ? text->nbytes : 0;
}

