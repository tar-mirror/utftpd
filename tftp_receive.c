/* tftp_receive.c: client side receiving routine */

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

#include <sys/socket.h>
#include <netinet/in.h>
#include "no_tftp.h"
#include <netdb.h>

#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "timselsysdep.h"
#include "str_ulong.h"
#include "str2num.h"
#include "tftplib.h"

#define ERR_LOG1(x) do { const char *er=strerror(safe_errno); syslog(LOG_ERR,x,er); } while(0)
#define ERR_LOG2(x,y) do { const char *er=strerror(safe_errno); syslog(LOG_ERR,x,y,er); } while(0)
#define ES(x) do {int safe_errno=errno; { x } errno=safe_errno; } while(0)

static int got_sig;
static void
sig_handler(int signo)
{
	got_sig=signo;
}

static int
my_write(int fd, char *buf,size_t size)
{
	while (size) {
		ssize_t got;
		got=write(fd,buf,size);
		if (-1==got) {
			if (errno!=EINTR) return -1;
		} else {
			buf+=got;
			size-=got;
		}
	}
	return 0;
}

/*
 * 
 */
int 
tftp_receive(const char *remotehost, int port, const char *remotefilename, const char *localfilename, struct tftplib_ctrl *flags)
{
    struct sockaddr_in s_in;
    int remotefd;
	int blockno;
	int lastchar=-1; /* in case of netascii */
	ssize_t got;
	int fd;
	int got_timeout=0;
	struct sigaction sa;
	struct sigaction old_sa;
	const char *errortext;
	int is_connected=0;
	int may_get_oack=0;
	size_t sendlength;
	int is_final=0;
	short got_opcode=-1; /* get rid of warning */
	short got_blockno;
	int retcode=UTFTP_EC_UNDEF; /* undefined errror */


    memset (&s_in, 0, sizeof (s_in));
    s_in.sin_family = AF_INET;
    remotefd = socket (AF_INET, SOCK_DGRAM, 0);
    if (remotefd < 0) { ES(syslog(LOG_ERR,"socket: %s",strerror(errno));); retcode=UTFTP_EC_LOCAL; goto err0; }
    if (bind (remotefd, (struct sockaddr *) &s_in, sizeof (s_in)) < 0)
        { ES(syslog(LOG_ERR,"bind: %s",strerror(errno));); retcode=UTFTP_EC_LOCAL; goto err1; }

	fd=open(localfilename,O_WRONLY|O_CREAT|O_TRUNC
#if !defined(HAVE_FSYNC) && defined(O_SYNC)
		|O_SYNC
#endif
		,0666);
	if (fd==-1) {
		ES(
			const char *cs=strerror(safe_errno);
			syslog(LOG_ERR,"open(%s): %s",localfilename, cs);
		);
		retcode=UTFTP_EC_LOCAL;
		goto err1;
	}

	sa.sa_handler = sig_handler;
	sa.sa_flags = 0;
	sigemptyset (&sa.sa_mask);
	sigaction (SIGALRM, &sa, &old_sa);
	blockno=1; /* we expect this */
	got=flags->segsize+TFTP_OFFSET;

	sendlength=tftp_prepare_rq(RRQ, remotehost, port, remotefilename, &s_in, &may_get_oack, flags);
	if (sendlength==0) goto err2; /* undefined error */

	while (1) {
		/* flags->sendbuf contains to data to be sent.
		 * sendlength is the number of bytes to send.
		 */
  		while (1) {
			if (is_connected) {
				if (send (remotefd, flags->sendbuf.buf, sendlength, 0) 
					!= (ssize_t) sendlength) {
					ES (ERR_LOG1 ("send: %s"););
					retcode = UTFTP_EC_NETWORK;
					goto err3;
				}
				if (flags->duplicate_ack) {
					if (send (remotefd, flags->sendbuf.buf, sendlength, 0) 
						!= (ssize_t) sendlength) {
						ES (ERR_LOG1 ("send: %s"););
						retcode = UTFTP_EC_NETWORK;
						goto err3;
					}
				}
			} else {
				if (sendto(remotefd, flags->sendbuf.buf, sendlength, 0,(struct sockaddr *)&s_in,sizeof(s_in)) 
					!= (ssize_t) sendlength) 
						{ ES(ERR_LOG1("send: %s");); retcode=UTFTP_EC_NETWORK; goto err3; }
			}
			if (is_final) break;

			/* read data */
			alarm(flags->timeout ? flags->timeout : 5);
			if (is_connected) {
				got = recv(remotefd, flags->recvbuf.buf, flags->segsize+TFTP_OFFSET, 0);
			} else {
				socklen_t s_len=sizeof(s_in);
				got = recvfrom(remotefd, flags->recvbuf.buf, flags->segsize+TFTP_OFFSET, 0, (struct sockaddr *)&s_in,&s_len);
				if (got>=0) {
					if (0!= connect(remotefd,(struct sockaddr *)&s_in,sizeof(s_in)))
							{ ES(syslog(LOG_ERR,"connect: %s",strerror(errno));); retcode=UTFTP_EC_NETWORK; goto err3; }
					is_connected=1;
				}
			}
			if (got < 0) { 
				if (errno==EINTR && got_sig==SIGALRM) {
					if (++got_timeout<flags->retries) continue; /* resend my ACK */
				}
				ES(ERR_LOG1("recv: %s");); retcode=UTFTP_EC_TIMEOUT; goto err3;
			}
			got_timeout=0; /* we got a packet */
			got_opcode = ntohs((u_short)flags->recvbuf.hdr->th_opcode);
			got_blockno = ntohs((u_short)flags->recvbuf.hdr->th_block);
			if (got_opcode == ERROR) { 
				if (got>5) {
					if (flags->recvbuf.buf[got-1]!=0) {
						ES(syslog(LOG_ERR,"got ERROR opcode with unterminated error text, last character removed:"););
						flags->recvbuf.buf[got-1]=0;
					}
					ES(syslog(LOG_ERR,"%s",flags->recvbuf.buf+TFTP_OFFSET););
					retcode=UTFTP_EC_ERROR;
					goto err3; 
				} else {
					ES(syslog(LOG_ERR,"got ERROR opcode, no error text");); retcode=UTFTP_EC_ERROR; goto err3; 
				}
			}
			if (got_opcode == OACK) {
				char *p;
				if (!may_get_oack) { ES(syslog(LOG_ERR,"got unwanted OACK opcode");); retcode=UTFTP_EC_PROTO; goto err3; }
				if (blockno!=1) { ES(syslog(LOG_ERR,"got OACK opcode at the wrong time");); retcode=UTFTP_EC_PROTO; goto err3; }
				/* we need to process the options here. Server may use a lower blksize ... */
				p=flags->recvbuf.buf+TFTP_OFFSET;
				while (p!=flags->recvbuf.buf+got) {
					char *opt;
					char *val;
					opt=p;
					/* proceed to end of option name */
					while (p!=flags->recvbuf.buf+got && *p!=0) p++;
					if (p==flags->recvbuf.buf+got) {
						syslog(LOG_ERR,"unterminated option name in OACK packet");
						_exit(1);
					}
					val=p+1;
					p=val;
					/* proceed to end of option name */
					while (p!=flags->recvbuf.buf+got && *p!=0) p++;
					if (p==flags->recvbuf.buf+got) {
						syslog(LOG_ERR,"unterminated option value in OACK packet");
						retcode=UTFTP_EC_PROTO; 
						goto err3;
					}
					p++;
					if (0==strcasecmp(opt,"blksize")) {
						unsigned long x;
						if (-1==str2ulong(val,&x,0)) {
							syslog(LOG_ERR,"received blksize %s is not a valid unsigned long",val);
							retcode=UTFTP_EC_PROTO; 
							goto err3;
						}
						if (x<8) { syslog(LOG_ERR,"received blksize %s is too low",val); retcode=UTFTP_EC_PROTO; goto err3; }
						if (x>flags->segsize) { 
							syslog(LOG_ERR,"received blksize %s is higher then we wanted",val); retcode=UTFTP_EC_PROTO; goto err3;
						}
						flags->segsize=x;
					}
				}

				blockno=0;
				got=TFTP_OFFSET;
				may_get_oack=0;
				break;
			}
			if (got_opcode != DATA)  { ES(syslog(LOG_ERR, "got unknown opcode %d",flags->recvbuf.hdr->th_opcode);); retcode=UTFTP_EC_PROTO; goto err3; }
			if (got_blockno == blockno) {
				if (may_get_oack) {
					/* we should have gotten an OACK, but didn't get one.
					 * go back to defaults.
					 */
					flags=tftp_setup_ctrl(flags, 512);
					if (!flags) { ES(syslog(LOG_ERR,"out of memory");); retcode=1; retcode=UTFTP_EC_LOCAL; goto err3; }
				}
				break;
			}
			/* out of sync. Throw away everything the kernel may have buffered */
			while (1) {
				struct timeval tv;
				fd_set set;
				FD_SET(remotefd,&set);
				tv.tv_sec=0;
				tv.tv_usec=0;
				if (select(remotefd+1,&set,0,0,&tv)<1)
					break;
				alarm(flags->timeout ? flags->timeout : 5);
				(void) recv(remotefd, flags->recvbuf.buf, flags->segsize +TFTP_OFFSET, 0);
			}
			/* go back to resend packet */
		}
		if (is_final)
			break;
		/* ok, we got something. Safe it: */
		if (flags->netascii) {
			char *p,*q,*end;
			p=q=flags->recvbuf.buf+TFTP_OFFSET;
			end=flags->recvbuf.buf+got;
			while (p!=end) {
				if (lastchar=='\r') {
					if (*p==0) {
						*q++='\r';
						p++;
					} else
						*q++=*p++;
					lastchar=-1;
				} else {
					lastchar=*p;
					if (*p=='\r')
						p++;
					else
						*q++=*p++;
				}
			}
			if (-1==my_write(fd,flags->recvbuf.buf+TFTP_OFFSET,q-(flags->recvbuf.buf+TFTP_OFFSET))) {
				ES(ERR_LOG2("write(%s): %s", localfilename); errortext=strerror(safe_errno);); retcode=UTFTP_EC_LOCAL ; goto do_error_nak;
			}
		} else {
			if (-1==my_write(fd,flags->recvbuf.buf+TFTP_OFFSET,got-TFTP_OFFSET)) {
				ES(ERR_LOG2("write(%s): %s", localfilename); errortext=strerror(safe_errno);); retcode=UTFTP_EC_LOCAL ; goto do_error_nak;
			}
		}
		/* send an ACK */
		flags->sendbuf.hdr->th_opcode = htons((u_short)ACK);
		flags->sendbuf.hdr->th_block = htons((u_short)blockno++);
		if (flags->force_timeout) {
			int o=alarm(0);
			sleep(flags->force_timeout);
			alarm(o);
		}
		sendlength=TFTP_OFFSET;
		if (got_opcode==DATA && got!=(ssize_t) flags->segsize+TFTP_OFFSET) {
			/* send final ACK */
#ifdef HAVE_FSYNC
			if (-1==fsync(fd)) { ES(ERR_LOG2("fsync(%s): %s", localfilename); errortext=strerror(safe_errno);); 
				retcode=UTFTP_EC_LOCAL; goto do_error_nak; }
#else 
# ifndef(O_SYNC)
			sync();
# endif
#endif
			if (-1==close(fd)) { ES(ERR_LOG2("close(%s): %s", localfilename); errortext=strerror(safe_errno);); 
				retcode=UTFTP_EC_LOCAL; goto do_error_nak; }
			is_final=1;
		}
	}
	alarm(0);
	sigaction (SIGALRM, &old_sa,0 );
	close(remotefd);
	return UTFTP_EC_OK;
do_error_nak:
	tftp_nak(remotefd,EUNDEF,errortext,flags);
err3:
	ES(
		alarm(0);
		sigaction (SIGALRM, &old_sa,0 );
	);
err2:
	close(fd);
err1:
	close(remotefd);
err0:
	return retcode;
}
