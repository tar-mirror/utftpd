/* utftpd_rcs: rcs support functions */

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
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "uogetopt.h"
#include "timselsysdep.h"
#include "nonblock.h"
#include "str2num.h"
#include "uostr.h"
#include "uoio.h"
#include "sys/wait.h"
#include "wildmat.h"
#include "utftpd.h"
#include "str_ulong.h"

static int 
utftpd_rcs_test(const char *filename, struct utftpd_ctrl *flags)
{
	const char *slash;
	static uostr_t s=UOSTR_INIT;
	size_t pos;
	struct stat st;
	if (!opt_rcs_co || !opt_rcs_ci) return -1;
	slash=strrchr(filename,'/');
	if (!uostr_dup_mem(&s,filename,slash-filename+1)) goto oom;
	pos=s.len;
	/* try to get an RCS file */
	if (!uostr_add_cstr(&s,"RCS/")) goto oom;
	if (!uostr_add_cstr(&s,slash+1)) goto oom;
	if (!uostr_add_mem(&s,",v",3)) goto oom; /* \0 */
	if (0==stat(s.data,&st)) {
		return 0;
	}
	return 1; /* this file is not under RCS */
  oom:
  	syslog(LOG_ERR,"out of memory");
	do_nak(flags->remotefd,EUNDEF,"out of memory");
	_exit(1); return 0; /* supress useless warning */
}

/*
 * this is very easy compared to SCCS, since RCS by default does not
 * create a new revision when checking in unchanged files.
 */
static int 
utftpd_rcs_ci(const char *comment, struct utftpd_ctrl *flags)
{
	const char *slash;
	static uostr_t s=UOSTR_INIT;
	size_t pos;
	const char *er;
	pid_t pid;

	slash=strrchr(flags->filename,'/');
	if (!uostr_dup_mem(&s,flags->filename,slash-flags->filename+1)) goto oom;
	pos=s.len;
	/* try to get an RCS file */
	if (!uostr_add_cstr(&s,"RCS/")) goto oom;
	if (!uostr_add_cstr(&s,slash+1)) goto oom;
	if (!uostr_add_mem(&s,",v",3)) goto oom; /* \0 */

	/* the top level closed the file */
	pid=fork();
	if (pid==0) {
		static uostr_t co=UOSTR_INIT;
		union {const char **a1; char *const *a2;} d;
		const char *argv[5];
		int oc=0;
		uostr_xdup_cstr(&co,"-mby utftpd for ");
		uostr_xadd_cstr(&co,remoteip);
		uostr_xadd_cstr(&co," ");
		if (comment) {
			uostr_xadd_cstr(&co," ");
			uostr_xadd_cstr(&co,comment);
		}
		uostr_xadd_mem(&co,"",1);
		uostr_cut(&s,pos); uostr_add_mem(&s,"",1);
		if (-1==chdir(s.data)) _exit(1);
		if (-1==dup2(nullfd,2)) _exit(1);
		argv[oc++]="ci";
		if (flags->revision) {
			static uostr_t rev=UOSTR_INIT;
			if (!uostr_add_cstr(&rev,"-r")) _exit(1);
			if (!uostr_add_cstr(&rev,flags->revision)) _exit(1);
			if (!uostr_add_mem(&rev,"",1)) _exit(1);
			argv[oc++]=rev.data;
		}
		argv[oc++]=co.data;
		argv[oc]=strdup(slash+1);
		if (!argv[oc++]) _exit(1);
		argv[oc++]=0;
		d.a1=argv;
		execv(opt_rcs_ci,d.a2);
		_exit(1);
	} else if (pid==-1) {
		er=strerror(errno);
		syslog(LOG_ERR,"can't fork: %s",er);
		do_nak(flags->remotefd,EUNDEF,er);
		_exit(1);
	} else {
		int fail=wait_check_and_log(pid);
		if (fail) {
			syslog(LOG_ERR,"version control system failed");
			do_nak(flags->remotefd,EUNDEF,"version control system failed");
			_exit(1);
		}
	}
	return 0;
  oom:
  	syslog(LOG_ERR,"out of memory");
	do_nak(flags->remotefd,EUNDEF,"out of memory");
	_exit(1); return 0; /* supress useless warning */
}

