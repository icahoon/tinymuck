/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* frames.c,v 2.2 1993/12/15 00:41:26 dmoore Exp */
#include "config.h"

#include "db.h"
#include "params.h"
#include "externs.h"


struct frame_entry {
    dbref player;		/* Person running a player. */
    struct frame *interactive;	/* Interactive frame. */
    /* background field totally unused currently. */
    struct frame *background;	/* List of background frames. */
};


/* Hash table of players currently being edited. */
static hash_tab *frame_table = NULL;


/* Hash table routines.  Maps a player # -> frames. */
static unsigned long hash_frame_entry(const void *entry)
{
    const struct frame_entry *info = entry;

    return dbref2long(info->player);
}


static int compare_frame_entries(const void *entry1, const void *entry2)
{
    const struct frame_entry *info1 = entry1;
    const struct frame_entry *info2 = entry2;

    return (info1->player != info2->player);
}


static void delete_frame_entry(void *entry)
{
    struct frame_entry *info = entry;

    FREE(info);
}


void init_frame_table(void)
{
    if (frame_table) return;

    frame_table = init_hash_table("Frame table", compare_frame_entries,
			     hash_frame_entry, delete_frame_entry,
			     FRAME_HASH_SIZE);
}


void clear_frame_table(void)
{
    clear_hash_table(frame_table);
    frame_table = NULL;
}


struct frame *get_frame(const dbref player)
{
    struct frame_entry match;
    const struct frame_entry *result;

    match.player = player;
    result = find_hash_entry(frame_table, &match);
    
    if (!result) return NULL;

    return result->interactive;
}


int running_program(const dbref player)
{
    return (get_frame(player) != NULL);
}


void add_frame(const dbref player, struct frame *frame)
{
    struct frame_entry *info;
    
    MALLOC(info, struct frame_entry, 1);

    info->player = player;
    info->interactive = frame;
    info->background = NULL;
    add_hash_entry(frame_table, info);
}


void delete_frame(const dbref player)
{
    struct frame_entry match;

    match.player = player;
    clear_hash_entry(frame_table, &match);
}

