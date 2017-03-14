/* utftpd: the tftp daemon */

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
#include "timing.h"
#include "nonblock.h"
#include "str2num.h"
#include "uostr.h"
#include "uoio.h"
#include "cdb.h"
#include "sys/wait.h"
#include "wildmat.h"
#include "utftpd.h"
#include "str_ulong.h"
#include "uosock.h"

#ifndef errno
extern int errno;
#endif

int nullfd;

#define TFTP_OFFSET 4
static struct utftpd_ctrl *ctrl;

static uostr_t rules=UOSTR_INIT;
ssize_t got;

static char *opt_configfile;
static char *opt_global_chdir;
#ifdef HAVE_CHROOT
static char *opt_global_chroot;
#endif
static char *opt_global_uid;
static char **opt_old_style_ac;
int opt_verbose;

/* from configfile */
static char *opt_chdir;
static char *opt_chroot;
static char *opt_write;
static char *opt_create;
static char *opt_read;
static char *opt_uid;
char *opt_rcs_ci;
char *opt_rcs_co;
char *opt_sccs_get;
char *opt_sccs_unget;
char *opt_sccs_clean;
char *opt_sccs_delta;
static int opt_syslog; /* "-l" option of inetutils tftpd. This is just a dummy variable */
static int opt_keeprun;
static const char *opt_standalone;
int opt_suppress_naks;
int opt_timing;

static const char *need_commit; /* a filename */
static char *argv0;

char *remoteip=NULL;
static struct utftpd_ctrl *
	utftpd_setup_ctrl(struct utftpd_ctrl *old, size_t segsize);

static uogetopt_t myopts[]={
	{'c',"config-file",   UOGO_STRING,&opt_configfile,0,
	 "Specify location of configuration file.\n"
	 "This has to be a cdb file, created by utftpd_make.\n"
	 "It will be opened before a chroot, if any.","CDBFILE"},
#ifdef HAVE_CHROOT
	{'C',"global-chroot",   UOGO_STRING,&opt_global_chroot,0,
	/*12345678901234567890123456789012345678901234567890 */
	 "Chroot to DIRECTORY before reading configuration.\n"
	 "This will change the working directory, too.","DIR"},
#endif
	{'d',"global-chdir",   UOGO_STRING,&opt_global_chdir,0,
	 "Chdir to DIRECTORY before reading configuration.\n"
	 "This will happen _after_ the chroot.", "DIR"},
	{'k',"keeprunning",   UOGO_FLAG,&opt_keeprun,1,
	 "Accept other request until all are finished",0},
	{'l',0,   UOGO_FLAG,&opt_syslog,1,
	 "Log all request to syslog() (active by default).\n"
	 "This option is for compatability",0},
	{'n',"suppress-naks",   UOGO_FLAG,&opt_suppress_naks,1,
	 "Suppress NAKs for nonexistant files.\n"
	/*12345678901234567890123456789012345678901234567890 */
	 "Don't send ACK on read requests for nonexistant\n"
	 "relative filenames.",0},
	{'s',"standalone",   UOGO_STRING,&opt_standalone,1,
	 "Don't use inetd.\n"
	 "Needs an address and port to bind to:\n"
	 "  HOST:PORT -> bind to address HOST, port PORT\n"
	 "  HOST      -> bind to address HOST, port 69\n"
	 "  HOST:     -> same\n"
	 "  :PORT     -> bind to IPADDR_ANY, port PORT\n"
	 "  :         -> bind to IPADDR_ANY, port 69\n" ,"ADDRESS:PORT"},
	{'t',"timing",   UOGO_FLAG,&opt_timing,1,
	 "Log transfer rates to syslog().",0},
	{'u',"global-uid",   UOGO_STRING,&opt_global_uid,0,
	 "Change user id before reading configuration.\n"
	 "Use \"user.group\" to spefify a group.\n"
	 "Use \".group\" to specify only a group.\n"
	 "Numerical ids are allowed, as are names."
	 ,0},
	{'v',"verbose",   UOGO_FLAG,&opt_verbose,1,
	 "Be verbose to syslog().",0},
	{  0,"copyright", UOGO_HIDDEN|UOGO_PRINT,0,0,
	 "Copyright (C) 1999 Uwe Ohse\n"
	 PACKAGE " comes with NO WARRANTY,\n"
	 "to the extent permitted by law.\n"
	 "You may redistribute copies of " PACKAGE "\n"
	 "under the terms of the GNU General Public License.\n"
	 "For more information about these matters,\n"
	 "see the files named COPYING.\n"
	 ,0},
	{0,0,0,0,0,0,0}
};

static struct utftpd_vc *
find_vc(const char *filename, struct utftpd_ctrl *flags)
{
	if (0==utftpd_sccs.test(filename,flags)) return &utftpd_sccs;
	if (0==utftpd_rcs.test(filename,flags)) return &utftpd_rcs;
	return &utftpd_novc;
}

