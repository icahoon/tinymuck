/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* log.c,v 2.11 1997/08/10 07:44:27 dmoore Exp */
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>

#include "db.h"
#include "buffer.h"
#include "muck_time.h"
#include "params.h"
#include "externs.h"


static void do_log(const char *file, const char *fmt, va_list ap)
{
    Buffer buf;
    FILE *fp;
    time_t now;

    now = time(NULL);
    
    fp = fopen(file, "a");
    if (!fp) {
	fprintf(stderr, "Unable to open %s!\n", file);
	fp = stderr;
    }

    if (fmt && *fmt) {
        fprintf(fp, "%.16s: ", ctime(&now));
	Bufvsprint(&buf, fmt, ap);
	fprintf(fp, "%s --[%ld]\n", Buftext(&buf), systime2mucktime(now));
    } else {
        fprintf(fp, "\n");
    }

    if (fp != stderr) fclose(fp);
}


void log_status(const char *fmt, ...)
{
    va_list ap;
    
    va_start(ap, fmt);
    do_log(STATUS_LOG, fmt, ap);
    va_end(ap);
}


void log_muf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    do_log(MUF_LOG, fmt, ap);
    va_end(ap);
}


void log_gripe(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    do_log(GRIPE_LOG, fmt, ap);
    va_end(ap);
}


void log_command(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    do_log(COMMAND_LOG, fmt, ap);
    va_end(ap);
}


void log_info(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    do_log(INFO_LOG, fmt, ap);
    va_end(ap);
}


void log_last_command(const char *fmt, ...)
{
    Buffer buf;
    FILE *fp;
    time_t now;
    va_list ap;
    const char *file = "logs/last-command";

    va_start(ap, fmt);
    now = time(NULL);
    
    fp = fopen(file, "w");
    if (!fp) {
	fprintf(stderr, "Unable to open %s!\n", file);
	fp = stderr;
    }
    
    fprintf(fp, "%.16s: ", ctime(&now));
    Bufvsprint(&buf, fmt, ap);
    fprintf(fp, "%s --[%ld]\n", Buftext(&buf), systime2mucktime(now));
    fflush(fp);
    if (fp != stderr) fclose(fp);
    va_end(ap);
}


void log_sanity(const int type, const dbref obj, const char *fmt, va_list ap)
{
    const char *type_string;
    Buffer buf;

    if (type == 0) type_string = "Warning";
    else if (type == 1) type_string = "Fatal";
    else if (type == 2) type_string = "Repairing";
    else type_string = "XXX";

    Bufvsprint(&buf, fmt, ap);
    fprintf(stderr, "%s object #%ld: %s\n", type_string, obj, Buftext(&buf));
    fflush(stderr);
}
