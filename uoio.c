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
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "timselsysdep.h"
#include "uostr.h"
#include "uoio.h"

#define READCONST 8192
#define SAMEBLOCK(move,start) (((size_t) ((move)-(start)))<READCONST)

#define ULEFT(u) (u->buflen-(u->rstart-u->buf))

void 
uoio_destroy (uoio_t * u)
{
	if (u->buf)
		free (u->buf);
}

static int
handle_timeout(uoio_t *u, int w)
{
	fd_set set;
	time_t start=0;
	int x;
	struct timeval tv;
	while (1) {
		tv.tv_usec=0;
		if (!start) { 
			start=time(0); 
			tv.tv_sec=u->timeout; 
		} else  
			tv.tv_sec=u->timeout-(time(0)-start);
		FD_ZERO(&set); 
		FD_SET(u->fd,&set);
		x=select(u->fd + 1,w ? 0 : &set, w ? &set : 0,0,&tv);
		if (x==-1 && errno==EINTR) continue;
		if (x==-1) return -1;
		if (x==0) { errno=ETIMEDOUT; return -1; }
		return 0;
	}
}

#define DO_DISQUALIFY_CONST(from,to) \
	union DISQUALIFY {const char *ccs; char *cs;} disqua; \
	disqua.ccs=from;to=disqua.cs;

static int
uoio_do_write(uoio_t *u,const char *robuf,size_t len)
{
	char *buf;

	DO_DISQUALIFY_CONST(robuf,buf);

	if (u->oerr) {
		errno=u->oerr;
		return -1;
	}
	while(len) {	
		ssize_t r;
		if (u->timeout) {
			if (handle_timeout(u,1))
				return -1;
		}
		r=u->op.w(u->fd,buf,len);
		if (r==-1) {
			if (errno==EINTR)
				continue;
			u->oerr=errno;
			return -1; /* maybe data loss */
		}
		buf+=r;
		len-=r;
	}
	return 0;
}

static inline int
uoio_do_writev(uoio_t *u,struct iovec *iov,int count)
{
	if (u->oerr) {
		errno=u->oerr;
		return -1;
	}
	do {	
		ssize_t r;
		if (u->timeout) {
			if (handle_timeout(u,1))
				return -1;
		}
		r=u->opv(u->fd,iov,count);
		if (r==-1) {
			if (errno==EINTR)
				continue;
			u->oerr=errno;
			return -1; /* maybe data loss */
		}
		while (count && r) {
			if (r>=(ssize_t) iov[0].iov_len) {
				r-=iov[0].iov_len;
				iov[0].iov_len=0;
				count--;
				iov++;
			} else {
				iov[0].iov_len-=r;
				r=0;
			}
		}
	} while (count);
	return 0;
}

ssize_t 
uoio_flush(uoio_t *u)
{
	ssize_t r;
	if (u->oerr) {
		errno=u->oerr;
		return -1;
	}
	if (!u->buf || u->buflen==0)
		return 0;
	r=uoio_do_write(u,(const char *)u->buf,u->buflen);
	if (r<0)
		return r;
	u->buflen=0;
	return 0;
}