static char *
rule_lookup(const char *tag)
{
	char *s;
	size_t tl=strlen(tag);
	s=rules.data;
	while (*s) {
		char *nul;
		char *eq;
		char *q;
		char *end;
		int found=0;
		end=rules.data+rules.len;
		eq=s;
		while (eq!=end && *eq!='=') eq++;
		if (eq==end) goto bad_format;
		nul=eq+1;
		while (nul!=end && *nul!='\0') nul++;
		if (nul==end) goto bad_format;
		q=strstr(s,tag);
		if (q && (q==s || q[-1]==',') && (q[tl]=='=' || q[tl]==','))
			found=1;
		if (found) {
			char *v;
			v=(char *) malloc(nul-eq);
			if (!v) goto oom;
			memcpy(v,eq+1,nul-eq);
			if (opt_verbose)
				syslog(LOG_INFO,"CONFIG %s=%s",tag,v);
			return v;
		}
		s=nul+1;
		if (s==end) break;
	}
	return NULL;
  bad_format:
	syslog(LOG_ERR,"bad config file %s",opt_configfile);
	_exit(1);
  oom:
	syslog(LOG_ERR,"out of memory");
	_exit(1);
}

static void
do_config(int config_fd)
{
	int did_default=0;
	uostr_t a;
	a.data=0;
	if (!uostr_dup_cstr(&a,remoteip)) goto oom;

	/* try 1.2.3.4 1.2.3. 1.2. 1. */
	while (a.len || !did_default) {
		uint32 dlen32;
		unsigned int datalen;
		if (!a.len)
			did_default=0;
		switch(cdb_seek(config_fd,
			(const unsigned char *) (a.len ? a.data : "default"),
			a.len ? a.len : 7,
			&dlen32)) {
		case -1: goto some_error;
		case 0:  break;
		default:
			datalen = dlen32;
			if (!uostr_needmore(&rules,datalen)) goto oom;
			if (cdb_bread(config_fd,
				(unsigned char *)(rules.data+rules.len),
				datalen) != 0) goto some_error;
			rules.len+=datalen;
			break;
		}
		if (!a.len)
			break;
		a.len--;
		while (a.len)  {
			if (a.data[a.len-1]=='.')
				break;
			a.len--;
		}
	}
	if (!rules.data) 
		return;
	if (!uostr_add_mem(&rules,"",1)) goto  oom;
	opt_uid=rule_lookup("uid");
	opt_chroot=rule_lookup("root");
	opt_chdir=rule_lookup("dir");
	opt_read=rule_lookup("read");
	opt_write=rule_lookup("write");
	opt_create=rule_lookup("create");
	opt_sccs_clean=rule_lookup("sccs-clean"); if (opt_sccs_clean && !*opt_sccs_clean) opt_sccs_clean=0;
	opt_sccs_unget=rule_lookup("sccs-unget"); if (opt_sccs_unget && !*opt_sccs_unget) opt_sccs_unget=0;
	opt_sccs_get=rule_lookup("sccs-get"); if (opt_sccs_get && !*opt_sccs_get) opt_sccs_get=0;
	opt_sccs_delta=rule_lookup("sccs-delta"); if (opt_sccs_delta && !*opt_sccs_delta) opt_sccs_delta=0;
	opt_rcs_ci=rule_lookup("rcs-ci"); if (opt_rcs_ci && !*opt_rcs_ci) opt_rcs_ci=0;
	opt_rcs_co=rule_lookup("rcs-co"); if (opt_rcs_co && !*opt_rcs_co) opt_rcs_co=0;
	return;
  some_error:
	syslog(LOG_ERR,"error reading %s: %s",opt_configfile,strerror(errno));
	_exit(1);
  oom:
	syslog(LOG_ERR,"out of memory");
	_exit(1);
}

static void
get_ids(char *str,uid_t *puid, gid_t *pgid)
{
	uid_t uid;
	unsigned long ul;
	char *dot;
	*pgid=(uid_t) -1; /* magic */
	*puid=(gid_t) -1; /* magic */
	dot=strchr(str,'.');
	if (!dot) dot=strchr(str,':');
	if (dot) {
		gid_t gid;
		*dot++=0;
		if (0>=str2ulong(dot,&ul,0)) {
			struct group * g;
			g=getgrnam(dot);
			if (!g) {
				syslog(LOG_ERR,"%s is not a valid group name",dot);
				_exit(1);
			}
			gid=g->gr_gid;
		} else {
			gid=(gid_t) ul;
			if ((unsigned long) gid != ul) {
				syslog(LOG_ERR,"%s is not a valid group number",dot);
				_exit(1);
			}
		}
		*pgid=gid;
	}
	if (!*str) /* .gid */
		return;

	if (0>=str2ulong(str,&ul,0)) {
		struct passwd * pw;
		pw=getpwnam(str);
		if (!pw) {
			syslog(LOG_ERR,"%s is not a valid user name",str);
			_exit(1);
		}
		uid=pw->pw_uid;
	} else {
		uid=(uid_t) ul;
		if ((unsigned long) uid != ul) {
			syslog(LOG_ERR,"%s is not a valid user number",str);
			_exit(1);
		}
	}
	*puid=uid;
}

