/* hostname.h,v 2.3 1993/04/08 20:24:47 dmoore Exp */
#ifndef MUCK_HOSTNAME_H
#define MUCK_HOSTNAME_H

#define HOSTNAME_OK_RESOLVE 1
#define HOSTNAME_NO_RESOLVE 0

typedef unsigned long ip_addr_t;
const char *lookup_host(ip_addr_t, int);
void init_hostnames(void);
void clear_hostnames(void);

#endif /* MUCK_HOSTNAME_H */