#define WBUF 4096
ssize_t 
uoio_write_mem(uoio_t *u, const void *buf,size_t len)
{
	if (!u->buf) {
		u->buf=(char *) malloc(WBUF);
		if (u->buf) {
			u->bufsize=WBUF;
		} else {
			/* uh ... */
			ssize_t r;
			r=uoio_do_write(u,(const char *)buf,len);
			if (r<0)
				return r;
			return len;
		}
	}
	/* can we put it into our buffer? */
	if (len+u->buflen<=u->bufsize) {
		size_t done=0;
		done=u->bufsize-u->buflen;
		memcpy(u->buf+u->buflen,buf,len);
		u->buflen+=len;
		if (u->buflen==u->bufsize) {
			ssize_t r;
			r=uoio_do_write(u,u->buf,u->buflen);
			if (r)
				return r;
			u->buflen=0;
		}
		return len;
	}
	/* ok, too much */
	if (u->buflen==0) {
		/* easy case */
		ssize_t r;
		r=uoio_do_write(u,(const char *)buf,len);
		if (r<0)
			return r;
		return len;
	}
	if (u->opv) {
		/* writev the whole garbage */
		ssize_t r;
		struct iovec iov[2];
		char *wbuf;
		DO_DISQUALIFY_CONST((const char *)buf,wbuf);
		iov[0].iov_base=u->buf;
		iov[0].iov_len=u->buflen;
		iov[1].iov_base=wbuf;
		iov[1].iov_len=len;
		r=uoio_do_writev(u,iov,2);
		u->buflen=0;
		if (r<0)
			return r;
		return len;
	}
	/* if it's too much for the rest of this buffer and
	 * too much for the next buffer then do it the hard
	 * way.
	 */
	if (len>(u->bufsize-u->buflen)+u->bufsize) {
		ssize_t r;
		r=uoio_do_write(u,(const char *)u->buf,u->buflen);
		if (r<0)	
			return r;
		u->buflen=0;
		r=uoio_do_write(u,(const char *)buf,len);
		if (r<0)	
			return r;
		return len;
	}
	/* copy as much as we can into the buffer */
	{
		size_t done;
		ssize_t r;

		done=u->bufsize-u->buflen;
		if (done)
			memcpy(u->buf+u->buflen,buf,done);
		r=uoio_do_write(u,u->buf,u->bufsize);
		if (r<0)	
			return r;
		memcpy(u->buf,((const char *)buf)+done,len-done);
		u->buflen=len-done;
		return len;
	}
}

ssize_t 
uoio_write_cstr(uoio_t *u,const char *s)
{
	return uoio_write_mem(u,s,strlen(s));
}

ssize_t 
uoio_write_char(uoio_t *u,char c)
{
	/* bye bye performance */
	return uoio_write_mem(u,&c,1);
}

ssize_t 
uoio_write_uostr(uoio_t *u,uostr_t *s)
{
	return uoio_write_mem(u,s->data,s->len);
}

