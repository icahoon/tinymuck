/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* hostname.c,v 2.11 1996/12/23 22:15:19 dmoore Exp */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "db.h"
#include "buffer.h"
#include "hostname.h"
#include "params.h"
#include "externs.h"

/* Number of seconds to add to delay each time a consecutive name server
   failure occurs. */
#define TIME_TO_DELAY 300
#define MAX_TIME_TO_DELAY 900

struct host_info {
    ip_addr_t ip_address;	/* Actually address. */
    const char *name;		/* Hostname for address. */
};


/* last_checked is only used if a lookup for a given host doesn't complete.
   The host won't be attempted to be looked up again for timeout seconds. */
static time_t last_checked;
static int timeout = 0;
static hash_tab *hostname_table = NULL; /* Table mapping addresses to names. */
static int hosts_loaded = 0;	/* 0 if need to reload from file. */


static unsigned long hash_host_entry(const void *entry)
{
    return ((const struct host_info *) entry)->ip_address;
}


static int compare_host_entries(const void *entry1, const void *entry2)
{
    const struct host_info *host1 = entry1;
    const struct host_info *host2 = entry2;

    return ((host1->ip_address) != (host2->ip_address));
}


static void delete_host_entry(void *entry)
{
    struct host_info *host = entry;
    
    if (!host) return;

    if (host->name) FREE_STRING(host->name);
    FREE(host);
}


const char *lookup_host(ip_addr_t addr, int try_nameservice)
{
    struct host_info match;
    struct host_info *result;
    struct hostent *he;
    time_t now;
    static Buffer buf;

#ifndef HOSTNAMES
    try_nameservice = HOSTNAME_NO_RESOLVE;
#endif

    if (!hosts_loaded)
	init_hostnames();

    match.ip_address = addr;

    result = find_hash_entry(hostname_table, &match);
    if (result) {
	/* Found a solution. */
	return result->name;
    }

    /* No entry stored in table.  If hostname resolution is desired, then
       try that now. */

    if (try_nameservice) {
	/* Check that enough time has elasped since last lookup.  In case
	   the nameserver appears to be down. */

	now = time(NULL);
	if (difftime(now, last_checked) > timeout) {
	    last_checked = now;
	    timeout += TIME_TO_DELAY;
	    if (timeout > MAX_TIME_TO_DELAY)
		timeout = MAX_TIME_TO_DELAY;

	    /* Hmm, if there is no name for a given address we
	       treat it as a failure, but there might just not
	       be a name for it.  Look into checking herror. */
	    
	    he = gethostbyaddr(&addr, sizeof(addr), AF_INET);
	    if (he && he->h_name) {
		/* Looked up successfully, store it in the table, and return
		   the result. */
	    
		MALLOC(result, struct host_info, 1);
		result->ip_address = addr;
		result->name = ALLOC_STRING(he->h_name);
		
		add_hash_entry(hostname_table, result);
	    
		timeout = 0;
		return result->name;
	    }
	}
    }

    /* Here if lookup failed any place above. */
    addr = ntohl(addr);
    Bufsprint(&buf, "%d.%d.%d.%d", (addr >> 24) & 0xff, (addr >> 16) & 0xff,
            (addr >> 8) & 0xff, addr & 0xff);
    return Buftext(&buf);
    
}


void init_hostnames(void)
{
    last_checked = time(NULL);
    timeout = 0;

    if (!hostname_table) {
	hostname_table = init_hash_table("Hostname table",
					 compare_host_entries,
					 hash_host_entry,
					 delete_host_entry,
					 HOST_HASH_SIZE);
    }

    hosts_loaded = 1;

    {
	/* Stick in sdnp1/2 for now. :) */
	struct host_info *temp;
	
	MALLOC(temp, struct host_info, 1);
	temp->ip_address = htonl(2230281190UL);
	temp->name = ALLOC_STRING("OJ's.machine");
	add_hash_entry(hostname_table, temp);
	
	MALLOC(temp, struct host_info, 1);
	temp->ip_address = htonl(2230275850UL);
	temp->name = ALLOC_STRING("OJ's.machine");
	add_hash_entry(hostname_table, temp);
    }

}


void clear_hostnames(void)
{
    clear_hash_table(hostname_table);
    hostname_table = NULL;
    hosts_loaded = 0;
}
