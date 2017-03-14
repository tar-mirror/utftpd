/* utftpd_novc: the "no version control" version control */

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
#include <fcntl.h>
#include <netinet/in.h>

#include "no_tftp.h"

#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "utftpd.h"

/* every file is under "no version control" */
static int
novc_test(const char *filename, struct utftpd_ctrl *flags) 
{
	(void) filename;
	(void) flags;
	return 0;
}

static int 
novc_commit (const char *comment, struct utftpd_ctrl *flags)
{
	(void) comment;
#ifdef HAVE_FSYNC
	if (fsync(flags->filefd)==-1)  {
		syslog(LOG_ERR,"unable to fsync: %s", strerror(errno));
		do_nak(flags->remotefd,EUNDEF,"unable to fsync");
		_exit(1);
	}
#else
# ifndef (O_SYNC)
    sync();
# endif
#endif
	if (close (flags->filefd) == -1) {
		syslog(LOG_ERR,"unable to close: %s", strerror(errno));
		do_nak(flags->remotefd,EUNDEF,"unable to close");
		_exit(1);
	}
	return 0;
}

static int
novc_checkout(int mode, struct utftpd_ctrl *flags)
{
	int fd;
#define ADD_ON 0
#ifndef HAVE_FSYNC
# ifdef (O_SYNC)
#  undef ADD_ON
#  define ADD_ON O_SYNC
# endif
#endif

	if (mode==TSG_READ) {
		fd=open(flags->filename,O_RDONLY);
	} else if (mode==TSG_CREATE) {
		fd=open(flags->filename,O_WRONLY|O_CREAT|O_EXCL|ADD_ON,0644);
	} else {
		fd=open(flags->filename,O_WRONLY|O_TRUNC|ADD_ON,0644);
	}
	if (fd==-1) {
		const char *er=strerror(errno);
		syslog(LOG_ERR,"open %s: %s",flags->filename,er);
		/* magic stuff. Basically this is catastropheprevention in case of a TFTP read 
		 * request to a broadcast address.
		 */
		if (mode==TSG_READ && opt_suppress_naks && *flags->origfilename!='/') {
			if (opt_verbose) syslog(LOG_INFO,"suppressing NAK for nonexistant file");
			_exit(0);
		}
		do_nak(flags->remotefd,EUNDEF,er);
		_exit(1);
	}
	flags->filefd=fd;
	return 0;
}


struct utftpd_vc utftpd_novc={
	novc_test,
	novc_commit,
	novc_checkout,
	NULL
};
