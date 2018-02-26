/* Copyright (c) 1992 by David Moore.  All rights reserved. */
#include "copyright.h"
/* interface.c,v 2.28 1997/08/16 22:31:43 dmoore Exp */

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
#include <sys/param.h>
#include <netinet/in.h>

#if defined(_AIX) || defined(___AIX)
#include <sys/select.h>
#endif

#include "ansify.h"
#include "db.h"
#include "buffer.h"
#include "text.h"
#include "hostname.h"
#include "muf_con.h"
#include "params.h"
#include "externs.h"



/* Only things in this file should know what descriptor_info has in it,
   and generally everybody should talk to it via the provided interface.
   Namely: check_awake, connected_players, send_player, send_all,
   boot_off, emergency_shutdown, normal_shutdown, dump_status,
   set_up_socket, main_loop. */

/* Have a hash table by dbref of connected players??  This is a pain only
   because of multiply connected players. */
/* fd based searches are always by means of going down the linked list. */

/* Perhaps add in things to count total # of lines sent, total # of
   bytes sent.  Total # of lines received, # bytes received, etc.
   Other useful statistical information like that. */
struct descriptor_info {
    dbref player;		/* dbref of player, NOTHING if not connected */
    int descriptor;		/* fd of the socket connection */

    unsigned int close_down : 1; /* True if this connection should be closed */
    unsigned int output_flushed : 1; /* True if the output flushed. */

    time_t connect_time;	/* connect time of player */
    time_t last_time;		/* last time input received */

    ip_addr_t hostip;		/* address of the player */

    const char *output_prefix;	/* OUTPUTPREFIX */
    const char *output_suffix;	/* OUTPUTSUFFIX */

    struct text *input_queue;	/* lines from player */
    Buffer *input_buffer;	/* buffer for partial input line */
    int quota;			/* number of lines player gets this slice */
    int ed_quota;		/* number of lines when in editor */

    struct text *output_queue;	/* lines to player */
    const char *output_pos;	/* pointer to partially sent output line */

    struct descriptor_info *next;
    struct descriptor_info **prev;
};


int shutdown_flag = 0;
int dump_flag = 0;

static struct descriptor_info *available_descriptors = NULL;
static struct descriptor_info *connections = NULL;
static int ndescriptors;

static const char connect_fail[] = "Either that player does not exist, or has a different password.";
static const char create_fail[] = "Either there is already a player with that name, or that name is illegal.";
/* Used by lockout, if this exact character cannot connect from that site. */
static const char connect_illegal[] = "That character cannot be connected to from that site.";
static const char flushed_message[] = "<Output Flushed>\r\n";
static const char crash_message[] = "\r\nAieeeeee -- splat.\r\n";
static const char shutdown_message[] = "\r\nGoing down -- bye.\r\n";

static void queue_string(struct descriptor_info *, const char *, int);


/* Check size of max_command_len requirements. */
#if (BUFFER_LEN < MAX_COMMAND_LEN)
#error BUFFER_LEN must be >= MAX_COMMAND_LEN
#endif


#ifdef RWHOD
static const char *rwho_uid(const dbref who)
{
    static Buffer buf;

    Bufsprint(&buf, "%d@%s", who, RWHO_NAME);
    return Buftext(&buf);
}


static void rwho_update(void)
{
    struct descriptor_info *d;
    
    rwhocli_pingalive();
    
    for (d = connections; d; d = d->next) {
	if (d->player != NOTHING) {
	    rwhocli_userlogin(rwho_uid(d->player),
			      GetName(d->player), d->connect_time);
	}
    }
}
#endif /* RWHOD */


static void generate_descriptors(long count)
{
    struct descriptor_info *hunk;
    int num_4k;
    int i, j;
 
    /* Try allocating them in 4K sets.  The -4 is for some mallocs
       which want to stick some overhead in the space.  This keeps
       things on a single page (hopefully). */
    num_4k = (4096 - 4) / sizeof(struct descriptor_info);
 
    count = (count / num_4k) + 1;
 
    for (i = 0; i < count; i++) {
        MALLOC(hunk, struct descriptor_info, num_4k);
        for (j = 0; j < num_4k; j++) {
            hunk->next = available_descriptors;
            available_descriptors = hunk;
            hunk++;
        }
    }
}


static struct descriptor_info *make_new_descriptor(void)
{
    struct descriptor_info *result;

    if (!available_descriptors) {
	generate_descriptors(1);
    }
    result = available_descriptors;
    available_descriptors = result->next;

    return result;
}


static void free_descriptor(struct descriptor_info *entry)
{
    entry->next = available_descriptors;
    available_descriptors = entry;
}


