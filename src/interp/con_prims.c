/* Copyright (c) 1992 by Ben Jackson and David Moore.  All rights reserved. */
/* con_prims.c,v 2.6 1997/08/29 21:01:57 dmoore Exp */
#include "config.h"
 
#include "db.h"
#include "code.h"
#include "prim_offsets.h"
#include "buffer.h"
#include "muck_time.h"
#include "muf_con.h"
#include "externs.h"
 
 
#ifdef PRIM_concount
MUF_PRIM(prim_concount)
{
    inst arg;
    int count;
 
    connected_cons(&count);
    arg.type = INST_INTEGER;
    arg.un.integer = count;
    push_stack(curr_data_st, &arg);
}
#endif /* concount */
 
 
#ifdef PRIM_connections
MUF_PRIM(prim_connections)
{
    inst arg;
    int count, x, *cons;
 
    cons = connected_cons(&count);
 
    arg.type = INST_CONNECTION;
    for(x = count; x; --x) {
        arg.un.integer = *cons++;
        safe_push_data_st(curr_frame, curr_data_st, &arg, PRIM_connections);
	if (curr_frame->error) return;
    }

    arg.type = INST_INTEGER;
    arg.un.integer = count;
    safe_push_data_st(curr_frame, curr_data_st, &arg, PRIM_connections);
}
#endif /* connections */
 
 
#ifdef PRIM_condbref
MUF_PRIM(prim_condbref)
{
    inst arg;
    struct muf_con_info c;
 
    pop_stack(curr_data_st, &arg);
 
    if(con_info(arg.un.connection, &c)) {
        arg.type = INST_OBJECT;
        arg.un.object = c.player;
        push_stack(curr_data_st, &arg);
    } else {
        interp_error(curr_frame, PRIM_condbref,
		     "Invalid descriptor %i", arg.un.connection);
    }
}
#endif /* condbref */
 
 
#ifdef PRIM_contime
MUF_PRIM(prim_contime)
{
    inst arg;
    struct muf_con_info c;
 
    pop_stack(curr_data_st, &arg);
 
    if(con_info(arg.un.connection, &c)) {
        arg.type = INST_INTEGER;
        arg.un.integer = systime2mucktime(c.connect_time);
        push_stack(curr_data_st, &arg);
    } else {
        interp_error(curr_frame, PRIM_contime,
		     "Invalid descriptor %i", arg.un.connection);
    }
}
#endif /* contime */
 
 
#ifdef PRIM_conidle
MUF_PRIM(prim_conidle)
{
    inst arg;
    struct muf_con_info c;
    time_t now;
 
    pop_stack(curr_data_st, &arg);
 
    if(con_info(arg.un.connection, &c)) {
        now = time(NULL);

        arg.type = INST_INTEGER;
        arg.un.integer = difftime(now, c.last_time);
        push_stack(curr_data_st, &arg);
    } else {
        interp_error(curr_frame, PRIM_conidle,
		     "Invalid descriptor %i", arg.un.connection);
    }
}
#endif /* conidle */
 
 
#ifdef PRIM_conhost
MUF_PRIM(prim_conhost)
{
    inst arg;
    struct muf_con_info c;
 
    pop_stack(curr_data_st, &arg);
 
    if(con_info(arg.un.connection, &c)) {
        arg.type = INST_STRING;
	if (c.hostname) {
	    arg.un.string = make_shared_string(c.hostname, strlen(c.hostname));
	} else {
	    arg.un.string = NULL;
	}
        push_stack(curr_data_st, &arg);
    } else {
        interp_error(curr_frame, PRIM_conhost,
		     "Invalid descriptor %i", arg.un.connection);
    }
}
#endif /* conhost */
 
 
#ifdef PRIM_conboot
MUF_PRIM(prim_conboot)
{
    inst arg;
    struct muf_con_info c;
 
    pop_stack(curr_data_st, &arg);
 
    if(con_info(arg.un.connection, &c)) {
        int success;
 
        if(TrueGod(c.player)) {
            success = 0;
        } else {
            notify(c.player, "You have been booted off the game.");
            success = boot_off_d(arg.un.connection);
	}

	if(success) {
	    log_status("BOOTED: %u by %u.",
		       c.player, c.player,
		       curr_frame->player, curr_frame->player);
	}
    } else {
        interp_error(curr_frame, PRIM_conboot,
		     "Invalid descriptor %i", arg.un.connection);
    }
}
#endif /* conboot */
 
 
#ifdef PRIM_online
MUF_PRIM(prim_online)
{
    inst arg;
    int count;
    int i;
    dbref *who;
 
    who = connected_players(&count);
 
    arg.type = INST_OBJECT;
    for (i = count; i; i--) {
        arg.un.object = *who++;

        safe_push_data_st(curr_frame, curr_data_st, &arg, PRIM_online);
	if (curr_frame->error) return;
    }

    arg.type = INST_INTEGER;
    arg.un.integer = count;
    safe_push_data_st(curr_frame, curr_data_st, &arg, PRIM_online);
}
#endif /* online */


#ifdef PRIM_awakep
MUF_PRIM(prim_awakep)
{
    inst who;
    inst result;

    pop_stack(curr_data_st, &who);
    
    result.type = INST_INTEGER;
    result.un.integer = check_awake(who.un.object);

    push_stack(curr_data_st, &result);
}
#endif /* awakep */
