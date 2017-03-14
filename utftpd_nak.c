
/* utftpd_nak.c: send a NAK */

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
/* #include <arpa/inet.h> */

#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "timselsysdep.h"
#include "utftpd.h"

int
utftpd_nak(int peer, int ec, const char *et, struct utftpd_ctrl *flags)
{
	struct tftphdr *ehdr;
	int length;

	ehdr = flags->sendbuf.hdr;
	ehdr->th_opcode = htons((u_short)ERROR);
	ehdr->th_code = htons((u_short)ec);
	length=TFTP_OFFSET;
	if (et) {
		size_t l=strlen(et);
		memcpy(flags->sendbuf.buf+length,et,l+1);
		length+=l+1;
	} else
		flags->sendbuf.buf[length++]=0;
	if (send(peer, flags->sendbuf.buf, length, 0) != length) {
		int e=errno;
		syslog(LOG_ERR, "send() for NAK: %s",strerror(errno));
		errno=e;
		return 1;
	}
	return 0;
}

