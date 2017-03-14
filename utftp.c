/* utftp.c: simple tftp client to put files */
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
#include "no_tftp.h"
#include <netdb.h>
#include <pwd.h>
#include <grp.h>

#include <setjmp.h>
#include <syslog.h>
#include <errno.h>
#include <ctype.h>
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
#include "wildmat.h"
#include "tftplib.h"


static const char *argv0;
static char *opt_revision;
static uoio_t io_e;
static int opt_netascii=0;
static int opt_is_put=0;
static int opt_stop=0;
static int opt_duplicate_ack=0;
static unsigned long opt_force_timeout=0;
static unsigned long opt_segsize=512;
static unsigned long opt_timeout=5;
static unsigned long opt_retries=5;
static int opt_send_garbage=0;

static uogetopt_t myopts[]={
	{'b',"blocksize",   UOGO_ULONG,&opt_segsize,0, "use a blocksize different from the default 512.\n",0},
	{'d',"duplicate-ack",   UOGO_FLAG,&opt_duplicate_ack,1, 
		"duplicate ACKs sent\n",0},
	{'f',"force-timeout",   UOGO_ULONG,&opt_force_timeout,0, 
		"test server timeout handling\n"
		"by sleeping some seconds","SECONDS"},
	{'G',"garbage",   UOGO_FLAG,&opt_send_garbage,1, 
		"send 2 garbage bytes after the request (test only)\n",0},
	{'g',"get",   UOGO_FLAG,&opt_is_put,0, "get file from server (default)\n",0},
	{'n',"netascii",   UOGO_FLAG,&opt_netascii,1, "use netascii conversion\n",0},
	{'p',"put",   UOGO_FLAG,&opt_is_put,1, "put file onto server\n",0},
	{'r',"revision",  UOGO_STRING,&opt_revision,1, "get or put revision REV\n","REV"},
	{'R',"retries",   UOGO_ULONG,&opt_retries,0, "number of retries on timeout (1..255, default 5).\n",0},
	{'S',"stop",   UOGO_FLAG,&opt_stop,1, "stop upload after the first block.\n",0},
	{'t',"timeout",   UOGO_ULONG,&opt_timeout,0, "timeout value (1..255, default 5).\n",0},
    {0,0,0,0,0,0,0}
};

static void
die (int e, const char *where)
{
	uoio_write_cstr (&io_e, argv0);
	uoio_write_cstr (&io_e, ": ");
	uoio_write_cstr (&io_e, where);
	if (e) {
		uoio_write_cstr (&io_e, ": ");
		uoio_write_cstr (&io_e, strerror (e));
	}
	uoio_write_cstr (&io_e, "\n");
	uoio_flush (&io_e);
	_exit (1);
}


static void 
usage(void)
{ die(0,"Give --help for usage information"); _exit(1); }

int 
main (int argc, char **argv)
{
	int port;
	struct servent *sp;
	char *p;
	struct tftplib_ctrl *ctrl;
	const char *target;
	const char *source;
#ifdef HAVE_LIBEFENCE
    /* hack around efence. Don't care about error messages here */
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
	if (argv0)
		argv0++;
	else
		argv0 = argv[0];
	uoio_assign_w (&io_e, 2, write, 0);
	if (strstr(argv0,"put")) opt_is_put=1;

	uogetopt ("utftp", PACKAGE, VERSION, &argc, argv, uogetopt_out,
			  "usage: utftp host[:port] filename-on-host [local-file-name]",
			  myopts,
			  "  \"local-file-name\" defaults to \"filename-on-host\" without any directory part.\n"
			  "  Report bugs to uwe-utftpd@bulkmail.ohse.de");

	if (!argv[1] || !argv[2] || (argv[3] && argv[4]))
		usage ();
	if (opt_segsize<512) {
		/* this can't work: if the server doesn't know about the blksize option it will send 512 byte blocks */
		die(0,"blocksize to low. 512 is minimum");
	}
#ifdef LOG_PERROR
	openlog(argv0,LOG_PERROR|LOG_PID,LOG_USER);
#else
	/* damned, HPUX 9.00 doesn't have LOG_PERROR. */
	openlog(argv0,LOG_PID,LOG_USER);
#endif
	if (opt_timeout<1 || opt_timeout >255) die(0,"illegal timeout value");
	if (opt_retries<1 || opt_retries >255) die(0,"illegal retry value");

	p=strchr(argv[1],':'); 
	if (p) {
		unsigned long ul;
		*p++=0;
		if (-1==str2ulong(p,&ul,0)) die(0,"not a valid port number");
		port=ul;
		if (ul!=(unsigned long) port) die(0,"not a valid port number");
		port=htons((unsigned short)port);
	} else {
		sp = getservbyname ("tftp", "udp");
		if (sp == 0) die (0, "udp/tftp: unknown service");
		port=sp->s_port;
	}
	
	ctrl=tftp_setup_ctrl(0, opt_segsize);
	if (!ctrl) die(0,"out of memory");
	ctrl->netascii=opt_netascii;
	ctrl->timeout=opt_timeout;
	ctrl->retries=opt_retries;
	ctrl->send_garbage=opt_send_garbage;
	if (opt_revision) ctrl->revision=opt_revision;
	source=argv[2];

	target=argv[3];
	if (!target || 0==strcmp(target,"--")) {
		target=strrchr(source,'/'); 
		if (target) target++;
		else target=source;
	}
	ctrl->force_timeout=opt_force_timeout;
	ctrl->force_stop=opt_stop;
	ctrl->duplicate_ack=opt_duplicate_ack;

	if (opt_is_put)
		return tftp_send(argv[1],port,source,target,ctrl);
	else
		return tftp_receive(argv[1],port,source,target,ctrl);
}
