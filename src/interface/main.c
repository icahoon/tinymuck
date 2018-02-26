/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* main.c,v 2.18 1994/07/13 17:19:19 dmoore Exp */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/param.h>

#include "ansify.h"
#include "db.h"
#include "buffer.h"
#include "params.h"
#include "muck_time.h"
#include "externs.h"

typedef RETSIGTYPE (*sig_hndlr)(int);


const char *dumpfile = NULL;
static int epoch = 0;
static pid_t childpid = 0;
static ok_to_panic = 0;		/* 1 if panic dump ok, 0 if should just die. */

void close_all_files(void);
static void set_child_signals(void);

/* Dump out the database to the file named in the global var dumpfile. */
static void dump_database_internal(const char *kind_str)
{
    Buffer buf;
    char *tmpfile;

    if (!dumpfile) {
	log_status("DUMP ERROR: no dumpfile set, shouldn't happen.");
	dumpfile = ALLOC_STRING("muck.db");
    }
    log_status("%s: %s.#%d#", kind_str, dumpfile, epoch);

    /* Write out the db. */
    Bufsprint(&buf, "%s.#%i#", dumpfile, epoch);
    tmpfile = Buftext(&buf);

    if (db_write(tmpfile)) {
	if (rename(tmpfile, dumpfile) < 0) {
	    log_status("DUMP ERROR: couldn't rename the dump file.  %s to %s: %s",
		       tmpfile, dumpfile, strerror(errno));
	    fprintf(stderr, "DUMP FAILED\n");
	    remove(tmpfile);
	}
    } else {
	log_status("DUMP ERROR: write to disk failed.  %s: %s.",
		   tmpfile, strerror(errno));
	fprintf(stderr, "DUMP FAILED\n");
	remove(tmpfile);
    }
    
    /* Write out the macros */
    Bufsprint(&buf, "%s.#%i#", MACRO_FILE, epoch);
    tmpfile = Buftext(&buf);

    if (macrodump(tmpfile)) {
	if (rename(tmpfile, MACRO_FILE) < 0) {
	    log_status("DUMP ERROR: couldn't rename the dump file.  %s to %s: %s",
		       tmpfile, MACRO_FILE, strerror(errno));
	    fprintf(stderr, "DUMP FAILED\n");
	    remove(tmpfile);
	}
    } else {
	log_status("DUMP ERROR: write to disk failed.  %s: %s.",
		   tmpfile, strerror(errno));
	fprintf(stderr, "DUMP FAILED\n");
	remove(tmpfile);
    }

    log_status("%s: %s.#%d# (done)", kind_str, dumpfile, epoch);
}


/* Called when something very bad happens.  It attempts to write out the
   database into a special panic file, then dumps core. */
void panic(const char *message)
{
    Buffer buf;
    char *panicfile;
    int i;
    
    log_status("PANIC: %s", message);
    fprintf(stderr, "PANIC: %s\n", message);

    if (!ok_to_panic) {
	abort();
    }

    /* turn off signals */
    for (i = 0; i < NSIG; i++) {
	signal(i, SIG_IGN);
    }

    /* shut down interface */
    emergency_shutdown();
    
    /* dump panic file */
    Bufsprint(&buf, "%s.PANIC", dumpfile);
    panicfile = Buftext(&buf);

    if (db_write(panicfile)) {
	log_status("DUMPING(panic): %s (done)", panicfile);
	fprintf(stderr, "DUMPING(panic): %s (done)\n", panicfile);
    } else {
	log_status("DUMP FAILED: %s", strerror(errno));
	fprintf(stderr, "PANIC DUMP FAILED");
    }

    for (i = 0; i < NSIG; i++) {
	signal(i, SIG_DFL);
    }
    abort();
}


/* Save the database by the selected method. */
void fork_and_dump(void)
{
#ifdef INLINE_DUMP
#ifdef LOUD_DUMP
    static const char message[] = "Saving database...";
    send_all(message, sizeof(message)-1, 1); /* Notify and immediately flush. */
#endif
    epoch++;
    dump_database_internal("CHECKPOINTING(inline)");
#else /* INLINE_DUMP */
    if (childpid) {
	/* Already doing a dump, don't do another. */
	return;
    }
    epoch++;
    childpid = fork();
    if (childpid == 0) {
	set_child_signals();
	close_all_files();
	dump_database_internal("CHECKPOINTING(fork)");
	_exit(0);		/* !!! */
    } else if (childpid < 0) {
	/* Fork failed. */
	childpid = 0;
        log_status("PERROR: fork_and_dump: fork -- %s", strerror(errno));
	dump_database_internal("CHECKPOINTING(inline)");
    }
#endif /* INLINE_DUMP */
}