static void process_output(struct descriptor_info *d)
{
    int cnt;
    int actual;

    if (d->close_down) return;	/* Marked to close, don't try to send. */

    while (1) {
	/* Loop taking lines off the front and trying to send them.
	   Stop if you run out of lines to send, or a write fails. */

	if (!d->output_pos) {
	    /* No partial line left from last time, fetch a new line! */
	
	    if (d->output_flushed) {
		/* Since output got flushed, sneak the message in on top
		   of the queue now. */
		insert_text(d->output_queue,
			    flushed_message, sizeof(flushed_message)-1,
			    TEXT_INSERT_FRONT);
		d->output_flushed = 0;
	    }

	    d->output_pos = text_line(d->output_queue, TEXT_FIRST_LINE, &cnt);
	} else {
	    /* Recalculate the characters remaining. */
	    const char *start;
	
	    start = text_line(d->output_queue, TEXT_FIRST_LINE, &cnt);
	    cnt -= (d->output_pos - start);
	}

	if (!d->output_pos) return; /* Out of lines to send. */

	/* Try to do the write. */
	actual = write(d->descriptor, d->output_pos, cnt);
	if (actual < 0) {
	    /* Couldn't write anything.  If it's any reason except blocking
	       mark the socket to be closed. */
	    if (errno != EWOULDBLOCK)
		d->close_down = 1;
	    return;
	}

	/* #### should kick out if we get actual=0.  or maybe if it
	   happens twice in a row or something.  basically treat it
	   like EWOULDBLOCK. */

	if (actual == cnt) {
	    /* The line was entirely written.  Delete it off of the
	       front, and fetch the next line. */
	    delete_text(d->output_queue, TEXT_FIRST_LINE, TEXT_FIRST_LINE);
	    d->output_pos = text_line(d->output_queue, TEXT_FIRST_LINE, &cnt);
	} else {
	    /* Partially written line.  Skip past amount really written
	       and decrement amount remaining to write. */
	    d->output_pos += actual;
	    cnt -= actual;
	}
    }
}


/* Checks if a player is connected to the server. */
int check_awake(const dbref player)
{
    /* Jingoro 5/21/91 */
    struct descriptor_info *d;
    int retval = 0;

    for (d = connections; d; d = d->next) {
	if (d->player == player)
	    retval++;
    }
    return retval;
}


/* Returns an array of dbrefs of connected players. */
dbref *connected_players(int *count)
{
    static dbref *who = NULL;
    static int size = 255;	/* Initial size of array, later actual size. */
    
    dbref *temp;
    int number;
    struct descriptor_info *d;

    /* Init the array if needed to the initial size. */
    if (who == NULL) MALLOC(who, dbref, size);
	
    /* First compute the number. */
    for (number = 0, d = connections; d; d = d->next) {
	if (d->player != NOTHING) number++;
    }

    /* Possibly resize array. */
    if (number > size) {
	REALLOC(who, dbref, number);
	size = number;
    }

    /* Fill in the array. */
    for (temp = who, d = connections; d; d = d->next) {
	if (d->player != NOTHING)
	    *temp++ = d->player;
    }
	
    *count = number;
    return who;
}


/* returns an array of descriptors of connected players */
int *connected_cons(int *count)
{
    /* Ben 8/4/92 */
    static int *list = NULL;
    static int size = 255; /* initial size, updated when the array expands */
 
    int *temp;
    int number;
    struct descriptor_info *d;

    if (!list) MALLOC(list, int, size);
 
    for (number = 0, d = connections; d; d = d->next)
        if(d->player != NOTHING)
            number++;
 
    if (number > size) {
	REALLOC(list, int, number);
	size = number;
    }
 
    for (temp = list, d = connections; d; d = d->next)
        if (d->player != NOTHING)
            *temp++ = d->descriptor;
 
    *count = number;
    return list;
}
 
/* fetches the info on connection, translates it to muf_ format */
int con_info(int descr, struct muf_con_info *dest)
{
    /* Ben 8/4/92 */
    struct descriptor_info *d;

    for (d = connections; d; d = d->next)
        if (d->descriptor == descr) {
            dest->player = d->player;
            dest->connect_time = d->connect_time;
	    dest->last_time = d->last_time;
            dest->hostname = lookup_host(d->hostip, HOSTNAME_NO_RESOLVE);
 
            return 1;  /* success! */
        }
 
    return 0;  /* failure */
}

 
/* boots a descriptor */
int boot_off_d(int descr)
{
    /* Ben 8/4/92 */
    struct descriptor_info *d;
 
    for (d = connections; d; d = d->next)
        if (d->descriptor == descr) {
            process_output(d);
            d->close_down = 1;
 
            return 1;  /* success! */
        }
 
    return 0;  /* failure */
}
 

