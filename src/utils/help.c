/* Copyright (c) 1992 by Ben Jackson.  All rights reserved. */
/* help.c,v 2.5 1994/03/10 04:14:47 dmoore Exp */

#define HELPSEPCHAR '&'

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>  /* strchr() */

#include "db.h"
#include "params.h"
#include "buffer.h"
#include "externs.h"


static int match_help_line(Buffer *buf, const char *arg1, const int arglen) {

    char *i = Buftext(buf);

    if(Buflen(buf) >= arglen) {
       do {
	   if(!muck_strnicmp(i, arg1, arglen))
               return 1;
       } while((i = strchr(i, ',')) && *++i && isspace(*i) && *++i);
    }
    return 0;
}

void help(dbref player, const char *arg1, const char *file)
{
    FILE *f;
    Buffer buf;
    int arglen, show_topic;

    f = fopen(file, "r");

    if (!f) {
        notify(player, "Sorry, %s is broken.  Management has been notified.",
	       file);
	log_status("FILE: %s not found.", file);
    }
    else 
    {
        if(arg1 && *arg1) 
        {
            arglen = strlen(arg1);
            do 
            {
                do 
                {
                    if(!(Bufgets(&buf, f, NULL))) 
                    {
		      notify(player, "Sorry, no help available on topic `%s'.",
			     arg1);
		      fclose(f);
		      return;
                    }
                } 
                while(*Buftext(&buf) != HELPSEPCHAR);
		show_topic = !(HELPSEPCHAR == *(Buftext(&buf)+1));
                Bufgets(&buf, f, NULL);
            } 
            while(!match_help_line(&buf, arg1, arglen));

	    notify(player, "--");
	    if(show_topic)
	      notify(player, "%S", &buf);
        }

        while(Bufgets(&buf, f, NULL)) 
        {
            if(*Buftext(&buf) == HELPSEPCHAR)
                break;
            notify(player, "%S", &buf);
        }
	notify(player, "--");

        fclose(f);
    }
}

void do_help(const dbref player, const char *arg1, const char *ign2, const char *ign3)
{
    help(player, arg1, HELP_FILE);
}

void do_man(const dbref player, const char *arg1, const char *ign2, const char *ign3)
{
    help(player, arg1, MAN_FILE);
}

void do_news(const dbref player, const char *arg1, const char *ign2, const char *ign3)
{
    help(player, arg1, NEWS_FILE);
}