/* Routine to happily wait for our children processes when they finish,
   so that we don't get any zombies. */
static void reaper(int signum)
{
    int stat;			/* union wait stat for bsd. */
    pid_t pid;

    while (1) {
#if HAVE_WAIT3
	pid = wait3(&stat, WNOHANG, 0);
#else
	pid = waitpid(-1, &stat, WNOHANG);
#endif
	if ((pid == 0) || ((pid == -1) && (errno == ECHILD)))
	    break;

	if (pid < 0) {
	    if (errno == EINTR) continue;
	    log_status("PERROR: reaper: wait3 -- %s", strerror(errno));
	    break;
	}

	if (pid != childpid) {
	    log_status("STRANGE CHILD: child process not created by muck, pid = %d.", pid);
	} else {
	    childpid = 0;
	}
    }
    signal(signum, reaper);	/* Reinstall signal handler. */
}


/* Signal handler for dangerous signals.  Basically calls panic
   to handle it. */
static void bailout(int signum)
{
    char message[1024];
    
    sprintf(message, "BAILOUT: caught signal %d", signum);
    panic(message);
    _exit(7);			/* Never reached. */
}


static int init_game(const char *infile, const char *outfile)
{
    FILE *f;
    
    if ((f = fopen(infile, "r")) == NULL) return -1;
    
    /* ok, read it in */
    log_status("LOADING: %s", infile);
    fprintf(stderr, "LOADING: %s\n", infile);
    if (db_read(f) < 0) return -1;
    log_status("LOADING: %s (done)", infile);
    fprintf(stderr, "LOADING: %s (done)\n", infile);
    
    /* everything ok */
    fclose(f);

    /* try to read macros. */
    if ((f = fopen(MACRO_FILE, "r")) == NULL)
	log_status("INIT: Macro storage file %s is tweaked.", MACRO_FILE);
    else {
	init_macros();		/* Initialize macro hash table. */
	macroload(f);
	fclose(f);
    }
    
    /* set up dumper */
    if (dumpfile) FREE_STRING(dumpfile);
    dumpfile = ALLOC_STRING(outfile);

    return 0;
}


/* Close all file descriptors except stderr(2). */
void close_all_files(void)
{
    int fd;

    for (fd = muck_max_fds() - 1; fd >= 0 ; fd--) {
	if (fd != 2) close(fd);	/* Don't close stderr. */
    }
}


/* Change the id of the muck process to the specified user. Exits if
   unable to do anything. */
static void do_setuid(void)
{
#ifdef MUD_ID
#include <pwd.h>
    struct passwd *pw;
    
    if ((pw = getpwnam(MUD_ID)) == NULL) {
	log_status("MUD ID: no pwent for %s", MUD_ID);
	exit(1);
    }
    if (setuid(pw->pw_uid) == -1) {
	log_status("MUD ID: can't setuid to %d", pw->pw_uid);
        log_status("PERROR: do_setuid: setuid -- %s", strerror(errno));

	exit(1);
    }
#endif /* MUD_ID */
}


/* Set up signals based on the passed arguments.  Make most dangerous
   signals set off the ondeath functions.  Have USR1 and USR2 be able
   to point at special functions. */
static void set_signals(sig_hndlr ondeath, sig_hndlr segv, sig_hndlr usr1, sig_hndlr usr2, sig_hndlr child)
{
    signal(SIGPIPE, SIG_IGN);	/* We use select to tell when fds are ready. */
    signal(SIGHUP, SIG_IGN);	/* Have disconnects not bother us. */

    /* Protect against most any signal. */
    signal(SIGINT, ondeath);
    signal(SIGTERM, ondeath);
    signal(SIGQUIT, ondeath);
    signal(SIGTRAP, ondeath);
    signal(SIGSYS, ondeath);
    signal(SIGTERM, ondeath);
#ifdef SIGABRT
    signal(SIGABRT, ondeath);
#endif
#ifdef SIGIOT
    signal(SIGIOT, ondeath);
#endif
#ifdef SIGILL
    signal(SIGILL, ondeath);
#endif
#ifdef SIGEMT
    signal(SIGEMT, ondeath);
#endif /* SIGEMT */
#ifdef SIGFPE
    signal(SIGFPE, ondeath);
#endif /* SIGFPE */
#ifdef SIGBUS
    signal(SIGBUS, ondeath);
#endif /* SIGBUS */
#ifdef SIGXCPU
    signal(SIGXCPU, ondeath);
#endif /* SIGXCPU */
#ifdef SIGXFSZ
    signal(SIGXFSZ, ondeath);
#endif /* SIGXFSZ */
#ifdef SIGVTALRM
    signal(SIGVTALRM, ondeath);
#endif /* SIGVTALRM */
#ifdef SIGDANGER
    signal(SIGDANGER, ondeath);
#endif /* SIGDANGER */

    /* SEGV is generally the same as ondeath, but might not for debugging. */
    signal(SIGSEGV, segv);
    
    /* If the muck gets a sigusr1 or sigusr2, then do something specific.
       For normal muck process it sends some additional information to
       stderr.  It's basically like the wizard WHO list but additionally
       shows what program a person is running if any. */
    signal(SIGUSR1, usr1);
    signal(SIGUSR2, usr2);

    /* What to do about children processes. */
#ifndef SIGCLD
    signal(SIGCHLD, child);
#else
    signal(SIGCLD, child);
#endif
}


