/* tftp_send.c: the sending routine */

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
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include "no_tftp.h"

#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "timselsysdep.h"
#include "str2num.h"
#include "uostr.h"
#include "tftplib.h"

#define ERR_LOG1(x) do { const char *er=strerror(safe_errno); syslog(LOG_ERR,x,er); } while(1)
#define ERR_LOG2(x,y) do { const char *er=strerror(safe_errno); syslog(LOG_ERR,x,y,er); } while(1)
#define ERR_LOG3(x,y,z) do { const char *er=strerror(safe_errno); syslog(LOG_ERR,x,y,z,er); } while(1)
#define ES(x) do {int safe_errno=errno; { x } errno=safe_errno; } while(0)

static int got_sig;
static void 
sig_handler(int signo)
{ 
	got_sig=signo;
}

static ssize_t 
my_read(int fd, char *buf,size_t len)
{
	ssize_t got=0;
	while (len) {
		ssize_t t;
		t=read(fd,buf,len);
		if (t==-1 && errno==EINTR) continue;
		if (t==-1) return -1;
		if (t==0) break;
		buf+=t;
		len-=t;
		got+=t;
	}
	return got;
}


int 
tftp_send(const char *server, int port, const char *remotefilename, const char *localfilename, struct tftplib_ctrl *flags)
{
	struct sockaddr_in s_in;
	int remotefd;
	int fd;
	int blockno;
	int lastchar=0; /* in case of netascii */
	int may_get_oack=0;
	const char *errortext;
	struct sigaction sa,sa_old;
	int got_timeouts=0;
	size_t sendlength;
	int is_connected=0;
	int retcode=UTFTP_EC_UNDEF; /* undefined error */


	memset (&s_in, 0, sizeof (s_in));
	s_in.sin_family = AF_INET;
	remotefd = socket (AF_INET, SOCK_DGRAM, 0);
	if (remotefd < 0) { ES(syslog(LOG_ERR,"socket: %s",strerror(errno));); retcode=UTFTP_EC_LOCAL; goto err0; }
	if (bind (remotefd, (struct sockaddr *) &s_in, sizeof (s_in)) < 0)
	{ ES(syslog(LOG_ERR,"bind: %s",strerror(errno));); retcode=UTFTP_EC_LOCAL; goto err1; }

	fd=open(localfilename,O_RDONLY);
	if (fd==-1) {
		int e=errno;
		const char *cs=strerror(errno);
		syslog(LOG_ERR,"open(%s): %s",localfilename, cs);
		if (e==ENOENT) tftp_nak(remotefd,ENOTFOUND,cs,flags);
		if (e==EACCES) tftp_nak(remotefd,EACCESS,cs,flags);
		else tftp_nak(remotefd,EUNDEF,cs,flags);
		errno=e; 
		return 1;
	}

	sa.sa_handler = sig_handler;
	sa.sa_flags = 0;
	sigemptyset (&sa.sa_mask);
	sigaction (SIGALRM, &sa, &sa_old);

	sendlength=tftp_prepare_rq(WRQ, server, port, remotefilename, &s_in, &may_get_oack, flags);
	if (sendlength==0) goto err2;

	blockno=0;
	while (1) {
		ssize_t len;
		/* get data */
		if (sendlength) {
			len=sendlength;
			sendlength=0;
		} else {
			if (flags->netascii) {
			char c;
			/* LF -> CR LF, CR -> CR nul .... what a stupid thing :-) */
			for (len=0;len<(ssize_t) flags->segsize;len++) {
				if (lastchar=='\r') { c=0; lastchar = 0; }
				else if (lastchar =='\n') { c='\n'; lastchar = 0; }
				else {
						ssize_t got;
						got=my_read(fd,&c,1);
						if (got==-1) {
							errortext=strerror(errno);
							ES( syslog(LOG_ERR,"read(%s): %s",localfilename,errortext););
							retcode=UTFTP_EC_LOCAL;
							goto do_error_nak;
						}
						if (got==0) break;
						lastchar=c;
						if (c=='\n') c='\r';
					}
					flags->sendbuf.buf[TFTP_OFFSET+len]=c;
				}
			} else {
				len=my_read(fd,flags->sendbuf.buf+TFTP_OFFSET,flags->segsize);
				if (len==-1) {
					errortext=strerror(errno);
					ES(syslog(LOG_ERR,"read(%s): %s",localfilename,errortext););
					retcode=UTFTP_EC_LOCAL;
					goto do_error_nak;
				}
			}
			flags->sendbuf.hdr->th_opcode = htons((u_short)DATA);
			flags->sendbuf.hdr->th_block = htons((u_short)blockno);
			len+=TFTP_OFFSET;
		}
		while (1) {
			struct tftphdr *rhdr;
			ssize_t got;
			/* send to the peer */
			if (is_connected) {
				if (send(remotefd, flags->sendbuf.buf, len, 0) != len) {
					errortext=strerror(errno);
					ES(syslog(LOG_ERR,"send(): %s",errortext););
					retcode=UTFTP_EC_NETWORK;
					goto err2;
				}
			} else {
				if (sendto(remotefd, flags->sendbuf.buf, len, 0,(struct sockaddr *)&s_in,sizeof(s_in)) != len) {
					errortext=strerror(errno);
					ES(syslog(LOG_ERR,"send(): %s",errortext););
					retcode=UTFTP_EC_NETWORK;
					goto err2;
				}
			}
			alarm(flags->timeout);
			if (is_connected)
				got = recv(remotefd, flags->recvbuf.buf, flags->segsize+TFTP_OFFSET, 0);
			else {
				socklen_t sl=sizeof(s_in);
				got = recvfrom(remotefd, flags->recvbuf.buf, flags->segsize+TFTP_OFFSET, 0,(struct sockaddr *)&s_in,&sl);
				if (got>=0) {
					connect(remotefd,(struct sockaddr *)&s_in,sl);
					is_connected=1;
				}
			}
			if (got < 0) {
				if (errno==EINTR && ++got_timeouts<flags->retries) continue;
				ES(syslog(LOG_ERR, "recv: %s",strerror(errno)););
				retcode=UTFTP_EC_TIMEOUT;
				goto err2;
			}
			got_timeouts=0;

			rhdr=(struct tftphdr *)flags->recvbuf.hdr;
			rhdr->th_opcode = ntohs((u_short)rhdr->th_opcode);
			rhdr->th_block = ntohs((u_short)rhdr->th_block);
			if (rhdr->th_opcode == ERROR) {
				ES(syslog(LOG_ERR, "got ERROR"););
				retcode=UTFTP_EC_ERROR;
				errortext="got ERROR";
				goto err2;
			}
			if (rhdr->th_opcode==OACK)  {
				char *p;
				if (!may_get_oack) { 
					errortext="got unwanted OACK opcode";ES(syslog(LOG_ERR,errortext);); 
					retcode=UTFTP_EC_PROTO; 
					goto do_error_nak;
				}
				if (rhdr->th_block!=0) { 
					errortext="got OACK opcode at the wrong time";
					ES(syslog(LOG_ERR,errortext););
					retcode=UTFTP_EC_PROTO;
					goto do_error_nak;
				}
				/* we need to process the options here. Server may use a lower blksize ... */
				p=flags->recvbuf.buf+TFTP_OFFSET;
				while (p!=flags->recvbuf.buf+got) {
					char *opt;
					char *val;
					opt=p;
					/* proceed to end of option name */
					while (p!=flags->recvbuf.buf+got && *p!=0) p++;
					if (p==flags->recvbuf.buf+got) { 
						errortext="unterminated option name in OACK packet"; 
						syslog(LOG_ERR,errortext); 
						retcode=UTFTP_EC_PROTO; 
						goto do_error_nak;
					}
					val=p+1;
					p=val;
					/* proceed to end of option name */
					while (p!=flags->recvbuf.buf+got && *p!=0) p++;
					if (p==flags->recvbuf.buf+got) { 
						errortext="unterminated option value in OACK packet"; 
						syslog(LOG_ERR,errortext); 
						retcode=UTFTP_EC_PROTO; 
						goto do_error_nak;
					}
					p++;
					if (0==strcasecmp(opt,"blksize")) {
						unsigned long x;
						if (-1==str2ulong(val,&x,0)) {
							errortext="received blksize is not a valid unsigned long";
							syslog(LOG_ERR,"received blksize %s is not a valid unsigned long",val);
							retcode=UTFTP_EC_PROTO; goto do_error_nak;
						}
						if (x<8) { 
							errortext="received blksize is too low";
							syslog(LOG_ERR,"received blksize %s is too low",val); 
							retcode=UTFTP_EC_PROTO; goto do_error_nak; 
						}
						if (x>flags->segsize) { 
							errortext="received blksize is too high";
							syslog(LOG_ERR,"received blksize %s is higher then we wanted",val); 
							retcode=UTFTP_EC_PROTO; goto do_error_nak;
						}
						flags->segsize=x;
					}
				}
				blockno=0;
				got=TFTP_OFFSET;
				may_get_oack=0;
				break;
			}

			if (rhdr->th_opcode!=ACK)  {
				errortext="unknown TFTP opcode";
				ES(syslog(LOG_ERR, "got unknown opcode %d",rhdr->th_opcode););
				retcode=UTFTP_EC_PROTO; goto do_error_nak;
			}
				
			if (rhdr->th_block == blockno) { /* ACK for our block */
				if (flags->force_timeout) {
					int o=alarm(0);
					sleep(flags->force_timeout);
					alarm(o);
				}
				if (flags->force_stop) {
					close(fd);
					return 0;
				}
				break;
			}

			/* ACK for something else. Throw away everything the kernel may have buffered */
			while (1) {
				struct timeval tv;
				fd_set set;
				FD_SET(remotefd,&set);
				tv.tv_sec=0;
				tv.tv_usec=0;
				if (select(remotefd+1,&set,0,0,&tv)<1)
					break;
				(void) recv(remotefd, flags->recvbuf.buf, flags->segsize +TFTP_OFFSET, 0);
			}
			/* we will now simply resend the block */
		} /* while !ACK for our block */
		if (len-TFTP_OFFSET!=(ssize_t) flags->segsize && blockno)
			break;
		blockno++;
	}
	return 0;
  do_error_nak:
  	ES(
		tftp_nak(remotefd,EUNDEF,errortext,flags);
	);
  err2: close(fd); alarm(0); sigaction(SIGALRM,&sa_old,0);
  err1: close(remotefd);
  err0:
	return retcode;
}