static void
set_ids(uid_t uid, gid_t gid)
{
	if (gid != (gid_t) -1 && -1==setgid(gid)) {
		syslog(LOG_ERR,"setgid(%lu): %s",(unsigned long) gid, strerror(errno));
		_exit(1);
	}
	if (uid!=(uid_t) -1 && -1==setuid(uid)) {
		syslog(LOG_ERR,"setuid(%lu): %s",(unsigned long) uid, strerror(errno));
		_exit(1);
	}
}

void
do_nak(int peer, int ec, const char *et)
{
	utftpd_nak(peer,ec,et,ctrl);
}

static char *
qualify(const char *prefix, const char *filename)
{
	char *n;
	size_t l1,l2;
	l1=strlen(prefix);
	l2=strlen(filename);
	n=(char *) malloc(l1+l2+2);
	if (!n) return 0;
	memcpy(n,prefix,l1);
	if (prefix[l1-1]!='/')
		n[l1++]='/';
	memcpy(n+l1,filename,l2+1);
	return n;
}

static const char *
qualify_filename(int peer, int opcode, const char *filename)
{
	const char *old_filename=filename;
	/* first qualify the filename, if it doesn't start with a / */
	/* this is actually very simple, although it looks somewhat complicated:
	 * prefix with opt_chdir (from configfile), and if that's still not enough:
	 * prefix with opt_global_chdir, if we didn't chroot since then 
	 */
	if (*filename!='/' && opt_chdir) {
		filename=qualify(opt_chdir,filename);
		if (!filename) goto oom;
	}
	if (*filename!='/' && opt_global_chdir && !opt_chroot) {
		filename=qualify(opt_global_chdir,filename);
		if (!filename) goto oom;
	}
	if (*filename!='/') {
		if (!opt_configfile) {
			char **a;
			uostr_t s;
			/* traditional mode */
			if (opcode!=RRQ) {
				syslog(LOG_ERR,"denied write access to relative filename");
				do_nak(peer, EACCESS,"denied write access to relative filename");
				_exit(1);
			}
			/* walk through all directories trying to find that file */
			s.data=0;
			a=opt_old_style_ac;
			while (*a) {
				struct stat st;
				if (!uostr_dup_cstr(&s,*a)) goto oom;
				if (!uostr_add_mem(&s,"/",1)) goto oom;
				if (!uostr_add_cstr(&s,filename)) goto oom;
				if (!uostr_add_mem(&s,"",1)) goto oom;
				if (0==stat(s.data,&st)) {
					if (S_ISREG(st.st_mode) && (st.st_mode & S_IROTH) != 0) {
						filename=s.data;
						return filename;
					}
					break;
				}
				a++;
			}
			syslog(LOG_ERR,"denied read access to relative filename");
			do_nak(peer, EACCESS,"access denied");
			_exit(1);
		}
		filename=qualify("/",filename);
		if (!filename) goto oom;
	}
	if (opt_verbose && filename!=old_filename) syslog(LOG_INFO,"qualified filename %s to %s",old_filename,filename);
	return filename;
  oom:
  	syslog(LOG_ERR,"out of memory");
	do_nak(peer, EUNDEF,"out of memory");
	_exit(1);
}

