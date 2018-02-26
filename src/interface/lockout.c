/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* lockout.c,v 2.9 1994/03/10 04:14:04 dmoore Exp */
#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "db.h"
#include "buffer.h"
#include "lockout.h"
#include "externs.h"

/* Written by dmoore on 2/26/92.  Idea for being able to force site
   specific creation due to Jiro. */

static void load_lockout(void);
static int lockout_match(const char *, const char *);


#define LOCK_CONN	1
#define LOCK_CREATE	2
#define LOCK_PLAYER	3

typedef struct site_node {
    const char *site_name;	/* Name of site to lock out. */
    unsigned int kind : 2;	/* Lockout any connection or just creation. */
    unsigned int allow : 1;	/* 1 if this connection allowed, else 0. */
    const char *player_pattern; /* Pattern of dbref for player entries. */
    struct site_node *next;	/* Linked list. */
} site_node;


static site_node *sites = NULL;
static int sites_loaded = 0;


/* This routine is exported out to allow the USR2 signal handler to force
   a reload of the lockout file. */
void reload_lockout(const int signum)
{
    sites_loaded = 0;
    signal(signum, reload_lockout);
}


/* Deallocates any memory used by the lockout code. */
void free_lockout(void)
{
    site_node *temp;

    /* Free up any existing sites. */
    while (sites) {
	temp = sites->next;
	FREE_STRING(sites->site_name);
	if (sites->player_pattern) FREE_STRING(sites->player_pattern);
	FREE(sites);
	sites = temp;
    }
    sites = NULL;
}


/* This loads up the lockout file, making a linked list of sites
   mentioned. */
static void load_lockout(void)
{
    Buffer buf, site_name;
    site_node *temp;
    FILE *fp;
    char *p, *q, *player_pattern;
    int kind;
    int allow;

    free_lockout();		/* Clean up any existing sites. */

    sites_loaded = 1;

    /* Read in the new file.  Each line should be an address followed
       by whitespace followed by either 'connection' or 'creation'. */
    fp = fopen(LOCKOUT_FILE, "r");
    if (!fp) return;

    while (Bufgets(&buf, fp, NULL)) {
	p = Buftext(&buf);

	/* Skip any leading white space. */
	while (*p && isspace(*p)) p++;

	/* First char is ; or line is empty, skip. */
	if (!*p || *p == ';') continue;

	/* Skip to the end of the hostname. */
	for (q = p; *q && !isspace(*q); q++);

	/* Error if we're at the end of the string already. */
	if (!*q) {
	    log_status("LOCKOUT: bad entry (host) %s", Buftext(&buf));
	    continue;
	}

	/* Slip in a NUL, and use this string to get the host address. */
	*q = '\0';
	Bufcpy(&site_name, p);
	*q = ' ';		/* We know that it was whitespace. */

	/* Skip the white space. */
	for (p = q; *p && isspace(*p); p++);

	/* Skip to the end of the type, and slip in a NUL. */
	for (q = p; *q && !isspace(*q); q++);
	*q = '\0';

	/* Check for ! symbol. */
	if (*p == '!') { 
	    allow = 0;
	    p++;
	} else {
	    allow = 1;
	}

	/* Check out the specifier. */
	if (!strcmp(p, "connection")) {
	    kind = LOCK_CONN;
	} else if (!strcmp(p, "creation")) {
	    kind = LOCK_CREATE;
	} else if (*p == '#') {
	    kind = LOCK_PLAYER;
	    player_pattern = p + 1;
	} else {
	    log_status("LOCKOUT: bad entry (kind) %s", Buftext(&buf));
	    continue;
	}

	/* We got to here so this entry is fine.  Make a new node, and link
	   it into the list. */
	MALLOC(temp, struct site_node, 1);
	temp->site_name = ALLOC_STRING(Buftext(&site_name));
	temp->kind = kind;
	temp->allow = allow;
	if (kind == LOCK_PLAYER) {
	    temp->player_pattern = ALLOC_STRING(player_pattern);
	} else {
	    temp->player_pattern = NULL;
	}
	temp->next = sites;
	sites = temp;

    }
    fclose(fp);
}


/* This routine checks whether a given site is allowed to connect to the
   muck. */
int ok_conn_site(const char *hostname)
{
    site_node *temp;

    if (!sites_loaded) load_lockout();

    for (temp = sites; temp; temp = temp->next) {
	if ((temp->kind == LOCK_CONN)
	    && (lockout_match(temp->site_name, hostname))) 
	    return temp->allow;
    }

    return 1;
}


/* This routine checks whether a given site is allowed to create new
   characters on the muck. */
int ok_create_site(const char *hostname)
{
    site_node *temp;

    if (!sites_loaded) load_lockout();

    for (temp = sites; temp; temp = temp->next) {
	if ((temp->kind == LOCK_CREATE)
	    && (lockout_match(temp->site_name, hostname)))
	    return temp->allow;
    }

    return 1;
}


/* This routines checks that this site is allowed to connect to a
   specific character. */
int ok_player_site(const char *hostname, const dbref who_dbref)
{
    Buffer who;
    site_node *temp;

    if (!sites_loaded) load_lockout();

    /* Turn the dbref into a string for comparison. */
    Bufsprint(&who, "%d", who_dbref);

    for (temp = sites; temp; temp = temp->next) {
	if ((temp->kind == LOCK_PLAYER)
	    && (lockout_match(temp->site_name, hostname))
	    && (lockout_match(temp->player_pattern, Buftext(&who))))
	    return temp->allow;
    }

    return 1;
}


/* Match a pattern which might contain * for wildcard against the string. */
/* Return 1 if matched, 0 otherwise. */
static int lockout_match(const char *pattern, const char *str)
{
    if ((!pattern) || (!str)) return 0;
    while (*pattern) {
	if (*pattern != '*') {
	    if (tolower(*pattern) != tolower(*str)) return 0;
	    pattern++;
	    str++;
	} else {
	    while (*pattern == '*') pattern++;
	    if (!*pattern) return 1;
	    while (str = strchr(str, *pattern)) {
		if (lockout_match(pattern, str)) return 1;
		str++;
	    }
	    return 0;
	}
    }
    /* Only matched if we got to the end of both strings. */
    if (*str) return 0;
    else return 1;
}