/* Sends a message to the given player. */
int send_player(const dbref player, const char *msg, const int len)
{
    struct descriptor_info *d;
    int retval = 0;

    for (d = connections; d; d = d->next) {
	if (d->player == player) {
	    queue_string(d, msg, len);
	    retval++;
	}
    }
    return retval;
}


/* Send a message to connected players in specified room, unless the
   player is to be skipped. */
void send_except(const dbref where, const dbref except, const char *msg, const int len)
{
    struct descriptor_info *d;

    for (d = connections; d; d = d->next) {
	if ((d->player != NOTHING)
	    && (GetLoc(d->player) == where)
	    && (d->player != except)) {
	    queue_string(d, msg, len);
	}
    }
}


/* Send a message to all connected players.  If important, then try to
   flush the output queue also. */
/* Hmm, maybe important message should even go to non-logged in players? */
void send_all(const char *msg, const int len, const int important)
{
    struct descriptor_info *d;

    for (d = connections; d; d = d->next) {
	/* Skip non-connected connections. */
	if (d->player != NOTHING) {
	    queue_string(d, msg, len);
	    if (important)
		process_output(d); /* Make sure it can be seen. */
	}
    }
}


/* These timeval routines blatantly kept from original tinymud.  Should
   be totally nuked and stuck in a separate timequeue.c file to handle
   all of this stuff in a sane manner. */
static struct timeval timeval_sub(struct timeval now, const struct timeval then)
{
    now.tv_sec -= then.tv_sec;
    now.tv_usec -= then.tv_usec;
    if (now.tv_usec < 0) {
	now.tv_usec += 1000000;
	now.tv_sec--;
    }
    return now;
}


static int msec_diff(const struct timeval now, const struct timeval then)
{
    return ((now.tv_sec - then.tv_sec) * 1000
	    + (now.tv_usec - then.tv_usec) / 1000);
}


static struct timeval timeval_min(const struct timeval time1, const struct timeval time2)
{
    if (msec_diff(time1, time2) < 0)
        return time1;
    else
        return time2;
}


static struct timeval msec_add(struct timeval t, const int x)
{
    t.tv_sec += x / 1000;
    t.tv_usec += (x % 1000) * 1000;
    if (t.tv_usec >= 1000000) {
	t.tv_sec += t.tv_usec / 1000000;
	t.tv_usec = t.tv_usec % 1000000;
    }
    return t;
}


#define get_quota(d) \
    ((d)->player != NOTHING && HasFlag((d)->player, IN_EDITOR) \
     ? (d)->ed_quota : (d)->quota)
#define dec_quota(d) \
    ((d)->player != NOTHING && HasFlag((d)->player, IN_EDITOR) \
     ? (d)->ed_quota-- : (d)->quota--)

static struct timeval update_quotas(struct timeval last, const struct timeval curr)
{
    int nslices;
    struct descriptor_info *d;
    
    nslices = msec_diff(curr, last) / COMMAND_TIME_MSEC;
    
    if (nslices > 0) {
	for (d = connections; d; d = d->next) {
	    d->quota += COMMANDS_PER_TIME * nslices;
	    if (d->quota > COMMAND_BURST_SIZE)
		d->quota = COMMAND_BURST_SIZE;

	    d->ed_quota += COMMANDS_PER_TIME * nslices * 5;
	    if (d->ed_quota > COMMAND_BURST_SIZE * 5)
		d->ed_quota = COMMAND_BURST_SIZE * 5;
	}
    }
    return msec_add(last, nslices * COMMAND_TIME_MSEC);
}


static void welcome_user(struct descriptor_info *d)
{
    FILE *f;
    Buffer buf;
    
    f = fopen(WELC_FILE, "r");

    if (f == NULL) {
	queue_string(d, DEFAULT_WELCOME_MESSAGE,
		     sizeof(DEFAULT_WELCOME_MESSAGE)-1);
	log_status("FILE: %s not found.", WELC_FILE);
    } else {
	while (Bufgets(&buf, f, NULL)) {
	    queue_string(d, Buftext(&buf), Buflen(&buf));
	}
	fclose(f);
    }
}


static struct descriptor_info *initializesock(const int s, ip_addr_t address)
{
    struct descriptor_info *d;
    int fcntl_result = 0;
    
    ndescriptors++;
#ifdef FNONBLOCK
    fcntl_result = fcntl(s, F_SETFL, FNONBLOCK); /* Preferred posix name. */
#else
#ifdef O_NDELAY
    fcntl_result = fcntl(s, F_SETFL, O_NDELAY);	/* Sys5 name. */
#else
    fcntl_result = fcntl(s, F_SETFL, FNDELAY); /* BSD name, wrong for sys5. */
#endif /* O_NDELAY */
#endif /* FNONBLOCK */
    if (fcntl_result == -1) {
	log_status("PERROR: initialsizesock: fcntl -- %s", strerror(errno));
	panic("Nonblocking fcntl failed");
    }

