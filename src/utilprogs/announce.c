/*
	Dumb announcer.  Courtesy Andrew Molitor.

*/

#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <ctype.h>
#include <string.h>


main(ac, av)
int	ac;
char	*av[];
{
	struct sockaddr_in	s, remote;
	int			opt;
	int			fd, newfd;
	int			remotesiz;
	int			retval;
	FILE			*netw, *datafile;
	char			filename[256], buff[512];
	unsigned short		port;
	unsigned int		clientIp, clientPort;

	switch(ac) {
	case 2:
		port = 3333;
		break;
	case 3:
		port = atoi(av[2]);
		break;
	default:
		fprintf(stderr, "Usage: %s filename [ portnum ]\n", av[0]);
		exit(-1);
		break;
	}

	strcpy(filename, av[1]);

	s.sin_family = AF_INET;
	s.sin_port = htons(port);
	s.sin_addr.s_addr = INADDR_ANY;

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if(fd < 0) {
		perror("socket");
		exit(-1);
	}

	opt = 1;
	if(setsockopt(
		fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0){
		perror("setsockopt");
		exit(-1);
	}

	if(bind(fd, &s, sizeof(s)) < 0) {
		perror("bind");
		exit(-1);
	}

	if(listen(fd, 5) < 0) {
		perror("listen");
		exit(-1);
	}

	while(1) {
		remotesiz = sizeof(remote);
		newfd = accept(fd, &remote, &remotesiz);
		if(newfd < 0) {
			perror("accept");
			sleep(1);
			continue;
		}
		clientPort = remote.sin_port;

		netw = fdopen(newfd, "w");
		if(netw == NULL) {
			perror("fdopen");
			fclose(netw);
			close(newfd);
			sleep(1);
			continue;
		}

		fprintf(netw, "Hi, %s/%d.\n", inet_ntoa(remote.sin_addr),
			clientPort);
		fflush(netw);

		datafile = fopen(filename, "r");
		if(datafile == NULL) {
			fprintf(netw, "Ooops. No info. Sorry!\n");
		} else {
			while(fgets(buff, 512, datafile)) {
				fputs(buff, netw);
				fflush(netw);
			}
			fclose(netw);
			fclose(datafile);
		}
	}
}
