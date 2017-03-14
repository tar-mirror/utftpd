/* utftpd_recv.c: the server side receiving routine */

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

#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "timselsysdep.h"
#include "str2num.h"
#include "uostr.h"
#include "uoio.h"
#include "utftpd.h"

#define ERR_LOG1(x) do { const char *er=strerror(errno); syslog(LOG_ERR,x,er); } while(0)
#define ERR_LOG2(x,y) do { const char *er=strerror(errno); syslog(LOG_ERR,x,y,er); } while(0)

static int got_sig;
static void
sig_handler(int signo)
{
	got_sig=signo;
}

/* 
 * well, this is everything but beautiful. To be able to resend a lost packet we
 * have to send the first ACK/OACK here. But since that contains data we should not
 * now about ... 
 * i absolutely don't care about any cleanup actions here: we exit after we received
 * that file.
 */
int 
utftpd_recv(struct utftpd_ctrl *flags)
{
	uoio_t io;
	int blockno;
	int lastchar=-1; /* in case of netascii */
	int netascii=flags->netascii;
	ssize_t got;
	int got_timeouts=0;
	struct sigaction sa;
	struct sigaction old_sa;
	const char *errortext;
	size_t sendlength;
	short got_blockno;
	short got_opcode;
	int is_final=0;

	sa.sa_handler = sig_handler;
	sa.sa_flags = 0;
	sigemptyset (&sa.sa_mask);
	sigaction (SIGALRM, &sa, &old_sa);

	uoio_assign_w(&io,flags->filefd,write,0);

	blockno=1;
	got=flags->segsize+TFTP_OFFSET;
	sendlength=flags->first_packet_length+TFTP_OFFSET;

	while (1) {
		/* send the ACK / OACK */
  		while (1) {
			if (send(flags->remotefd, flags->sendbuf.buf, sendlength, 0) != (ssize_t) sendlength) 
				{ ERR_LOG1("send: %s"); goto do_error; }

			/* read data */
			alarm( (flags->timeout ? flags->timeout : 5)
				* (got_timeouts+1));
			got = recv(flags->remotefd, flags->recvbuf.buf, flags->segsize+TFTP_OFFSET, 0);
			if (got < 0) { 
				if (errno==EINTR && got_sig==SIGALRM && got_timeouts++<5) continue; /* resend my ACK */
				ERR_LOG1("recv: %s");
				goto do_error;
			}
			got_timeouts=0; /* we got a packet */
			got_opcode = ntohs((u_short)flags->recvbuf.hdr->th_opcode);
			got_blockno = ntohs((u_short)flags->recvbuf.hdr->th_block);
			if (got_opcode == ERROR) { 
				if (got>5) {
					if (flags->recvbuf.buf[got-1]!=0) {
						syslog(LOG_ERR,"got ERROR opcode with unterminated error text, last character removed:");
						flags->recvbuf.buf[got-1]=0;
					}
					syslog(LOG_ERR,"%s",flags->recvbuf.buf+TFTP_OFFSET);
					goto do_error; 
				} else {
					syslog(LOG_ERR,"got ERROR opcode, no error text"); 
					goto do_error;
				}
			}
			if (got_opcode != DATA) { 
				syslog(LOG_ERR, "got unknown opcode %d",flags->recvbuf.hdr->th_opcode); 
				errortext="got unknown TFTP opcode";
				goto do_error_nak;
			}
			if (got_blockno == blockno) break;
			/* out of sync. Throw away everything the kernel may have buffered */
			while (1) {
				struct timeval tv;
				fd_set set;
				FD_SET(flags->remotefd,&set);
				tv.tv_sec=0;
				tv.tv_usec=0;
				if (select(flags->remotefd+1,&set,0,0,&tv)<1)
					break;
				alarm(flags->timeout ? flags->timeout : 5);
				(void) recv(flags->remotefd, flags->recvbuf.buf, flags->segsize +TFTP_OFFSET, 0);
			}
			/* go back to resend ACK */
		}
		/* ok, got something */
		if (netascii) {
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
			uoio_write_mem(&io,flags->recvbuf.buf+TFTP_OFFSET,q-(flags->recvbuf.buf+TFTP_OFFSET));
			flags->bytes+=q-(flags->recvbuf.buf+TFTP_OFFSET);
		} else {
			uoio_write_mem(&io,flags->recvbuf.buf+TFTP_OFFSET,got-TFTP_OFFSET);
			flags->bytes+=got-TFTP_OFFSET;
		}
		if (got!=(ssize_t) flags->segsize+TFTP_OFFSET) {
			uoio_flush(&io);
			uoio_destroy(&io);
#if 0
#ifdef HAVE_FSYNC
			if (-1==fsync(flags->filefd)) { errortext=strerror(errno); ERR_LOG2("fsync(%s): %s", flags->filename); goto do_error_nak; }
#else
			sync();
#endif
			if (-1==close(flags->filefd)) { errortext=strerror(errno); ERR_LOG2("close(%s): %s", flags->filename); goto do_error_nak; }
#endif
			is_final=1;
		}
		flags->sendbuf.hdr->th_opcode = htons((u_short)ACK);
		flags->sendbuf.hdr->th_block = htons((u_short)blockno++);
		sendlength=TFTP_OFFSET;
		if (is_final) break; /* final ACK is sent from above, after version control */
	} /* while 1 */
	alarm(0);
	sigaction (SIGALRM, &old_sa,0 );
	return 0;
  do_error_nak:
	alarm(0);
	sigaction (SIGALRM, &old_sa,0 );
	utftpd_nak(flags->remotefd,EUNDEF,errortext,flags);
	return -1;
  do_error:
	alarm(0);
	sigaction (SIGALRM, &old_sa,0 );
	return -1;
}