ssize_t
uoio_getmem (uoio_t * u, void *vs, size_t len)
{
	char *s=(char *) vs;
	size_t gotlen=0;
	while (1) {
		ssize_t r;
		if (u->rstart) {
			while (len && u->rstart) {
				*s++=*u->rstart;
				u->rstart++;
				gotlen++;
				len--;
				if (u->rstart == u->buf + u->buflen) {
					u->buflen=0;
					u->rstart = 0;
				}
			}
			if (!len) return gotlen; /* finished */
		}
		if (u->eof) return gotlen;
		if (!u->buf) {
			u->rstart = u->buf = (char *) malloc (READCONST);
			if (!u->buf) {
				errno = ENOMEM;
				return -1;
			}
			u->bufsize = READCONST;
		}
		/* read something */
		if (u->timeout) {
			if (handle_timeout(u,0))
				return -1;
		}
		while (1) {
			r = u->op.r (u->fd, u->buf + u->buflen, u->bufsize-u->buflen);
			if (r < 0 && errno == EINTR)
				continue;
			if (r < 0)
				return -1;
			break;
		}
		if (r == 0) {
			u->eof = 1;
			u->rstart=0;
		} else
			u->rstart=u->buf;
		u->buflen += r;
	}
}
ssize_t
uoio_getchar (uoio_t * u, char *s)
{
	while (1) {
		ssize_t r;
		if (u->rstart) {
			*s=*u->rstart;
			u->rstart++;
			if (u->rstart == u->buf + u->buflen) {
				u->buflen=0;
				u->rstart = 0;
			}
			return 1;
		}
		if (u->eof) return 0;
		if (!u->buf) {
			u->rstart = u->buf = (char *)malloc (READCONST);
			if (!u->buf) {
				errno = ENOMEM;
				return -1;
			}
			u->bufsize = READCONST;
		}
		/* read something */
		if (u->timeout) {
			if (handle_timeout(u,0))
				return -1;
		}
		while (1) {
			r = u->op.r (u->fd, u->buf + u->buflen, u->bufsize-u->buflen);
			if (r < 0 && errno == EINTR)
				continue;
			if (r < 0)
				return -1;
			break;
		}
		if (r == 0) {
			u->eof = 1;
			u->rstart=0;
		} else {
			u->buflen += r;
			u->rstart=u->buf;
		}
	}
}
ssize_t
uoio_getdelim_zc (uoio_t * u, char **s, int delim)
{
	while (1) {
		if (u->rstart) {
			char *p;
			char *e;
			for (p=u->rstart, e=u->buf+u->buflen; p!=e && *p!=delim; p++) /* nothing */;
			if (p!=e) {
				ssize_t ret;
				/* found delim in unread part of the buffer */
				*s = u->rstart;
				ret = p - u->rstart + 1;
				if (p >= u->buf + u->buflen -1) {
					u->rstart = 0;
					u->buflen = 0;
				} else
					u->rstart = p + 1;
/* write(1,*s,ret); */
				return ret;
			}
			/* there is something in the buffer, but not enough. 
			 * shuffle around if on the first block. 
			 */
			if (u->rstart != u->buf && !SAMEBLOCK(u->rstart,u->buf)) {
/* write(1,*s,ret); */
				size_t l = ULEFT (u);
				if (u->buf+l>=u->rstart)
					memcpy (u->buf, u->rstart, l);
				else
					memmove (u->buf, u->rstart, l);
				u->rstart=u->buf;
				u->buflen = l;
			}
			/* now get something more ... */

			/* if buffer is full get a new block */
			/* if buffer is nearly full get a new block plus something */
			if (u->bufsize - u->buflen < 1024) {
				size_t l;
				size_t o;
				l = u->bufsize + READCONST;
				o = (u->rstart - u->buf);
				p = (char *) realloc (u->buf, l);
				if (!p) {
					errno = ENOMEM;
					return -1;
				}
				u->bufsize = l;
				u->buf = p;
				u->rstart = p + o;
			}
			/* ok, enough place to read something */
		} else if (!u->buf) {
			u->rstart = u->buf = (char *) malloc (READCONST);
			if (!u->buf) {
				errno = ENOMEM;
				return -1;
			}
			u->bufsize = READCONST;
		}
		if (u->eof) {
			/* we didn't find the character. return the rest of the buffer */
			if (!u->rstart) {
				*s = NULL;
				return 0;
			} else {
				size_t l = ULEFT (u);
				*s = u->rstart;
				u->rstart = 0;
				return l;
			}
		}
		/* read something */
		{
			ssize_t r;
			if (u->timeout) {
				if (handle_timeout(u,0))
					return -1;
			}
			while (1) {
				r = u->op.r (u->fd, u->buf + u->buflen, u->bufsize-u->buflen);
				if (r < 0 && errno == EINTR)
					continue;
				if (r < 0)
					return -1;
				break;
			}
			if (r == 0)
				u->eof = 1;
			u->buflen += r;
			if (!u->rstart) u->rstart=u->buf;
		}
	}
	/* not reached */
}


void 
uoio_assign_r(uoio_t *u,int fd,
        ssize_t (*op)(int,void *,size_t),
	ssize_t (*opv)(int fd, const struct iovec * vector, int count))
{
	memset(u,0,sizeof(*u));
	u->fd = fd;
	u->op.r = op;
	u->opv = opv;
	u->buf = 0;
	u->rstart = 0;
	u->buflen = 0;
	u->bufsize = 0;
	u->eof = 0;
	u->oerr = 0;
	u->timeout = 0;
}


void 
uoio_assign_w(uoio_t *u,int fd,
        ssize_t (*op)(int,const void *,size_t),
	ssize_t (*opv)(int fd, const struct iovec * vector, int count))
{
	memset(u,0,sizeof(*u));
	u->fd = fd;
	u->op.w = op;
	u->opv = opv;
	u->buf = 0;
	u->rstart = 0;
	u->buflen = 0;
	u->bufsize = 0;
	u->eof = 0;
	u->oerr = 0;
	u->timeout = 0;
}