    d = make_new_descriptor();

    d->player = NOTHING;
    d->descriptor = s;

    d->close_down = 0;
    d->output_flushed = 0;

    d->connect_time = time(NULL);
    d->last_time = time(NULL);

    d->hostip = address;

    d->output_prefix = NULL;
    d->output_suffix = NULL;

    d->input_queue = make_new_text();
    d->input_buffer = NULL;
    d->quota = COMMAND_BURST_SIZE;
    d->ed_quota = COMMAND_BURST_SIZE * 5;

    d->output_queue = make_new_text();
    d->output_pos = NULL;

    if (connections)
	connections->prev = &d->next;
    d->next = connections;
    d->prev = &connections;
    connections = d;
    
    welcome_user(d);
    return d;
}


static struct descriptor_info *new_connection(const int sock)
{
    int newsock;
    struct sockaddr_in addr;
    int addr_len;
    ip_addr_t address;
    const char *hostname;
    
    addr_len = sizeof(addr);
    newsock = accept(sock, (struct sockaddr *) &addr, &addr_len);
    if (newsock < 0) {
	return 0;
    } else {
	address = addr.sin_addr.s_addr;
	hostname = lookup_host(address, HOSTNAME_OK_RESOLVE);
	if (!ok_conn_site(hostname)) {
	    log_status("REFUSED: connection from %s(%i) on fd %i.",
		       hostname, ntohs(addr.sin_port), newsock);
	    shutdown(newsock, 2);
	    close(newsock);
	    errno = 0;
	    return 0;
	} else {
	    log_status("ACCEPT: %s(%i) on fd %i.",
		       hostname, ntohs(addr.sin_port), newsock);
	    return initializesock(newsock, address);
	}
    }
}


static void shutdownsock(struct descriptor_info *d)
{
    shutdown(d->descriptor, 2);
    close(d->descriptor);

    if (d->output_prefix) FREE_STRING(d->output_prefix);
    if (d->output_suffix) FREE_STRING(d->output_suffix);

    free_text(d->input_queue);
    if (d->input_buffer) FREE(d->input_buffer);

    free_text(d->output_queue);

    *d->prev = d->next;
    if (d->next)
	d->next->prev = d->prev;
    free_descriptor(d);

    ndescriptors--;
}


static void close_connection(struct descriptor_info *d)
{
    const char *hostname;

    hostname = lookup_host(d->hostip, HOSTNAME_NO_RESOLVE);

    if (d->player == NOTHING) {
	log_status("DISCONNECT: Never connected from %s on fd %i.",
		   hostname, d->descriptor);
    } else {
	log_status("DISCONNECT: %u from %s on fd %i.",
		   d->player, d->player, hostname, d->descriptor);
	do_disconnect(d->player);
#ifdef RWHOD	
	rwhocli_userlogout(rwho_uid(d->player));
#endif
    }

    shutdownsock(d);
}


static void queue_string(struct descriptor_info *d, const char *s, int len)
{
    int where;
    int nbytes;

    /* we don't send anything if connection closed, so don't queue it. */
    if (d->close_down) return;

    /* Get the total number of bytes if we were to add this string (len),
       plus "\r\n" (2), to the queue. */
    nbytes = len + 2 + text_total_bytes(d->output_queue);

    if (nbytes > MAX_OUTPUT) {
	/* It'll be too much stuff, flush some output.  If a partial 
	   line is still waiting to be sent then start flushing from
	   line 2, otherwise from the start (line 1).  Flush enough
	   bytes to hold the string and the "\r\n". */
	where = TEXT_FIRST_LINE;
	if (d->output_pos) where++;
	flush_text(d->output_queue, where, len + 2);

	d->output_flushed = 1;
    }

    /* Insert the text on the last line of the queue. */
    insert2_text(d->output_queue, s, len, "\r\n", 2, TEXT_INSERT_END);
}


static void save_command(struct descriptor_info *d, Buffer *buf)
{
    insert_text(d->input_queue, Buftext(buf), Buflen(buf), TEXT_INSERT_END);
}