/* this is almost the same than try_sccs_get,
 * but with slight differences in file name
 * mangling and options.
 */
static int
utftpd_rcs_co(int mode, struct utftpd_ctrl *flags)
{
	const char *slash;
	static uostr_t s=UOSTR_INIT;
	size_t pos;

	const char *er;
	int fds[2];
	pid_t pid;

	if (!opt_rcs_co || !opt_rcs_ci) return 0;
	slash=strrchr(flags->filename,'/');
	if (!uostr_dup_mem(&s,flags->filename,slash-flags->filename+1)) goto oom;
	pos=s.len;
	/* try to get an RCS file */
	if (!uostr_add_cstr(&s,"RCS/")) goto oom;
	if (!uostr_add_cstr(&s,slash+1)) goto oom;
	if (!uostr_add_mem(&s,",v",3)) goto oom; /* \0 */

	if (mode==TSG_READ && -1==pipe(fds)) {
		er=strerror(errno);
		syslog(LOG_ERR,"pipe: %s",er);
		do_nak(flags->remotefd,EUNDEF,er);
		_exit(1);
	}

	pid=fork();
	if (pid==0) {
		union {const char **a1; char *const *a2;} d;
		const char *argv[10]; /* 6 */
		int oc=0;
		uostr_cut(&s,pos); uostr_add_mem(&s,"",1);
		if (-1==chdir(s.data)) _exit(1);
		if (-1==dup2(nullfd,2)) _exit(1);
		if (mode == TSG_READ && -1==dup2(fds[1],1)) _exit(1);
		argv[oc++]="co";
		/* check out a specific revision only if it not for a "write" request */
		if (mode==TSG_WRITE) argv[oc++]="-l";
		else if (flags->revision) {
			static uostr_t rev=UOSTR_INIT;
			if (!uostr_add_cstr(&rev,"-r")) _exit(1);
			if (!uostr_add_cstr(&rev,flags->revision)) _exit(1);
			if (!uostr_add_mem(&rev,"",1)) _exit(1);
			argv[oc++]=rev.data;
		}
		if (mode==TSG_READ) {
			argv[oc++]="-p";
			close(fds[0]);
		}
		argv[oc++]=strdup(slash+1);
		if (!argv[oc-1]) _exit(1);
		argv[oc]=0;
		d.a1=argv;
		execv(opt_rcs_co,d.a2);
		_exit(1);
	} else if (pid==-1) {
		er=strerror(errno);
		syslog(LOG_ERR,"can't fork: %s",er);
		do_nak(flags->remotefd,EUNDEF,er);
		_exit(1);
	} else  {
		if (mode==TSG_READ) {
			flags->pid=pid;
			flags->filefd=fds[0];
			close(fds[1]);
		} else {
			pid=waitpid(pid,0,0);
#define ADD_ON 0
#ifndef HAVE_FSYNC
# ifdef (O_SYNC)
#  undef ADD_ON
#  define ADD_ON O_SYNC
# endif
#endif
			flags->filefd=open(flags->filename,O_WRONLY|O_TRUNC|ADD_ON,0644);
			if (flags->filefd!=-1) return 0;
			syslog(LOG_ERR,"version control system failed, cannot open %s: %s",flags->filename,strerror(errno));
			do_nak(flags->remotefd,EUNDEF,"version control system failed");
			_exit(1);
		}
	}
	
	return 0;
  oom:
  	syslog(LOG_ERR,"out of memory");
	do_nak(flags->remotefd,EUNDEF,"out of memory");
	_exit(1); return 0; /* suppress useless warning */
}

struct utftpd_vc utftpd_rcs={
	utftpd_rcs_test,
	utftpd_rcs_ci,
	utftpd_rcs_co,
	NULL
};

