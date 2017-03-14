/* utftpd_sccs: sccs support functions */

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
#include "utftpd.h"
#include "str_ulong.h"

static ssize_t 
my_read (int fd, char *buf, size_t bytes)
{
	ssize_t got = 0;
	while (bytes) {
		ssize_t t = read (fd, buf, bytes);
		if (t == -1 && errno != EINTR) return -1;
		if (t == -1) continue;
		if (t==0) break;
		buf += t;
		bytes -= t;
		got += t;
	}
	return got;
}

static int
utftpd_sccs_delta(const char *comment, struct utftpd_ctrl *flags)
{
	const char *slash;
	static uostr_t s=UOSTR_INIT;
	static uostr_t t=UOSTR_INIT; /* tmp file */
	size_t pos;
	int target_fd;
	char numbuf[STR_ULONG];
	const char *er;
	pid_t pid;
	int is_identical=1;

	slash=strrchr(flags->filename,'/');
	if (!uostr_dup_mem(&s,flags->filename,slash-flags->filename+1)) goto oom;
	pos=s.len;
	/* try to get an sccs file */
	if (!uostr_add_cstr(&s,"SCCS/s.")) goto oom;
	if (!uostr_add_cstr(&s,slash+1)) goto oom;
	if (!uostr_add_mem(&s,"",1)) goto oom;

	if (!flags->revision) {
		/* first: get a copy of the last revision, to make sure we don't checkin the same stuff twice */
		/* but since we have a checked out copy here we need to use -p to put the output of sccs into a file, uah ... */
		/* so we create a temp file now */
		pid=getpid();
		if (!uostr_dup_mem(&t,flags->filename,slash-flags->filename+1)) goto oom;
		str_ulong(numbuf,(unsigned long)pid);
		if (!uostr_add_cstr(&t,"tmp.")) goto oom;
		if (!uostr_add_cstr(&t,numbuf)) goto oom;
		if (!uostr_add_cstr(&t,".")) goto oom;
		do {
			static int count=0;
			size_t pos_target=t.len;
			if (count==1000) {
				er=strerror(errno);
				syslog(LOG_ERR,"can't create temporary file: %s",er);
				do_nak(flags->remotefd,EUNDEF,"can't create temporary file");
				_exit(1);
			}
			str_ulong(numbuf,++count);
			if (!uostr_add_mem(&t,"",1)) goto oom;
			target_fd=open(t.data,O_RDWR|O_CREAT|O_EXCL,0600);
			if (target_fd==-1) uostr_cut(&t,pos_target);
		} while (target_fd==-1);
		unlink(t.data);

		pid=fork();
		if (pid==0) {
			union {const char **a1; char *const *a2;} d;
			const char *argv[4];
			int oc=0;
			argv[oc++]="get";
			argv[oc++]="-p";
			argv[oc++]=strdup(s.data+pos); /* SCCS/s.something */
			if (!argv[oc-1]) _exit(1);
			argv[oc]=0;

			uostr_cut(&s,pos); uostr_add_mem(&s,"",1);
			if (-1==chdir(s.data)) _exit(1);
			if (-1==dup2(target_fd,1)) _exit(1);
			if (-1==dup2(nullfd,2)) _exit(1);

			d.a1=argv;
			execv(opt_sccs_get,d.a2);
			_exit(1);
		} else if (pid==-1) {
			/* we will then not be able to fork for the checkin, too */
			er=strerror(errno);
			syslog(LOG_ERR,"can't fork: %s",er);
			do_nak(flags->remotefd,EUNDEF,er);
			_exit(1);
		} else {
			pid=waitpid(pid,0,0);
		}
		if (opt_sccs_clean || opt_sccs_unget) { /* that might tell us about NFS casualities. */
			char *buf1;
			char *buf2;
#define MYBUF 4096
			/* we got that file */
			int fd1;
			fd1=open(flags->filename,O_RDONLY);
			if (fd1==-1) {
				er=strerror(errno);
				syslog(LOG_ERR,"can't open %s: %s",flags->filename,er);
				do_nak(flags->remotefd,EUNDEF,er);
				_exit(1);
			}
			buf1=(char *)malloc(MYBUF); if (!buf1) goto oom;
			buf2=(char *)malloc(MYBUF); if (!buf2) goto oom;

			if (-1==lseek(target_fd,0,SEEK_SET)) {
				er=strerror(errno);
				syslog(LOG_ERR,"can't lseek in %s: %s",t.data,er);
				do_nak(flags->remotefd,EUNDEF,er);
				_exit(1);
			}

			while (1) {
				ssize_t got1;
				ssize_t got2;
				got1=my_read(fd1,buf1,MYBUF);
				if (got1==-1) {
					er=strerror(errno);
					syslog(LOG_ERR,"can't read %s: %s",flags->filename,er);
					do_nak(flags->remotefd,EUNDEF,er);
					_exit(1);
				}
				got2=my_read(target_fd,buf2,MYBUF);
				if (got2==-1) {
					er=strerror(errno);
					syslog(LOG_ERR,"can't read %s: %s",t.data,er);
					do_nak(flags->remotefd,EUNDEF,er);
					_exit(1);
				}
				if (got1!=got2 || 0!=memcmp(buf1,buf2,got1)) { is_identical=0; break; }
				if (got1==0) break; /* EOF */
			}
			close(fd1);
			close(target_fd);
			if (is_identical) {
				syslog(LOG_ERR,"received file %s is identical to repository version",flags->filename);
			}
		} else {
			/* no clean, no unget: treat it as different */
			is_identical=0;
		}
	} else {
		/* commit to a revision, so we check it in regardless of whether it is identical or not */
		is_identical=0;
	}

	/* second: check it in */
	pid=fork();
	if (pid==0) {
		static uostr_t co=UOSTR_INIT;
		union {const char **a1; char *const *a2;} d;
		int oc=0;
		const char *argv[5];
		char *s_file;
		s_file=strdup(s.data+pos); /* SCCS/s.something */
		if (!s_file) _exit(1);
		uostr_xdup_cstr(&co,"-yby utftpd for ");
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
		if (is_identical && opt_sccs_clean) {
			argv[oc++]="clean";
			argv[oc++]="-u";
			argv[oc++]=s_file;
			argv[oc]=0;
			d.a1=argv;
			execv(opt_sccs_clean,d.a2);
		} else if (is_identical && opt_sccs_unget) {
			argv[oc++]="unget";
			argv[oc++]="-s";
			argv[oc++]=s_file;
			argv[oc++]=0;
			d.a1=argv;
			execv(opt_sccs_unget,d.a2);
		} else {
			argv[oc++]="delta";
			argv[oc++]="-s";
			if (flags->revision) {
				static uostr_t rev=UOSTR_INIT;
				if (!uostr_add_cstr(&rev,"-R")) _exit(1);
				if (!uostr_add_cstr(&rev,flags->revision)) _exit(1);
				if (!uostr_add_mem(&rev,"",1)) _exit(1);
				argv[oc++]=rev.data;
			}
			argv[oc++]=co.data;
			argv[oc++]=s_file;
			argv[oc]=0;
			d.a1=argv;
			execv(opt_sccs_delta,d.a2);
		}
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
	exit(1);
}

static int
utftpd_sccs_get(int mode, struct utftpd_ctrl *flags)
{
	const char *slash;
	static uostr_t s=UOSTR_INIT;
	size_t pos;
	int fds[2];
	const char *er;
	pid_t pid;

	slash=strrchr(flags->filename,'/');
	if (!uostr_dup_mem(&s,flags->filename,slash-flags->filename+1)) goto oom;
	pos=s.len;
	/* try to get an sccs file */
	if (!uostr_add_cstr(&s,"SCCS/s.")) goto oom;
	if (!uostr_add_cstr(&s,slash+1)) goto oom;
	if (!uostr_add_mem(&s,"",1)) goto oom;

	if (mode==TSG_READ && -1==pipe(fds)) {
		er=strerror(errno);
		syslog(LOG_ERR,"pipe: %s",er);
		do_nak(flags->remotefd,EUNDEF,er);
		_exit(1);
	}

	pid=fork();
	if (pid==0) {
		union {const char **a1; char *const *a2;} d;
		const char *argv[10]; /* 6 used */
		int oc=0;
		char *s_file=strdup(s.data+pos); /* SCCS/s.something */
		if (!s_file) _exit(1);
		uostr_cut(&s,pos); uostr_add_mem(&s,"",1);
		if (-1==chdir(s.data)) _exit(1);
		if (mode==TSG_READ && -1==dup2(fds[1],1)) _exit(1);
		if (-1==dup2(nullfd,2)) _exit(1);
		argv[oc++]="get";
		/* in case we want to edit something we check out the latest revision. try_sccs_delta will commit
		 * to the right rev. 
		 */
		if (mode==TSG_WRITE) argv[oc++]="-e";
		else if (flags->revision) {
			static uostr_t rev=UOSTR_INIT;
			if (!uostr_add_cstr(&rev,"-r")) _exit(1);
			if (!uostr_add_cstr(&rev,flags->revision)) _exit(1);
			if (!uostr_add_mem(&rev,"",1)) _exit(1);
			argv[oc++]=rev.data;
		} 
		if (mode==TSG_READ) {
			argv[oc++]="-p"; /* print to stdout */
			close(fds[0]);
		}
		argv[oc++]=s_file;
		argv[oc]=0;
		d.a1=argv;
		execv(opt_sccs_get,d.a2);
		_exit(1); /* just in case */
	} else if (pid==-1) {
		er=strerror(errno);
		if (mode==TSG_READ) { close(fds[0]); close(fds[1]); }
		syslog(LOG_ERR,"can't fork: %s",er);
		do_nak(flags->remotefd,EUNDEF,er);
		_exit(1);
	} else {
		if (mode==TSG_READ) {
			close(fds[1]);
			flags->pid=pid;
			flags->filefd=fds[0];
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
			if (flags->filefd==-1) {
				syslog(LOG_ERR,"version control system failed, cannot open %s: %s",flags->filename,strerror(errno));
				do_nak(flags->remotefd,EUNDEF,"version control system failed");
				_exit(1);
			}
		}
	}
	return 0;
  oom:
  	syslog(LOG_ERR,"out of memory");
	do_nak(flags->remotefd,EUNDEF,"out of memory");
	exit(1);
}

static int
utftpd_sccs_test(const char *filename, struct utftpd_ctrl *flags)
{
	const char *slash;
	static uostr_t s=UOSTR_INIT;
	size_t pos;
	struct stat st;
	if (!opt_sccs_delta || !opt_sccs_get) return -1;

	slash=strrchr(filename,'/');
	if (!uostr_dup_mem(&s,filename,slash-filename+1)) goto oom;
	pos=s.len;
	/* try to get an sccs file */
	if (!uostr_add_cstr(&s,"SCCS/s.")) goto oom;
	if (!uostr_add_cstr(&s,slash+1)) goto oom;
	if (!uostr_add_mem(&s,"",1)) goto oom;
	if (0==stat(s.data,&st))
		return 0;
	return -1;
  oom:
  	syslog(LOG_ERR,"out of memory");
	do_nak(flags->remotefd,EUNDEF,"out of memory");
	exit(1);
}

static int
utftpd_sccs_unget(struct utftpd_ctrl *flags)
{
	const char *slash;
	static uostr_t s=UOSTR_INIT;
	size_t pos;
	pid_t pid;

	slash=strrchr(flags->filename,'/');
	if (!uostr_dup_mem(&s,flags->filename,slash-flags->filename+1)) goto oom;
	pos=s.len;
	if (!uostr_add_cstr(&s,"SCCS/s.")) goto oom;
	if (!uostr_add_cstr(&s,slash+1)) goto oom;
	if (!uostr_add_mem(&s,"",1)) goto oom;

	pid=fork();
	if (pid==0) {
		union {const char **a1; char *const *a2;} d;
		const char *argv[10]; /* 4 used */
		int oc=0;
		char *s_file;
		s_file=strdup(s.data+pos); /* SCCS/s.something */
		if (!s_file) _exit(1);
		uostr_cut(&s,pos); uostr_add_mem(&s,"",1);
		if (-1==chdir(s.data)) _exit(1);
		if (-1==dup2(nullfd,2)) _exit(1);
		if (opt_sccs_clean) {
			argv[oc++]="clean";
			argv[oc++]="-u";
			argv[oc++]=s_file;
			argv[oc]=0;
			d.a1=argv;
			execv(opt_sccs_clean,d.a2);
		} else if (opt_sccs_unget) {
			argv[oc++]="unget";
			argv[oc++]="-s";
			argv[oc++]=s_file;
			argv[oc++]=0;
			d.a1=argv;
			execv(opt_sccs_unget,d.a2);
		}
		_exit(1);
	}
	else {
		int fail=wait_check_and_log(pid);
		if (fail) {
			syslog(LOG_ERR,"version control system failed to unget");
			_exit(1);
		}
	}
	return 0;
  oom:
  	syslog(LOG_ERR,"out of memory");
	exit(1);
}

struct utftpd_vc utftpd_sccs={
	utftpd_sccs_test,
	utftpd_sccs_delta,
	utftpd_sccs_get,
	utftpd_sccs_unget
};