#define AC_READ 0
#define AC_WRITE 1
#define AC_CREATE 2
static struct utftpd_vc *
check_access(const char *dir,const char *filename, int mode)
{
	int ok=0;
	struct utftpd_vc *vc;
	/* prevent simple trickery */
	if (strstr(filename,"/../")) goto deny;
	if (!dir && !opt_configfile) {
		/* old style compatibility */
		char **a=opt_old_style_ac;
		/* /xx will allow /xx/y, but not /xxy */
		while (*a) {
			size_t l=strlen(*a);
			if (l==1 && **a=='/') break; /* "/" */
			if (0==strncmp(filename,*a,l) && filename[l]=='/') break;
			a++;
		}
		if (*a) {
			/* found it. now check access rights */
			struct stat st;
			if (stat(filename,&st)==-1) goto deny;
			if (!S_ISREG(st.st_mode)) goto deny;
			if (mode==AC_READ) {
				if (st.st_mode & S_IROTH) {
					utftpd_novc.checkout(TSG_READ,ctrl);
					return &utftpd_novc;
				}
			} else {
				if (st.st_mode & S_IWOTH) {
					utftpd_novc.checkout(TSG_WRITE,ctrl);
					return &utftpd_novc;
				}
			}
		}
		goto deny;
	}
	if (!dir || !*dir) /* means: no dir found in config file */
		goto deny;
	if (opt_verbose) syslog(LOG_INFO,"check access to %s against %s",filename,dir);
	/* go through the list of directories */
	while (dir) {
		char *dp=strchr(dir,':');
		size_t l;
		if (dp) {
			*dp=0;
			l=dp-dir;
		} else
			l=strlen(dir);
		/* cut of trailing / */
		while (l && dir[l-1]=='/') l--;
		if (0==l) {
			/* uah, access to root directory allowed. Hope admin chroot()ed */
			ok=1;
		} else {
			if (0==memcmp(filename,dir,l)) { /* "/xxx/Z" | "/xxxB/Z", "/xxxA" | "/xxx", 4|5 */
				if (filename[l]=='/' || filename[l]=='\0')
					ok=1;
			}
			if (!ok) ok=dir_wildmat(filename,dir);
		}
		if (ok) break;
		if (dp) dir=dp+1;
		else dir=0;
	}
	if (ok) {
		struct stat st;
		int ret;
		vc=find_vc(filename, ctrl);
		if (mode==AC_READ)  {
			vc->checkout(TSG_READ,ctrl);
			return vc;
		}
		ret=stat(filename,&st);
		if (mode==AC_CREATE) {
			if (ret==-1 && vc==&utftpd_novc) {
				vc->checkout(TSG_CREATE,ctrl);
				/* yup, that's create */
				return vc;
			}
			return 0; /* that would be a write */
		}
		if (ret!=-1 && vc!=&utftpd_novc) {
			/* writeable file? can't overwrite it, maybe some other process does
			 * also an upload now.
			 */
			if (st.st_mode & (S_IWOTH |S_IWGRP|S_IWUSR)) {
				syslog(LOG_ERR,"writeable %s exists -> DENY",filename);
				goto deny;
			}
			unlink(filename);
		}
		if (ret==-1 && vc==&utftpd_novc) {
			/* this would be an create */
			if (mode==AC_WRITE) return 0;
		}

		vc->checkout(TSG_WRITE,ctrl);
		need_commit=filename;
		return vc;
	}
 /* FALLTHROUGH */
  deny:
    return 0;
}

static void
oack_append(int peer, size_t *oack_length, const char *n, const char *v)
{	
	size_t l1,l2;
	l1=strlen(n)+1;
	l2=strlen(v)+1;
	if (*oack_length+l1+l2 >ctrl->segsize) {
		syslog(LOG_ERR,"OACK too large for new %s %s",n,v);
		utftpd_nak(peer,EUNDEF,"OACK too large",ctrl);
		_exit(1);
	}
	memcpy(ctrl->sendbuf.buf+TFTP_OFFSET+*oack_length,n,l1);
	memcpy(ctrl->sendbuf.buf+TFTP_OFFSET+*oack_length+l1,v,l2);
	*oack_length+=l1+l2;
}

