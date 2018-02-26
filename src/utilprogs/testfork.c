/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* testfork.c,v 2.3 1993/04/08 20:24:42 dmoore Exp */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>

#if defined(_AIX) || defined(___AIX)
#include <sys/select.h>
#endif

static pid_t childpid = 0;

static void reaper(int signum)
{
    int stat;			/* union wait stat for bsd. */
    pid_t pid;

    while (1) {
	pid = wait3(&stat, WNOHANG, 0);
	if ((pid == 0) || ((pid == -1) && (errno == ECHILD)))
	    break;

	if (pid < 0) {
	    if (errno == EINTR) continue;
	    perror("wait3");
	    break;
	}

	if (pid != childpid) {
	    fprintf(stderr, "Parent: strange child, pid = %d, expected = %d\n",
		   pid, childpid);
	} else {
	    fprintf(stderr, "Parent: child finished, pid = %d\n", pid);
	    childpid = 0;
	}
    }
    signal(signum, reaper);	/* Reinstall signal handler. */
}

int main(int argc, char **argv)
{
    int curr_size, step_size, num_steps, num_runs;
    int i, run, bad;
    size_t real_size;
    char *memory;

    if (argc < 4 || argc > 5) {
	fprintf(stderr, "usage: %s <start size> <step size> <# steps> [<# runs>]\n",
	       *argv);
	exit(1);
    }

    curr_size = atoi(argv[1]);
    step_size = atoi(argv[2]);
    num_steps = atoi(argv[3]);
    num_runs = (argc == 5) ? atoi(argv[4]) : 1;

    if (curr_size <= 0 || step_size <= 0 || num_steps <= 0 || num_runs <= 0) {
	fprintf(stderr, "all arguments must be positive\n");
	exit(1);
    }

    fprintf(stderr, "%s %d %d %d %d\n", *argv, curr_size, step_size,
	   num_steps, num_runs);

#ifndef SIGCLD
    signal(SIGCHLD, reaper);
#else
    signal(SIGCLD, reaper);
#endif

    memory = malloc(sizeof(*memory));
    if (!memory) {
	fprintf(stderr, "out of memory, no start\n.");
	exit(1);
    }

    for (; num_steps; curr_size += step_size, num_steps--) {
	for (run = 1; run <= num_runs; run++) {
	    fprintf(stderr, "Parent: starting run %d at %dM.\n", run, curr_size);

	    real_size = curr_size * 1024 * 1024 / sizeof(*memory);
	    memory = realloc(memory, real_size*sizeof(*memory));
	    if (!memory) {
		fprintf(stderr, "Parent: out of memory: %dM\n", curr_size);
		exit(1);
	    }

	    for (i = 0; i < real_size; i++)
		memory[i] = 0;

	    childpid = fork();
	    if (childpid == 0) {
		sleep(1);
		fprintf(stderr, "Child: starting memory munging\n");
		for (i = 0; i < real_size; i++)
		    memory[i] = 1;
		fprintf(stderr, "Child: done memory munging\n");
		_exit(0);
	    } else if (childpid < 0) {
		perror("Parent: fork");
		exit(1);
	    }
	    fprintf(stderr, "Parent: forking child = %d\n", childpid);

	    while (1) {
		select(0, NULL, NULL, NULL, NULL); /* Wait for a signal. */
		if (!childpid) break;
	    }

	    fprintf(stderr, "Parent: checking memory\n");
	    bad = 0;
	    for (i = 0; i < real_size; i++)
		if (memory[i] != 0) bad++;
	
	    if (bad)
		fprintf(stderr, "Parent: memory changed, bad cnt = %d\n", bad);
	    else
		fprintf(stderr, "Parent: memory unchanged\n");
	}
    }
}
