/* tftp_setup_ctrl.c: setup ctrl structure */

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
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "tftplib.h"

struct tftplib_ctrl *
tftp_setup_ctrl(struct tftplib_ctrl *old, size_t segsize)
{
	if (!old) {
		old=(struct tftplib_ctrl *)malloc(sizeof(struct tftplib_ctrl));
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