static void process_input(struct descriptor_info *d)
{
    Buffer buf;			/* To hold data read over socket. */
    int got;			/* # characters read in. */
    char *from;			/* Pointer into buf. */
    char *end;			/* Pointer to end of data in buf = from+got. */
    Buffer *to;			/* Same as d->input_buffer. */
    
    if (d->close_down) return;	/* Marked to close, don't bother reading. */

    /* If no buffer to hold text, make one. */
    if (!d->input_buffer) {
	MALLOC(d->input_buffer, Buffer, 1);
	Bufcpy(d->input_buffer, "");
    }
    to = d->input_buffer;

    /* Try to read in some data. */
    got = read(d->descriptor, Buftext(&buf), BUFFER_LEN);
    if (got <= 0) {
	/* Mark this socket to be closed. */
	d->close_down = 1;
	return;
    }

    /* Loop over all read characters copying them into the other buffer.
       Skip over nonprintables, and don't copy more than MAX_COMMAND_LEN.
       When a newline is seen store the line away, and reset the buffer
       back to the beginning to hold more data. If we see a backspace or
       delete, remove the previous character to make life easier for
       people with certain telnets. */
    for (from = Buftext(&buf), end = from + got; from < end; from++) {
	if (*from == '\n') {
	    if (Buflen(d->input_buffer)) {
		save_command(d, to);
		Bufcpy(to, "");
	    }
	} else if (isascii(*from) && isprint(*from)
		   && (Buflen(to) < MAX_COMMAND_LEN)) {
	    Bufcat_char(to, *from);
	} else if ((*from == '\b') || (*from == '\177')) {
	    Bufdel_char(to);
	}
    }

    /* No text left in the buffer. */
    if (!Buflen(to)) FREE(d->input_buffer);
}


static void set_userstring(const char **userstring, const char *command)
{
    if (*userstring) FREE_STRING(*userstring);

    while (*command && isspace(*command))
	command++;

    if (*command)
	*userstring = ALLOC_STRING(command);
}


static void parse_connect(const char *msg, char *command, char *user, char *pass)
{
    /* Get command. connect/create/etc. */
    while (*msg && isspace(*msg)) msg++;
    while (*msg && !isspace(*msg)) *command++ = *msg++;
    *command = '\0';

    /* Get user name. */
    while (*msg && isspace(*msg)) msg++;
    while (*msg && !isspace(*msg)) *user++ = *msg++;
    *user = '\0';

    /* Get password. */
    while (*msg && isspace(*msg)) msg++;
    while (*msg && !isspace(*msg)) *pass++ = *msg++;
    *pass = '\0';
}



static void check_connect(struct descriptor_info *d, const char *msg)
{
    Buffer command_buf, user_buf, password_buf;
    char *command, *user, *password;
    const char *hostname;
    dbref player;
    
    hostname = lookup_host(d->hostip, HOSTNAME_NO_RESOLVE);

    command = Buftext(&command_buf);
    user = Buftext(&user_buf);
    password = Buftext(&password_buf);

    parse_connect(msg, command, user, password);
    
    if (!strncmp (command, "co", 2)) {
	player = lookup_player(user);
	if ((player != NOTHING) && !ok_player_site(hostname, player)) {
	    /* This site locked out to this character. */
	    queue_string(d, connect_fail, sizeof(connect_fail)-1);
	    queue_string(d, connect_illegal, sizeof(connect_illegal)-1);
	    log_status("ILLEGAL CONNECT: %u from %s on fd %i.",
		       player, player, hostname, d->descriptor);
	    return;
	}

	player = connect_player(user, password);
	if (player == NOTHING) {
	    queue_string(d, connect_fail, sizeof(connect_fail)-1);
	    log_status("FAILED CONNECT: %s from %s on fd %i.",
		       user, hostname, d->descriptor);
	    return;
	}
    } else if (!strncmp(command, "cr", 2)) {
	if (!ok_create_site(hostname)) {
	    /* This site locked out to creation. */
	    queue_string(d, REG_MSG, sizeof(REG_MSG)-1);
	    log_status("ILLEGAL CREATE: %s from %s on fd %i.",
		       user, hostname, d->descriptor);
	    return;
	}

	player = create_player(user, password);
	if (player == NOTHING) {
	    queue_string(d, create_fail, sizeof(create_fail)-1);
	    log_status("FAILED CREATE: %s from %s on fd %i.",
		       user, hostname, d->descriptor);
	    return;
	} else {
	    log_status("CREATED: %u from %s on fd %i.",
		       player, player, hostname, d->descriptor);
	}
    } else {
	welcome_user(d);
	return;
    }
    /* Should only get here if connecting or just created. */
    log_status("CONNECTED: %u from %s on fd %i.",
	       player, player, hostname, d->descriptor);

    d->connect_time = time(NULL);
    d->player = player;

#ifdef RWHOD
    rwhocli_userlogin(rwho_uid(player), GetName(player), d->connect_time);
#endif

    do_connect(player);
}


static char *time_format_1(const long dt)
{
    static Buffer buf;

    if (dt >= TIME_DAY(1)) {
	Bufsprint(&buf, "%T", "%Jd %H:%M", dt);
    } else {
	Bufsprint(&buf, "%T", "%H:%M", dt);
    }

    return Buftext(&buf);
}


static char *time_format_2(const long dt)
{
    static Buffer buf;

    Bufsprint(&buf, "%T", "%i", dt);
    return Buftext(&buf);
}


