/* lockout.h,v 2.3 1993/04/08 20:24:47 dmoore Exp */
#ifndef MUCK_LOCKOUT_H
#define MUCK_LOCKOUT_H

void reload_lockout(const int dummy);
int ok_conn_site(const char *hostname);
int ok_create_site(const char *hostname);
int ok_player_site(const char *hostname, const dbref who);

#endif /* MUCK_LOCKOUT_H */