static void
do_init(int peer)
{
	char *filename;
	char *convert;
	char *p;
	struct tftphdr *hdr;
	size_t oack_length=0;
	size_t new_blksize=512;
	const char *realfilename;
	short opcode;
	int got_options=0;
	hdr = ctrl->recvbuf.hdr;
	opcode = ntohs(hdr->th_opcode);
	if (opcode != RRQ && opcode != WRQ) {
		syslog(LOG_ERR,"got opcode %d in init",(int) opcode);
		_exit(1);
	}
	filename=((char *)&hdr->th_opcode)+2; /* slowlartis doesn't have th_u union */
	/* proceed to end of filename */
	p=filename;
	while (p!=ctrl->recvbuf.buf+got && *p!=0) p++;
	if (p==ctrl->recvbuf.buf+got) {
		syslog(LOG_ERR,"unterminated filename in init packet");
		_exit(1);
	}

	convert=p+1;
	/* proceed to end of conversion */
	p=convert;
	while (p!=ctrl->recvbuf.buf+got && *p!=0) p++;
	if (p==ctrl->recvbuf.buf+got) {
		syslog(LOG_ERR,"unterminated conversion in init packet");
		_exit(1);
	}
	if (opcode==RRQ) {
		syslog(LOG_INFO,"peer requests %s, conversion %s", filename, convert);
	} else {
		syslog(LOG_INFO,"peer sends %s, conversion %s", filename, convert);
	}

	/* now there may be options */
	p++;
	while (p!=ctrl->recvbuf.buf+got) {
		char *opt;
		char *val;
		opt=p;
		/* proceed to end of option name */
		while (p!=ctrl->recvbuf.buf+got && *p!=0) p++;
		if (p==ctrl->recvbuf.buf+got) {
			if (got_options) {
				syslog(LOG_ERR,"unterminated option name in init packet");
				utftpd_nak(peer,EUNDEF,"unterminated option name in init packet",ctrl);
				_exit(1);
			} else {
				syslog(LOG_INFO,"ignored unterminated option name in init "
					"packet. bad client implementation?");
				break;
			}
		}
		got_options++;
		val=p+1;
		p=val;
		/* proceed to end of option name */
		while (p!=ctrl->recvbuf.buf+got && *p!=0) p++;
		if (p==ctrl->recvbuf.buf+got) {
			syslog(LOG_ERR,"unterminated option value in init packet");
			utftpd_nak(peer,EUNDEF,"unterminated option value in init packet",
				ctrl);
			_exit(1);
		}
		if (opt_verbose)
			syslog(LOG_INFO,"peer requests option %s, value %s", opt, val);
		p++;
		if (0==strcasecmp(opt,"blksize")) {
			unsigned long x;
			/* the actual resizing of the buffers is done later, we know parse a set of strings 
			 * inside those buffers.
			 * ignore illegal numbers or blksizes below 512 (which can't work). 
			 */
			if (-1!=str2ulong(val,&x,0) && x >= 512) {
				if (x>=65464) {
					x=65464;
					if (opt_verbose) syslog(LOG_INFO,"changed that to %lu bytes", x);
				}
				new_blksize=x;
			}
		} else if (0==strcasecmp(opt,"timeout")) {
			unsigned long x;
			/* must be between 1 and 255 inclusivly */
			if (-1!=str2ulong(val,&x,0) && x>=1 && x<=256) {
				ctrl->timeout=x;
				oack_append(peer,&oack_length, opt,val);
			}
		} else if (0==strcasecmp(opt,"revision")) {
			size_t x=strlen(val)+1;
			/* we will NACK that later, if that revision doesn't exist */
			ctrl->revision=(char *) malloc(x);
			if (!ctrl->revision) goto oom;
			memcpy(ctrl->revision,val,x);
			oack_append(peer,&oack_length, opt,val);
		}
	}
	filename=strdup(filename);
	convert=strdup(convert);
	if (!filename || !convert) goto oom;

	/* stop using "hdr" now: it will move. */

	if (new_blksize != 512) {
		struct utftpd_ctrl *n;
		char buf[STR_ULONG];
		n=utftpd_setup_ctrl(ctrl, new_blksize);
		if (!n) goto oom;
		ctrl=n;
		str_ulong(buf,new_blksize);
		/* note: this also checks wether the buffer now is to small to hold the old OACK values */
		oack_append(peer,&oack_length,"blksize",buf);
	}
	if (0==strcasecmp(convert,"netascii")) {
		ctrl->netascii=1;
	} else if (0==strcasecmp(convert,"octet")) {
		ctrl->netascii=0;
	} else {
		syslog(LOG_ERR,"unknown conversion -> NAK");
		utftpd_nak(peer,EUNDEF,"unknown conversion",ctrl);
		_exit(1);
	}

	realfilename=qualify_filename(peer, opcode, filename);
	ctrl->filename=strdup(realfilename);
	if (!ctrl->filename) goto oom;
	ctrl->origfilename=filename;
	if (opcode==RRQ) {
		int ret;
		ctrl->vc=check_access(opt_read,realfilename,AC_READ);
		if (!ctrl->vc) {
			syslog(LOG_NOTICE,"denied read access to %s", filename);
			do_nak(peer, EACCESS,"access denied");
			_exit(1);
		}
		ctrl->first_packet_length=oack_length;
		ctrl->sendbuf.hdr->th_opcode=htons(OACK);
		ret=utftpd_send(ctrl);
		if (opt_timing) {
			unsigned long used=stop();
			char buf[STR_ULONG];
			unsigned long x,y;
			x=str_ulong(buf,used/1000000);
			buf[x++]='.';
			y=used%1000000;
			if (y>99999) str_ulong(buf+x,y);
			else if (y>9999) {buf[x++]='0'; str_ulong(buf+x,y);}
			else if (y>999) {buf[x++]='0'; buf[x++]='0'; str_ulong(buf+x,y);}
			else if (y>99) {buf[x++]='0'; buf[x++]='0'; buf[x++]='0'; str_ulong(buf+x,y);}
			else if (y>9) {buf[x++]='0'; buf[x++]='0'; buf[x++]='0'; buf[x++]='0'; str_ulong(buf+x,y);}
			else {buf[x++]='0'; buf[x++]='0'; buf[x++]='0'; buf[x++]='0'; buf[x++]='0'; str_ulong(buf+x,y);}

			syslog(LOG_INFO,"transferred %lu bytes in %s seconds", ctrl->bytes,buf);
		}
		_exit (0!=ret);
	} else {
		int retcode;

		/* if we are root we never write. */
		if (getuid()==0) {
			syslog(LOG_ERR,"%s: will not write as root",realfilename);
			do_nak(peer, EACCESS,"access denied");
			_exit(1);
		}

		ctrl->vc=check_access(opt_write,realfilename,AC_WRITE);
		if (!ctrl->vc) {
			ctrl->vc=check_access(opt_create,realfilename,AC_CREATE);
			if (!ctrl->vc) {
				syslog(LOG_NOTICE,"denied write access to %s", filename);
				do_nak(peer, EACCESS,"access denied");
				_exit(1);
			}
		}

		/* we have to send either an OACK or an ACK. We prepare that here, but sending (and possible resending)
		 * is done in the receiving code. 
		 */
		ctrl->first_packet_length=oack_length;
		if (oack_length) 
			ctrl->sendbuf.hdr->th_opcode=htons(OACK);
		else
			ctrl->sendbuf.hdr->th_opcode=htons(ACK);

		
		retcode=utftpd_recv(ctrl);
		if (retcode==-1) {
			if (need_commit && ctrl->vc->unget)
				ctrl->vc->unget(ctrl);
			unlink(ctrl->filename);
			_exit(1); 
		}
		/* we _do_ have the whole file now, but didn't send the last ACK.
		 * we delay that until we did the version control checkin
		 */
		if (need_commit) {
			ctrl->vc->commit("got from remote",ctrl);
		}

		syslog(LOG_INFO,"got %s",realfilename);
		if (send(peer, ctrl->sendbuf.buf, TFTP_OFFSET, 0) != (ssize_t) TFTP_OFFSET) {
			syslog(LOG_ERR,"send() for final ACK failed for %s: %s",remoteip,strerror(errno));
			/* minor problem, so we return 0 */
		}
		if (opt_timing) {
			unsigned long used=stop();
			char buf[STR_ULONG];
			unsigned long x,y;
			x=str_ulong(buf,used/1000000);
			buf[x++]='.';
			y=used%1000000;
			if (y>99999) str_ulong(buf+x,y);
			else if (y>9999) {buf[x++]='0'; str_ulong(buf+x,y);}
			else if (y>999) {buf[x++]='0'; buf[x++]='0'; str_ulong(buf+x,y);}
			else if (y>99) {buf[x++]='0'; buf[x++]='0'; buf[x++]='0'; str_ulong(buf+x,y);}
			else if (y>9) {buf[x++]='0'; buf[x++]='0'; buf[x++]='0'; buf[x++]='0'; str_ulong(buf+x,y);}
			else {buf[x++]='0'; buf[x++]='0'; buf[x++]='0'; buf[x++]='0'; buf[x++]='0'; str_ulong(buf+x,y);}

			syslog(LOG_INFO,"received %lu bytes in %s seconds", ctrl->bytes,buf);
		}
		_exit(0);
	}
  oom:
	syslog(LOG_ERR,"out of memory");
	utftpd_nak(peer,EUNDEF,"out of memory",ctrl);
	_exit(1);
}