static void dump_users(struct descriptor_info *e, const char *user)
{
    struct descriptor_info *d;
    int wizard, players;
    time_t now;
    const char *hostname;
    const char *header;
    Buffer pbuf;
    /* This buffers should be of sufficient size.  Ideally bufsprint
       should be fixed to accept field length designators then this
       could be done right.  But this sizes should work. */
    char buf[2048];
    
    wizard = (e->player == NOTHING) ? 0 : Wizard(e->player);
    
    while (*user && isspace(*user)) user++;
    if (!*user) user = NULL;
    
    (void) time(&now);

    if (wizard) {
#ifdef HOSTNAMES
	header = "Player Name               Location     On For Idle Host";
#else
	header = "Player Name                         Location     On For Idle Host";
#endif
    } else {
	header = "Player Name          On For Idle";
    }
    queue_string(e, header, strlen(header));
    
    
    d = connections;
    players = 0;
    while (d) {
	if ((d->player != NOTHING) && ++players &&
#ifdef DRUID_MUCK
	    (!HasFlag(d->player, INVISIBLE) || (wizard)) &&
#endif
	    (!user || muck_strprefix(GetName(d->player), user))) {
	    if (wizard) {
		hostname = lookup_host(d->hostip, HOSTNAME_NO_RESOLVE);
#ifdef HOSTNAMES
		Bufsprint(&pbuf, "%n(#%d)", d->player, d->player);
		sprintf(buf, "%-*s [%6ld] %10s %4s %-.26s",
			PLAYER_NAME_LIMIT + 9, Buftext(&pbuf),
			GetLoc(d->player),
			time_format_1(now - d->connect_time),
			time_format_2(now - d->last_time),
			hostname);
#else
			
		Bufsprint(&pbuf, "%u", e->player, d->player);
		sprintf(buf,"%-*s [%6ld] %10s %4s %-.26s",
			PLAYER_NAME_LIMIT + 19, Buftext(&pbuf),
			GetLoc(d->player),
			time_format_1(now - d->connect_time),
			time_format_2(now - d->last_time),
			hostname);
#endif
	    } else {
		sprintf(buf, "%-*s %10s %4s",
			PLAYER_NAME_LIMIT, GetName(d->player),
			time_format_1(now - d->connect_time),
			time_format_2(now - d->last_time));
	    }
	    queue_string(e, buf, strlen(buf));
	}
	d = d->next;
    }

    sprintf(buf, "%d player%s %s connected.", players,
	    (players == 1) ? "" : "s", (players == 1) ? "is" : "are");
    queue_string(e, buf, strlen(buf));
}


static void do_command(struct descriptor_info *d, const char *command, int len)
{
    if (!strcmp(command, QUIT_COMMAND)) {
	write(d->descriptor, LEAVE_MESSAGE, sizeof(LEAVE_MESSAGE)-1);
	d->close_down = 1;

    } else if (!strncmp(command, WHO_COMMAND, sizeof(WHO_COMMAND) - 1)) {
	if (d->output_prefix)
	    queue_string(d, d->output_prefix, strlen(d->output_prefix));
	dump_users(d, command + sizeof(WHO_COMMAND) - 1);
	if (d->output_suffix)
	    queue_string(d, d->output_suffix, strlen(d->output_suffix));

    } else if (!strncmp(command, PREFIX_COMMAND, sizeof(PREFIX_COMMAND) - 1)) {
	set_userstring(&d->output_prefix, command+sizeof(PREFIX_COMMAND) - 1);

    } else if (!strncmp(command, SUFFIX_COMMAND, sizeof(SUFFIX_COMMAND) - 1)) {
	set_userstring(&d->output_suffix, command+sizeof(SUFFIX_COMMAND) - 1);

    } else {
	if (d->player != NOTHING) {
	    /* Already connected player. */
	    if (d->output_prefix)
		queue_string(d, d->output_prefix, strlen(d->output_prefix));
	    process_command(d->player, command, len);
	    if (d->output_suffix)
		queue_string(d, d->output_suffix, strlen(d->output_suffix));
	} else {
	    /* Someone at welcome screen. */
	    check_connect(d, command);
	}
    }
}


