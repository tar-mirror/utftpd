/*
 * Copyright (C) 1998,1999 Uwe Ohse
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
 * 
 * As a special exception this source may be used as part of the
 * SRS project by CORE/Computer Service Langenbach
 * regardless of the copyright they choose.
 * 
 * Contact: uwe@ohse.de
 */
#ifndef UOIO_H
#define UOIO_H

/* AUTOCONF: AC_CHECK_TYPE(ssize_t,int) */
#include "config.h"
#include "uostr.h"
#include <sys/uio.h>

typedef struct
{
	int fd;
	union {
		/* this is a union to get rid of warnings ... */
		ssize_t (*r) P__((int fd,void *buf,size_t size));
		ssize_t (*w) P__((int fd,const void *buf,size_t size));
	} op;
	ssize_t (*opv) P__((int fd, const struct iovec * vector, int count));
	char *buf;
	size_t buflen;
	size_t bufsize;
	char *rstart;
	int eof;
	int oerr;
	unsigned int timeout; /* in seconds, user-settable */
} uoio_t;

void uoio_assign_r P__((uoio_t *,int fd, 
	ssize_t (*op)(int,void *,size_t), 
	ssize_t (*opv)(int fd, const struct iovec * vector, int count)));
void uoio_assign_w P__((uoio_t *,int fd, 
	ssize_t (*op)(int,const void *,size_t), 
	ssize_t (*opv)(int fd, const struct iovec * vector, int count)));
void uoio_destroy P__((uoio_t *)); /* will not flush output */
ssize_t uoio_getdelim_zc  P__((uoio_t * u, char **s, int delim));
ssize_t uoio_getdelim_uostr  P__((uoio_t * u, uostr_t *s, int delim));
ssize_t uoio_getchar P__((uoio_t * u, char *s));
ssize_t uoio_getmem P__((uoio_t * u, void *vs, size_t len));


/* how many bytes are still in the buffer? */
#define UOIO_PENDING(u) ((u)->buflen-((u)->rstart-(u)->buf))

	/* flush output */
ssize_t uoio_flush P__((uoio_t *u));
/* return -1 on error or len */
ssize_t uoio_write_mem P__((uoio_t *u, const void *buf,size_t len));
ssize_t uoio_write_cstr P__((uoio_t *u,const char *s));
ssize_t uoio_write_cstrmulti P__((uoio_t *u,...));
ssize_t uoio_write_char P__((uoio_t *u,char c)); /* of no real use ... */
ssize_t uoio_write_uostr P__((uoio_t *u,uostr_t *s));
ssize_t uoio_write_v P__((uoio_t *u,struct iovec *iov, int count));

#endif
