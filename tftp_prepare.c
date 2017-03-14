/* tftp_prepare.c: common "first packet" thing for client routines */

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
#include <netdb.h>
#include "no_tftp.h"
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "timselsysdep.h"
#include "str_ulong.h"
#include "str2num.h"
#include "tftplib.h"

/*
 * 
 */
size_t 
tftp_prepare_rq(int type, const char *server, unsigned short port, const char *remotefilename, struct sockaddr_in *s_in,
	int *may_get_oack, struct tftplib_ctrl *flags)
{
    struct hostent *hp;
    size_t l;
	size_t sendlength;
	struct in_addr ina;

	if (0==inet_aton(server,&ina)) {
	    hp = gethostbyname (server);
	    if (hp == NULL) { int e=errno;syslog(LOG_ERR,"can't resolve hostname");errno=e; return 0; }
	    memcpy (&s_in->sin_addr, hp->h_addr, hp->h_length);
	} else {
	    memcpy (&s_in->sin_addr, &ina, sizeof(ina));
	}
    s_in->sin_port = port;

    /* so far, so good ... now do that protocol startup */

    /*
     *  2 bytes     string    1 byte     string   1 byte
     *  ------------------------------------------------
     *  | Opcode |  Filename  |   0  |    Mode    |   0  |
     *  ------------------------------------------------
    */
    flags->sendbuf.hdr->th_opcode=htons(type);
	sendlength=2;

    l=strlen(remotefilename)+1; /* include \0 */
	if (sendlength+l>512) goto toolong;
    memcpy(flags->sendbuf.buf+2,remotefilename,l);
    sendlength+=l;
    if (flags->netascii)  {
		if (sendlength+9>512) goto toolong;
        memcpy(flags->sendbuf.buf+sendlength,"netascii",9); /* \0  */
        sendlength+=9;
    } else {
		if (sendlength+6>512) goto toolong;
        memcpy(flags->sendbuf.buf+sendlength,"octet",6); /* \0  */
        sendlength+=6;
    }
    if (flags->segsize!=512) {
        char buf[STR_ULONG];
		if (sendlength+8>512) goto toolong;
        memcpy(flags->sendbuf.buf+sendlength,"blksize",8); /* \0  */
        sendlength+=8;

        l=str_ulong(buf,flags->segsize)+1; /* \0 */
		if (sendlength+l>512) goto toolong;
        memcpy(flags->sendbuf.buf+sendlength,buf,l);
        sendlength+=l;
		*may_get_oack=1;
    }
    if (flags->timeout!=5) {
        char buf[STR_ULONG];
		if (sendlength+8>512) goto toolong;
        memcpy(flags->sendbuf.buf+sendlength,"timeout",8); /* \0  */
        sendlength+=8;
        l=str_ulong(buf,flags->timeout)+1;
		if (sendlength+l>512) goto toolong;
        memcpy(flags->sendbuf.buf+sendlength,buf,l);
        sendlength+=l;
		*may_get_oack=1;
    }
    if (flags->revision) {
		if (sendlength+9>512) goto toolong;
        memcpy(flags->sendbuf.buf+sendlength,"revision",9); /* \0  */
        sendlength+=9;
        l=strlen(flags->revision)+1;
		if (sendlength+l>512) goto toolong;
        memcpy(flags->sendbuf.buf+sendlength,flags->revision,l);
        sendlength+=l;
		*may_get_oack=1;
    }
	if (flags->send_garbage) {
		if (sendlength+2>512) goto toolong;
		memcpy(flags->sendbuf.buf+sendlength,"AZ",2);
		sendlength+=2;
	}
	return sendlength;
  toolong:
    { int e=errno;syslog(LOG_ERR,"?RQ packet would be too long");errno=e; return 0; }
    return 0;
}