static void
chld_handler(int x)
{
	(void) x;
	signal(SIGCHLD,&chld_handler);
}

int 
main(int argc, char **argv)
{
	int config_fd=-1;
	struct	sockaddr_in from;
	struct	sockaddr_in s_in;
	socklen_t	fromlen;
	pid_t pid;
	uid_t guid,uid;
	gid_t ggid,gid;
	size_t l;
	const char *s;
	int initial_packet_errno;
	int peer;
	struct in_addr ina;
	unsigned short port;
	int mastersocket=0;

	nullfd=open("/dev/null",O_RDWR);
	if (nullfd==-1) {
		syslog(LOG_ERR,"cannot open /dev/null: %s",strerror(errno));
		_exit(1);
	}
#ifdef HAVE_LIBEFENCE
	/* hack around efence */
	{int fd2;void *waste;
	 if (-1==(fd2=dup(2))) _exit(1);
	 if (-1==(dup2(nullfd,2))) _exit(1);
	 waste=malloc(1);
	 if (-1==(dup2(fd2,2))) _exit(1);
	 close(fd2);
	}
#endif

	argv0=strrchr(argv[0],'/');
	if (!argv0++) argv0=argv[0];
	ctrl=utftpd_setup_ctrl(0,512); 
	if (!ctrl)  goto oom;


	/* at least read input even in case we crash / exit very early due to a 
	 * configuration or resource problem. We have to to that
	 */
	fromlen = sizeof (from);
	got = recvfrom(mastersocket, ctrl->recvbuf.buf, ctrl->segsize+TFTP_OFFSET, 0,
	    (struct sockaddr *)&from, &fromlen);
	initial_packet_errno=errno;

	uogetopt("utftpd","GNU " PACKAGE,VERSION,&argc,argv,uogetopt_out,0,myopts,
		"Report bugs to uwe-utftpd@bulkmail.ohse.de");
	/* in standalone mode it's okay to lose here, nobody gave us a socket on fd 0 */
	if (opt_standalone 
#ifdef ENOTSOCK /* BEOS doesn't know this */
		&& got==-1 && errno==ENOTSOCK
#endif
		) {
		got=0;
		initial_packet_errno=0;
	}

	openlog("utftpd", LOG_PID|LOG_NDELAY, LOG_DAEMON);
	/* open configuration file now */
	if (opt_configfile) {
		config_fd=open(opt_configfile, O_RDONLY);
		if (config_fd==-1) {
			syslog(LOG_ERR,"cannot open %s: %s", opt_configfile, strerror(errno));
			_exit(1);
		}
	} else {
		opt_old_style_ac=argv; 
	}

	/* parse host:port before the chroot happens. No /etc/services and so on afterwards */
	if (opt_standalone) {
		int r;
		port=htons(69);
		ina.s_addr=INADDR_ANY;
		r=parse_ip_port(opt_standalone,&ina,&port,"udp");
		if (r==-1) {
			syslog(LOG_ERR,"cannot understand %s", opt_standalone);
			_exit(1);
		}
		mastersocket=socket(AF_INET,SOCK_DGRAM, 0);
		if (mastersocket < 0) { syslog (LOG_ERR, "socket: %s", strerror (errno)); _exit (1); }
		s_in.sin_addr.s_addr=ina.s_addr;
		s_in.sin_port=port;
		s_in.sin_family=AF_INET;
		if (bind (mastersocket, (struct sockaddr *) &s_in, sizeof (s_in)) < 0) { 
			syslog (LOG_ERR, "bind: %s", strerror (errno)); _exit (1);
		}
	}

	/* now get rid of rights ... */
	if (opt_global_uid) get_ids(opt_global_uid,&guid,&ggid);
#ifdef HAVE_CHROOT
	if (opt_global_chroot) {
		if (-1==chdir(opt_global_chroot)) { syslog(LOG_ERR,"chdir %s: %s",opt_global_chroot,strerror(errno)); _exit(1); }
		if (-1==chroot(opt_global_chroot)) { syslog(LOG_ERR,"chroot %s: %s",opt_global_chroot,strerror(errno)); _exit(1); }
		if (opt_verbose) syslog(LOG_INFO,"did chdir/root to %s",opt_global_chroot);
	}
#endif
	if (opt_global_uid) set_ids(guid,ggid);
	if (opt_global_chdir) {
		if (-1==chdir(opt_global_chdir)) { syslog(LOG_ERR,"chdir %s: %s",opt_global_chdir,strerror(errno)); _exit(1); }
		if (opt_verbose) syslog(LOG_INFO,"did chdir to %s",opt_global_chdir);
	}


#ifndef __BEOS__
	/* why did i do this? */
	nonblock(0,1); /* it's a UDP socket - this should not be needed! */
#endif

	if (got < 0) {
#ifdef ENOTSOCK /* BEOS doesn't know this */
		 /* try to be nice to the user */
		if (got==-1 && errno==ENOTSOCK) {
			const char *m1,*m2;
			m1=": must be started by inetd.\n";
			m2="  Give --help option for usage information.\n";
			write(2,argv0,strlen(argv0));
			write(2,m1,strlen(m1));
			write(2,m2,strlen(m2));
			_exit(2);
		}

#endif
		syslog(LOG_ERR, "recvfrom(initial packet): %s",
			strerror(initial_packet_errno));
		exit(1);
	}

	/* fork/exit for inetd nowait mode */
	if (!opt_standalone) {
		pid = fork();
		if (pid==-1) {
			syslog(LOG_ERR,"can't fork: %s", strerror(errno));
			_exit(1);
		}
	}
	if (opt_standalone || pid!=0) {
		int childs;
		if (opt_standalone) childs=0;
		else childs=1;
		/* mother */
		if (!opt_keeprun && !opt_standalone)
			_exit(0);
		signal(SIGCHLD,&chld_handler);
		while (1) {
			fd_set set;
			int r;
			FD_ZERO(&set);
			FD_SET(mastersocket,&set);
			r=select(mastersocket+1,&set,0,0,0);
			if (r==-1 && errno!=EINTR) {
				syslog(LOG_ERR,"select failed: %s", strerror(errno));
				_exit(1);
			}
			if (FD_ISSET(mastersocket,&set)) {
				fromlen = sizeof (from);
				got = recvfrom(mastersocket, ctrl->recvbuf.buf, ctrl->segsize+TFTP_OFFSET, 0,
					(struct sockaddr *)&from, &fromlen);
				if (got==-1) {
					syslog(LOG_ERR, "recvfrom(initial packet on new connection): %s", strerror(errno));
				} else {
					pid=fork();
					if (pid==0)
						break; /* child */
					if (pid!=-1)
						childs++;
				}
			}
			while (r==-1) {
				pid_t pi=waitpid(WAIT_ANY,0,WNOHANG);
				if (pi==0 || pi==-1)
					break;
				childs--;
			}
			if (!childs && opt_keeprun) _exit(0);
		}
	}
	/* child */
	if (opt_timing) start();
	/* make a copy of the remote ip address, as string */
	s=inet_ntoa(from.sin_addr);
	l=strlen(s)+1;
	remoteip=(char *) malloc(l);
	if (!remoteip) goto oom;
	memcpy(remoteip,s,l);

	openlog("utftpd", LOG_PID|LOG_NDELAY, LOG_DAEMON);
	syslog(LOG_INFO,"connect from %s",remoteip);

	if (config_fd!=-1) {
		do_config(config_fd);
		close(config_fd);
	}

	from.sin_family = AF_INET;
	alarm(0);
	if (-1==dup2(nullfd,0)) { syslog(LOG_ERR,"cannot dup2 /dev/null -> 0: %s",strerror(errno)); _exit(1); }
	if (-1==dup2(nullfd,1)) { syslog(LOG_ERR,"cannot dup2 /dev/null -> 1: %s",strerror(errno)); _exit(1); }

	peer = socket(AF_INET, SOCK_DGRAM, 0);
	if (peer < 0) { syslog(LOG_ERR, "socket: %s",strerror(errno)); _exit(1); }
	ctrl->remotefd=peer;

	/* we need to bind to the same port we received this on ... */
	s_in.sin_addr.s_addr=INADDR_ANY;
	s_in.sin_port=0;
	s_in.sin_family=AF_INET;
	if (bind(peer, (struct sockaddr *)&s_in, sizeof (s_in)) < 0) { syslog(LOG_ERR, "bind: %s",strerror(errno)); _exit(1); }

	/* now get rid of rights ... */
	if (opt_uid) get_ids(opt_uid,&uid,&gid);
	if (opt_chroot) {
#ifdef HAVE_CHROOT
		const char *e;
		if (-1==chdir(opt_chroot)) { e=strerror(errno); syslog(LOG_ERR,"chdir %s: %s",opt_chroot,e);_exit(1); }
		if (-1==chroot(opt_chroot)) { e=strerror(errno); syslog(LOG_ERR,"chroot %s: %s",opt_chroot,e); _exit(1); }
		if (opt_verbose) syslog(LOG_INFO,"did chdir/root to %s",opt_chroot);
#else
		syslog(LOG_ERR,"chroot is not available on this platform"); 
		/* XXX should send a nak, but socket is still not connected */
		_exit(1);
#endif
	}
	if (opt_uid) set_ids(uid,gid);
	if (opt_chdir) {
		const char *e;
		if (-1==chdir(opt_chdir)) { e=strerror(errno); syslog(LOG_ERR,"chdir %s: %s",opt_chdir,e); _exit(1); }
		if (opt_verbose) syslog(LOG_INFO,"did chdir to %s",opt_chdir);
	}
	if (connect(peer, (struct sockaddr *)&from, sizeof(from)) < 0) { syslog(LOG_ERR, "connect: %s",strerror(errno)); exit(1); }
	setsid();

	do_init(peer);
	_exit(1);
oom:
	syslog(LOG_ERR,"out of memory");
	_exit(1);
}