static void process_commands(void)
{
    int nprocessed;
    struct descriptor_info *d;
    const char *command;
    int length;
    struct timeval start, now;

    /* In the old days most commands ran the same amount of time, with
       muf this is no longer entirely true.  So instead of processing
       until nothing more can be done which might take a long time, we
       kick out of processing if half of a second has passed. */
    /* #### ideally we should penalize a person for running a long
       program and not penalize people running shorter things.  This
       would involve potentially changing how the quotas are
       allocated, or maybe just subtracting off quota proportional to
       the command length instead of a constant 1.  You will still
       want some sort of kick out every so often to keep the perceived
       response time good, since no output is sent when in this
       loop. */

    gettimeofday(&start, NULL);
    do {
	nprocessed = 0;
	for (d = connections; d; d = d->next) {
	    if (d->close_down)
		continue;	/* Don't try to process this socket further. */
	    if (get_quota(d) > 0 && text_total_lines(d->input_queue)) {
		dec_quota(d);
		nprocessed++;
		command = text_line(d->input_queue, TEXT_FIRST_LINE, &length);
		do_command(d, command, length);
		delete_text(d->input_queue, TEXT_FIRST_LINE, TEXT_FIRST_LINE);
	    }
	}
	gettimeofday(&now, NULL);
    } while ((nprocessed > 0)
	     && (msec_diff(now, start) < (COMMAND_TIME_MSEC / 4)));
}


/* which == -1, boot all.
   which == 0, boot top.
   which == 1, boot most idle. */
int boot_off(const dbref player, int which)
{
    /* Fixed 2/29/92 by dmoore, no more @boot bug. */
    struct descriptor_info *d;
    struct descriptor_info *idlest;
    int cnt = 0;
    
    for (d = connections; d; d = d->next) {
	if (!d->close_down && d->player == player) {
	    if (which == -1) {
		process_output(d);	/* So they see the farewell message. */
		d->close_down = 1;
		cnt++;
	    } else if (which == 0) {
		process_output(d);
		d->close_down = 1;
		return 1;
	    } else {
		if (difftime(d->last_time, idlest->last_time) < 0) {
		    idlest = d;
		}
	    }
	}
    }

    if (which == 1) {
	process_output(idlest);
	idlest->close_down = 1;
	return 1;
    }

    return cnt;
}


void emergency_shutdown(void)
{
    struct descriptor_info *d;
    
    for (d = connections; d; d = d->next) {
	if (!d->close_down)
	    write(d->descriptor, crash_message, sizeof(crash_message)-1);
	if (shutdown(d->descriptor, 2) < 0)
	    log_status("PERROR: emergency_shutdown: shutdown -- %s",
		       strerror(errno));
	close(d->descriptor);
    }
    close_all_files();
}


void normal_shutdown(void)
{
    struct descriptor_info *d, *dnext;
    
    for (d = connections; d; d = dnext) {
	dnext = d->next;
	process_output(d);
	if (!d->close_down)
	    write(d->descriptor, shutdown_message, sizeof(shutdown_message)-1);

	shutdownsock(d);
    }
}


void dump_status(int signum)
{
    struct descriptor_info *d;
    time_t now;
    Buffer buf;
    const char *hostname;
    
    (void) time(&now);
    fprintf(stderr, "STATUS REPORT: ----------\n");
    for (d = connections; d; d = d->next) {
	if (d->player == NOTHING) {
	    Bufcpy(&buf, "CONNECT: ");
	} else {
	    Bufsprint(&buf, "PLAYING: %u ", d->player, d->player);
	}
	hostname = lookup_host(d->hostip, HOSTNAME_NO_RESOLVE);
	Bufcatlist(&buf, "HOSTNAME: ", hostname,
		   ", idle %d seconds, on fd %d.\n",
		   (const char *) 0);
	
	fprintf(stderr, Buftext(&buf), (int) difftime(now, d->last_time),
		d->descriptor);
    }
    fprintf(stderr, "END REPORT: -------------\n");
    signal(signum, dump_status);
}




/* Returning a listening fd on the specified port.  If the port is
   numeric then use that port #, if it is a text string then lookup
   that service name.  Unbind the port if it is already in use.
   Die if something doesn't work. */
int set_up_socket(const char *port)
{
    int sockfd, opt;
    u_short port_num;
    struct sockaddr_in serv_addr;
    
    /* Open up a tcp stream socket. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	log_status("PERROR: set_up_socket: socket -- %s", strerror(errno));
	exit(1);
    }

    /* Set the address as reusable. */
    opt = 1;
    if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR,
		    (char *) &opt, sizeof(opt)) < 0) {
	log_status("PERROR: set_up_socket: setsockopt -- %s",
		   strerror(errno));
	exit(1);
    }
    
    /* Set up the port number. */
    port_num = atoi(port);
    if (port_num != 0) {
	/* It was a number, so convert to network byte order. */
	port_num = htons(port_num);
    } else {
	/* It wasn't a number, so look it up by service name. */
	struct servent *sp;
	
	if ((sp = getservbyname(port, "tcp")) == NULL) {
	    fprintf(stderr, "can't find requested service: %s\n", port);
	    exit(1);
	}
	
	port_num = sp->s_port;
    }
    
    /* Set up the serv_addr structure. */
    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = port_num;	/* Already in network byte order. */
    
    /* Bind to the port. */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
	log_status("PERROR: set_up_socket: bind -- %s", strerror(errno));
	exit(1);
    }
    
    return (sockfd);
}


