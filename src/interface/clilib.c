/*
  Copyright (C) 1991, Marcus J. Ranum. All rights reserved.
  */

/*
  code to interface client MUDs with rwho server
  
  this is a standalone library.
  */

/* Indentation changed 3/12/92 by dmoore to reflect rest of muck style.
   Function declarations changed to ansi form.  The internals were not
   changed significantly.
   Other changes to malloc, and routines using bcopy and such.
   */


#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<fcntl.h>
#include	<sys/file.h>
#include	<sys/time.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>

#include "config.h"
#include "db.h"

#define	DGRAMPORT		6888

#ifndef	NO_HUGE_RESOLVER_CODE
extern	struct	hostent	*gethostbyname();
#endif


static	int			dgramfd = -1;
static	const char		*password = 0;
static	const char		*localnam = 0;
static	const char		*lcomment = 0;
static	struct sockaddr_in	addr;
static	time_t			senttime;


/* enable RWHO and send the server a "we are up" message */
int rwhocli_setup(const char *server, const char *serverpw, const char *myname, const char *comment)
{
    
#ifndef	NO_HUGE_RESOLVER_CODE
    struct	hostent		*hp;
#endif
    char			pbuf[512];
    const char			*p;
    
    if (dgramfd != -1)
	return(1);
    
    password = ALLOC_STRING(serverpw);
    localnam = ALLOC_STRING(myname);
    lcomment = ALLOC_STRING(comment);
    
    p = server;
    while (*p != '\0' && (*p == '.' || isdigit(*p))) p++;
    
    if (*p != '\0') {
#ifndef	NO_HUGE_RESOLVER_CODE
	if ((hp = gethostbyname(server)) == (struct hostent *)0)
	    return(1);
	memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
#else
	return(1);
#endif
    } else {
	unsigned long	f;
	
	if ((f = inet_addr(server)) == -1L)
	    return(1);
	memcpy(&addr.sin_addr, &f, sizeof(f));
    }
    
    addr.sin_port = htons(DGRAMPORT);
    addr.sin_family = AF_INET;
    
    if ((dgramfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	return(1);
    
    time(&senttime);
    
    sprintf(pbuf, "U\t%.20s\t%.20s\t%.20s\t%.10d\t0\t%.25s",
	    localnam, password, localnam, senttime, comment);
    sendto(dgramfd, pbuf, strlen(pbuf), 0, &addr, sizeof(addr));
    return(0);
}





/* disable RWHO */
int rwhocli_shutdown(void)
{
    char	pbuf[512];
    
    if (dgramfd != -1) {
	sprintf(pbuf, "D\t%.20s\t%.20s\t%.20s", localnam, password, localnam);
	sendto(dgramfd, pbuf, strlen(pbuf), 0, &addr, sizeof(addr));
	close(dgramfd);
	dgramfd = -1;
    }
    /* 12/26/91 dmoore, moved the frees out of the if above, but protected
       against freeing NULL.  Cause they might have been allocated, and
       still have dgramfd = -1. */
    if (password) FREE_STRING(password);
    if (localnam) FREE_STRING(localnam);
    if (lcomment) FREE_STRING(lcomment);
    password = localnam = lcomment = 0;
    return(0);
}





/* send an update ping that we're alive */
int rwhocli_pingalive(void)
{
    char	pbuf[512];
    
    if (dgramfd != -1) {
	sprintf(pbuf, "M\t%.20s\t%.20s\t%.20s\t%.10d\t0\t%.25s", 
		localnam, password, localnam, senttime, lcomment);
	sendto(dgramfd, pbuf, strlen(pbuf), 0, &addr, sizeof(addr));
    }
    return(0);
}





/* send a "so-and-so-logged in" message */
int rwhocli_userlogin(const char *uid, const char *name, const time_t tim)
{
    char	pbuf[512];
    
    if (dgramfd != -1) {
	sprintf(pbuf, "A\t%.20s\t%.20s\t%.20s\t%.20s\t%.10d\t0\t%.20s",
		localnam, password, localnam, uid, tim, name);
	sendto(dgramfd, pbuf, strlen(pbuf), 0, &addr, sizeof(addr));
    }
    return(0);
}





/* send a "so-and-so-logged out" message */
int rwhocli_userlogout(const char *uid)
{
    char	pbuf[512];
    
    if (dgramfd != -1) {
	sprintf(pbuf, "Z\t%.20s\t%.20s\t%.20s\t%.20s", 
		localnam, password, localnam, uid);
	sendto(dgramfd, pbuf, strlen(pbuf), 0, &addr, sizeof(addr));
    }
    return(0);
}