static struct utftpd_ctrl *
utftpd_setup_ctrl(struct utftpd_ctrl *old, size_t segsize)
{
	if (!old) {
		old=(struct utftpd_ctrl *)malloc(sizeof(struct utftpd_ctrl));
		if (!old) return 0;
		memset(old,0,sizeof(*old));
		old->segsize=512; /* default */
		old->netascii=0;
		old->timeout=5;
		old->retries=5;
		old->first_packet_length=0; /* tftp_open() adjusts this */
		old->filefd=-1;
		old->sendbuf.buf=(char *) malloc(old->segsize+TFTP_OFFSET);
		old->recvbuf.buf=(char *) malloc(old->segsize+TFTP_OFFSET);
		if (!old->sendbuf.buf || !old->recvbuf.buf) {
			free(old->sendbuf.buf);
			free(old->recvbuf.buf);
			free(old);
			return 0;
		}
	}
	if (segsize!=0 && old->segsize!=segsize) {
		void *v,*u;
		u=realloc(old->sendbuf.buf,segsize+TFTP_OFFSET); 
		v=realloc(old->recvbuf.buf,segsize+TFTP_OFFSET); 
		if (!u || !v) {
			if (u) free(u); else free(old->sendbuf.buf);
			if (v) free(v); else free(old->recvbuf.buf);
			free(old);
			return 0;
		}
		old->sendbuf.buf=(char *) u;
		old->recvbuf.buf=(char *) v;
		old->segsize=segsize;
	}
	return old;
}

