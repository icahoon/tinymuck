/* muf_con.h,v 2.4 1997/08/16 22:31:36 dmoore Exp */
#ifndef MUCK_MUF_CON_H
#define MUCK_MUF_CON_H

struct muf_con_info {
    dbref player;		/* dbref of player */
    time_t connect_time;	/* connect time of player */
    time_t last_time;		/* time of last player command */
    const char *hostname;	/* their hostname */
};
 
#endif /* MUCK_MUF_CON_H */
