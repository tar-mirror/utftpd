/* miniudpserv: something like an inetd for the "make check" target of utftpd */

/*
 * Copyright (C) 1999 Uwe Ohse
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "uogetopt.h"
#include "timselsysdep.h"
#include "nonblock.h"
#include "str2num.h"
#include "uostr.h"
#include "uoio.h"
#include "cdb.h"
#include "sys/wait.h"
#include "str_ulong.h"

#ifndef errno
extern int errno;
#endif

int peer;
int nullfd;

static char *argv0;

static uogetopt_t myopts[] =
{
	{0, 0, 0, 0, 0, 0, 0}
};

static void 
usage(void)
{write(2,argv0,strlen(argv0));write(2,": use the --help option for usage infomation\n",47); _exit(1); }

static int 
eat(int fd)
{
	char buf[1];
	recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *) 0, 0);
	return 0;
}

int
main (int argc, char **argv)
{
	unsigned long ul;
	fd_set set;
	struct sockaddr_in s_in;

#ifdef HAVE_LIBEFENCE
    /* hack around efence :-) */
	{int fd,fd2;void *waste;
	 if (-1==(fd=open("/dev/null",O_WRONLY))) _exit(1);
	 if (-1==(fd2=dup(2))) _exit(1);
	 if (-1==(dup2(fd,2))) _exit(1);
	 waste=malloc(1);
	 if (-1==(dup2(fd2,2))) _exit(1);
	 close(fd); close(fd2);
	}
#endif

	argv0 = strrchr (argv[0], '/');
	if (!argv0++)
		argv0 = argv[0];
	nullfd = open ("/dev/null", O_RDWR);
	if (nullfd == -1) { syslog (LOG_ERR, "cannot open /dev/null: %s", strerror (errno)); _exit (1); }
	uogetopt ("miniudpserv", PACKAGE, VERSION, &argc, argv, uogetopt_out, 0, myopts, "usage: miniudpserv host port program ...");
	if (argc<4) usage();

	openlog ("miniudpserv", LOG_PID | LOG_NDELAY, LOG_DAEMON);

	memset (&s_in, 0, sizeof (s_in));
	s_in.sin_family = AF_INET;
	if (0 == inet_aton (argv[1], &s_in.sin_addr)) { syslog (LOG_ERR, "%s: not a valid in_addr", argv[1]); _exit (1); }
	if (-1 == str2ulong (argv[2], &ul, 0) || ul == 0 || ul > 65535)
		{ syslog (LOG_ERR, "%s: not a valid port number", argv[2]); _exit (1); }
	s_in.sin_port = htons (((short) ul));

	peer = socket (AF_INET, SOCK_DGRAM, 0);
	if (peer < 0) { syslog (LOG_ERR, "socket: %s", strerror (errno)); _exit (1); }
	if (bind (peer, (struct sockaddr *) &s_in, sizeof (s_in)) < 0) { syslog (LOG_ERR, "bind: %s", strerror (errno)); _exit (1); }

	FD_ZERO(&set);
	while (1) {
		char buf[1];
		pid_t pid;
		socklen_t len;
		len = sizeof(s_in);
		FD_SET(peer,&set);
		if (-1==select(peer+1,&set,0,0,0)) { syslog(LOG_ERR,"select: %s",strerror(errno)); _exit(1); }
#ifdef MSG_PEEK
		if (recvfrom(peer, buf, sizeof(buf), MSG_PEEK, (struct sockaddr *) & s_in, &len) < 0) {
			/* uh? */
			syslog(LOG_WARNING,"recvfrom(MSG_PEEK): %s",strerror(errno));
			continue;
		}
		syslog(LOG_INFO,"packet from %s",inet_ntoa(s_in.sin_addr));
#endif
		pid=fork();
		if (pid==-1) {
			syslog(LOG_ERR,"cannot fork: %s",strerror(errno));
			eat(peer);
		} else if (pid==0) { 
			/* child */
			if (dup2(peer,0) != 0) { syslog(LOG_ERR,"cannot dup2 socket->stdin: %s",strerror(errno)); _exit(1); }
			argv+=3;
			execvp(*argv,argv);
			syslog(LOG_ERR,"execvp %s: %s",*argv,strerror(errno));
			eat(peer);
			_exit(1);
		} else {
			waitpid(pid,0,0);
		}
	}
	_exit (0);
}
