/* utftpd_send.c: server side sending code */

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
#include <sys/stat.h>
#include <netinet/in.h>
#include "no_tftp.h"

#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "timselsysdep.h"
#include "str2num.h"
#include "uostr.h"
#include "uoio.h"
#include "utftpd.h"

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


int
utftpd_send(struct utftpd_ctrl *flags)
{
	uoio_t io;
	int blockno;
	int lastchar=0; /* in case of netascii */
	const char *errortext;
	struct sigaction sa,sa_old;
	int got_timeouts=0;
	struct stat st;
	int sorcerers_problem;

	if (0!=fstat(flags->filefd,&st)) {
		int e=errno;
		const char *cs=strerror(errno);
		syslog(LOG_ERR,"fstat(%s): %s",flags->filename, cs);
		if (e==ENOENT) utftpd_nak(flags->remotefd,ENOTFOUND,cs,flags);
		if (e==EACCES) utftpd_nak(flags->remotefd,EACCESS,cs,flags);
		else utftpd_nak(flags->remotefd,EUNDEF,cs,flags);
		errno=e;
		return 1;
	} else if (!(S_ISREG(st.st_mode))) {
		/* if sending a version controlled file we will read from a pipe */
		if (flags->vc==&utftpd_novc) {
			syslog(LOG_ERR,"%s: is not a regular file",flags->filename);
			utftpd_nak(flags->remotefd,EUNDEF,"not a regular file",flags);
			return 1;
		}
	}
	/* if we are root be even more restrictive */
	if (getuid()==0) {
		if (!(st.st_mode & S_IROTH)) {
			int e=errno;
			syslog(LOG_ERR,"%s: not readable for everybody",flags->filename);
			utftpd_nak(flags->remotefd,EACCESS,"access denied",flags);
			errno=e;
			return 1;
		}
	}
	sa.sa_handler = sig_handler;
	sa.sa_flags = 0;
	sigemptyset (&sa.sa_mask);
	sigaction (SIGALRM, &sa, &sa_old);

	uoio_assign_r(&io,flags->filefd,read,0);

	blockno=1;
	while (1) {
		ssize_t len;
		int got_eof=0;
		int tryno;
		/* get data, if needed */
		if (flags->first_packet_length) {
			/* we have to send an OACK before ... */
			blockno=0; /* and expect an ACK for packet 0 */
			len=flags->first_packet_length;
			flags->first_packet_length=0;
		} else {
			if (flags->netascii) {
				char c;
				/* LF -> CR LF, CR -> CR nul .... what a stupid thing :-) */
				for (len=0;len<(ssize_t) flags->segsize;len++) {
					ssize_t l;
					if (lastchar=='\r') { c=0; lastchar = 0; }
					else if (lastchar =='\n') { c='\n'; lastchar = 0; }
					else {
						l=uoio_getchar(&io,&c);
						if (l==-1) {
							errortext=strerror(errno);
							ES( syslog(LOG_ERR,"read(%s): %s",flags->filename,errortext););
							goto do_error_nak;
						}
						if (l==0) {got_eof=1; break;}
						lastchar=c;
						if (c=='\n') c='\r';
					}
					flags->sendbuf.buf[TFTP_OFFSET+len]=c;
				}
			} else {
				len=uoio_getmem(&io,flags->sendbuf.buf+TFTP_OFFSET,flags->segsize);
				if (len==-1) {
					errortext=strerror(errno);
					ES(syslog(LOG_ERR,"read(%s): %s",flags->filename,errortext););
					goto do_error_nak;
				}
				if (len==0) got_eof=1;
			}
			flags->sendbuf.hdr->th_opcode = htons((u_short)DATA);
			flags->sendbuf.hdr->th_block = htons((u_short)blockno);
			flags->bytes+=len;
		}
		if (got_eof) {
			if (flags->pid) {
				int fail;
				fail=wait_check_and_log(flags->pid);
				if (fail) {
					errortext="child terminated abnormally";
					goto do_error_nak;
				}
				flags->pid=0; /* do not wait again */
			}
		}
		tryno=5;
		sorcerers_problem=0;
		while (1) {
			struct tftphdr *rhdr;
			ssize_t got;
			/* send to the peer */
			if (!sorcerers_problem) {
				if (send(flags->remotefd, flags->sendbuf.buf, len+TFTP_OFFSET, 0) != len+TFTP_OFFSET) {
					errortext=strerror(errno);
					ES(syslog(LOG_ERR,"send(): %s",errortext););
					goto do_error;
				}
			}
			sorcerers_problem=0;

			alarm(flags->timeout * (got_timeouts+1));
			got = recv(flags->remotefd, flags->recvbuf.buf, flags->segsize+TFTP_OFFSET, 0);
			if (got < 0) {
				if (errno==EINTR && got_timeouts++<5) continue;
				ES(syslog(LOG_ERR, "recv: %s",strerror(errno)););
				goto do_error;
			}
			got_timeouts=0;

			rhdr=(struct tftphdr *)flags->recvbuf.hdr;
			rhdr->th_opcode = ntohs((u_short)rhdr->th_opcode);
			rhdr->th_block = ntohs((u_short)rhdr->th_block);
			if (rhdr->th_opcode == ERROR) {
				ES(syslog(LOG_ERR, "got ERROR"););
				goto do_error;
			}
			if (rhdr->th_opcode!=ACK)  {
				errortext="unknown TFTP opcode";
				ES(syslog(LOG_ERR, "got unknown opcode %d",rhdr->th_opcode););
				goto do_error_nak;
			}
			sorcerers_problem=1;
				
			if (rhdr->th_block == blockno)  /* ACK for our block */
				break;
			if (rhdr->th_block == blockno-1 && blockno)
				sorcerers_problem=1;

			/* ACK for something else. Throw away everything the kernel may have buffered */
			while (1) {
				struct timeval tv;
				fd_set set;
				FD_ZERO(&set);
				FD_SET(flags->remotefd,&set);
				tv.tv_sec=0;
				tv.tv_usec=0;
				if (select(flags->remotefd+1,&set,0,0,&tv)<1)
					break;
				if (TFTP_OFFSET == recv(flags->remotefd, flags->recvbuf.buf, 
					flags->segsize +TFTP_OFFSET, 0)) {
					rhdr=(struct tftphdr *)flags->recvbuf.hdr;
					rhdr->th_opcode = ntohs((u_short)rhdr->th_opcode);
					rhdr->th_block = ntohs((u_short)rhdr->th_block);
					if (rhdr->th_opcode==ACK && rhdr->th_block==blockno)
						break;
				}
			}
			if (rhdr->th_block == blockno)  /* ACK for our block */
				break;
			/* we will now simply resend the block */
		} /* while !ACK for our block */
		/* the first packet which is not "full" is the last packet.
		 * Except, of course, if the get an OACK, which is block number 0
		 */
		if (len!=(ssize_t) flags->segsize && blockno!=0)
			break;
		blockno++;
	}
	uoio_destroy(&io);
	if (flags->pid) {
		int fail;
		fail=wait_check_and_log(flags->pid);
		if (fail) {
			errortext="child terminated abnormally";
			goto do_error_nak;
		}
	}
	alarm(0);
	sigaction(SIGALRM,&sa_old,0);
	syslog(LOG_INFO,"did send %s",flags->filename);
	return 0;
  do_error_nak:
  	ES(
		uoio_destroy(&io);
		utftpd_nak(flags->remotefd,EUNDEF,errortext,flags);
	);
	alarm(0);
	sigaction(SIGALRM,&sa_old,0);
	return -1;
  do_error:
  	ES( uoio_destroy(&io););
	alarm(0);
	sigaction(SIGALRM,&sa_old,0);
	return -1;
}