void main_loop(const int listen_sock)
{
    struct fd_set input_set, output_set;
    time_t now;
    struct timeval last_slice, current_time;
    struct timeval next_slice;
    struct timeval timeout, slice_timeout;
#ifdef RWHOD
    struct timeval rwho_timeout;
#endif
    int maxd;
    struct descriptor_info *d, *dnext;
    struct descriptor_info *newd;
    int avail_descriptors;
    time_t last_rwho_update;
    time_t last_dump_time;
    
    maxd = listen_sock + 1;
    gettimeofday(&last_slice, (struct timezone *) 0);
    time(&now);
    last_rwho_update = now;
    last_dump_time = now;
        
    /* 4 fd's needed for: status log, stderr, muf/dump/help/other files,
       accepting socket. */
    avail_descriptors = muck_max_fds() - 4;
    /* I think that rwho uses one more, check that. FIX FIX FIX */
    
    /* Start listening. */
    listen(listen_sock, 5);
    while (shutdown_flag == 0) {
	
	/* Check to see if it's time to dump the muck. */
	time(&now);
	if (dump_flag || difftime(now, last_dump_time) >= DUMP_INTERVAL) {
	    fork_and_dump();
	    last_dump_time = now;
	    dump_flag = 0;
	    time(&now);		/* Dumping might take a long time. */
	}

#ifdef RWHOD
	if (difftime(now, last_rwho_update) >= RWHO_INTERVAL) {
	    rwho_update();
	    last_rwho_update = now;
	}
#endif
	
	gettimeofday(&current_time, NULL);
	last_slice = update_quotas(last_slice, current_time);
	
	process_commands();

	if (shutdown_flag)
	    break;

	/* Close down anyone who QUIT or was booted, or any sockets which
	   closed for other reasons. */
	for (d = connections; d; d = dnext) {
	    dnext = d->next;
	    if (d->close_down) close_connection(d);
	}

	/* Set up the fdsets for select. */
	FD_ZERO(&input_set);
	FD_ZERO(&output_set);
	if (ndescriptors < avail_descriptors)
	    FD_SET(listen_sock, &input_set);
	for (d = connections; d; d = d->next) {
	    if (!text_total_lines(d->input_queue))
		FD_SET(d->descriptor, &input_set);
	    if (text_total_lines(d->output_queue))
		FD_SET(d->descriptor, &output_set);
	}


	/* Figure out the timeout for select.  Start with the minimum
	   of time to next rwho update or dump.  If people still have
	   text waiting to be processed then use the slice_timeout. */
	time(&now);
	gettimeofday(&current_time, NULL);

	timeout.tv_usec = 0;
	timeout.tv_sec = difftime(now, last_dump_time);

#ifdef RWHOD
	rwho_timeout.tv_usec = 0;
	rwho_timeout.tv_sec = difftime(now, last_rwho_update);
	timeout = timeval_min(timeout, rwho_timeout);
#endif

	next_slice = msec_add(last_slice, COMMAND_TIME_MSEC);
	slice_timeout = timeval_sub(next_slice, current_time);
	for (d = connections; d; d = d->next) {
	    if (text_total_lines(d->input_queue)) {
	        /* If someone has queued lines and remaining quota, we
		   want to get back to them quickly.  But if they ran
		   out of quota, then we sleep for a slice. */
	        if (d->quota) {
		    timeout.tv_usec = 0;
		    timeout.tv_sec = 0;
		    break;
		}
		timeout = timeval_min(timeout, slice_timeout);
	    }
	}

	while (timeout.tv_usec < 0) {
	    timeout.tv_usec += 1000000;
	    timeout.tv_sec--;
	}
	if (timeout.tv_sec < 0)
	    timeout.tv_sec = 0;

	if (select(maxd, &input_set, &output_set, (void*)NULL, &timeout) < 0) {
	    if (errno == EINTR) {
		continue;	/* Go back to top of loop, interrupted. */
	    }

	    log_status("PERROR: main_loop: select -- %s", strerror(errno));
	    return;
	}
	
	time(&now);
	if (FD_ISSET(listen_sock, &input_set)) {
	    if (!(newd = new_connection(listen_sock))) {
		log_status("PERROR: main_loop: new_connection -- %s",
			   strerror(errno));
		/* NOTE: this used to exit the mud if this failed. */
	    } else {
		if (newd->descriptor >= maxd)
		    maxd = newd->descriptor + 1;
	    }
	}
	for (d = connections; d; d = d->next) {
	    if (FD_ISSET(d->descriptor, &input_set)) {
		d->last_time = now;
		process_input(d);
	    }
	    if (FD_ISSET(d->descriptor, &output_set))
		process_output(d);
	}
    }
}