/* Set up the right signal handlers for the main muck process. */
static void set_muck_signals(void)
{
#if defined(DEBUG_MALLOCS) || defined(CORE_SEGV)
    /* If we have the debugging mallocs, we want a SEGV to really kill
       us.  Of course you really lose the db too. */
    set_signals(bailout, SIG_DFL, dump_status, reload_lockout, reaper);
#else
    set_signals(bailout, bailout, dump_status, reload_lockout, reaper);
#endif /* DEBUG_MALLOCS */
}


/* Set up signal handlers for a dumping child process. */
static void set_child_signals(void)
{
    set_signals(SIG_DFL, SIG_DFL, SIG_IGN, SIG_IGN, SIG_IGN);
}


/* Set up signal handlers for the shutting down process. */
static void set_shutdown_signals(void)
{
    set_signals(SIG_DFL, SIG_DFL, SIG_IGN, SIG_IGN, SIG_IGN);
}


/* Check command line args, load up the db, initialize things, start up
   the processing loop, shutdown the db, clean things up.
   Comments marked with 'Perhaps' mean that their operation is controled
   by one or more compiler define options. */
int main(const int argc, const char * const *argv)
{
    int listen_sock;

    /* Verify proper number of arguments 2-3. */
    if ((argc < 3) || (argc > 4)) {
	fprintf(stderr, "Usage: %s infile dumpfile [port]\n", *argv);
	exit(1);
    }

    close_all_files();		/* Close all fd's except stderr. */
    srandom(getpid()+time(NULL)); /* Set random number generator. */

    listen_sock = set_up_socket(argc == 4 ? argv[3] : TINYPORT);
    /* If the port couldn't be bound, we exitted. */

    /* Do this after binding the socket, in case want to run priv port. */
    do_setuid();		/* Perhaps change to special uid. */

    init_mucktime();		/* Initialize possibly needed time convs. */

    log_status("INIT: %s starting.", MUCK_VERSION);

    /* Load up macros, main db, etc.  This inits the hashtab code,
       property code, macro code, interned string code. */
    if (init_game (argv[1], argv[2]) < 0) {
	fprintf(stderr, "Couldn't load %s!\n", argv[1]);
	exit(1);
    }

    init_compiler();            /* Initialize the compiler. */
    init_interp();		/* Initialize the interpreter. */
    init_editor();		/* Initialize the editor. */
    init_hostnames();		/* Initialize host mapping. */

    set_muck_signals();		/* Set up signal handlers. */

    ok_to_panic = 1;		/* DB loaded, if danger then panic. */

#ifdef RWHOD
    rwhocli_setup(RWHO_SERVER, RWHO_PASS, RWHO_NAME, MUCK_VERSION);
#endif

    main_loop(listen_sock);	/* Loop doing all the real work. */

    close(listen_sock);		/* Close the listen socket. */
    normal_shutdown();		/* Close down all sockets, send goodbye msg. */

#ifdef RWHOD
    rwhocli_shutdown();
#endif

    set_shutdown_signals();
    epoch++;
    dump_database_internal("DUMPING"); /* Write out the database. */

    ok_to_panic = 0;		/* DB saved already, don't try to panic. */

    dump_hashtab_stats(NULL);	/* Dump hashtable information out. */

#ifdef DEBUG_MALLOCS_TRACE
    if (dumpfile) FREE_STRING(dumpfile);
    free_lockout();
    clear_players();
    clear_editor();
    clear_hostnames();
    clear_macros();
    db_FREE();
    clear_interp();		/* must come after db_FREE */
    debug_mstats("After DB free");
#endif				/* DEBUG_MALLOCS_TRACE */

    exit (0);
}
