/* utftpd_make: compile utftpd configuration file into cdb database */
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

/* somewhat based on cdbmake.c by djb at pobox.com, but the bugs are my own */

#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#include "cdbmake.h"
#include "uostr.h"
#include "uoio.h"
#include "utftpd_make.h"
#include "str2num.h"
#include "str_ulong.h"
#include "uogetopt.h"
#ifndef errno
extern int errno;
#endif


static char *argv0;
uoio_t io_e;

void 
die_line (int e, const char *text1, const char *text2)
{
	char buf[STR_ULONG];
	uoio_write_cstr (&io_e, argv0);
	uoio_write_cstr (&io_e, ": ");
	uoio_write_cstr (&io_e, text1);
	if (text2) {
		uoio_write_cstr (&io_e, ": ");
		uoio_write_cstr (&io_e, text2);
	}
	uoio_write_cstr (&io_e, " at line ");
	str_ulong(buf,parser_lineno);
	uoio_write_cstr (&io_e, buf);
	if (e) {
		uoio_write_cstr (&io_e, ": ");
		uoio_write_cstr (&io_e, strerror (e));
	}
	uoio_write_cstr (&io_e, "\n");
	uoio_flush (&io_e);
	_exit (1);
}
static void 
die (int e, const char *where)
{
	uoio_write_cstr (&io_e, argv0);
	uoio_write_cstr (&io_e, ": ");
	uoio_write_cstr (&io_e, where);
	if (e) {
		uoio_write_cstr (&io_e, ": ");
		uoio_write_cstr (&io_e, strerror (e));
	}
	uoio_write_cstr (&io_e, "\n");
	uoio_flush (&io_e);
	_exit (1);
}

static void 
writeerror (void)
{
	die (errno, "unable to write");
}
static void 
nomem (void)
{
	write (2, "out of memory\n", 14);
	exit (111);
}
static void 
overflow (void)
{
	die (0, "database too large");
	exit (111);
}
static void 
usage (void)
{
	die (0, "Give --help option for usage information");
	exit (1);
}
static void 
format (const char *text)
{
	die_line (0, "bad input format", text);
	exit (1);
}
static void 
mask_bad_range (const char *text)
{
	die_line (0, "bad /mask in input", text);
	exit (1);
}

static uint32 
safeadd (uint32 u, uint32 v)
{
	u += v;
	if (u < v)
		overflow ();
	return u;
}

struct cdbmake *cdbm;
uint32 pos;
unsigned char packbuf[8];

static void
add_one(uoio_t *io, const char *key, uint32 keylen, const char *data, uint32 datalen)
{
	int i;
	uint32 h;
	cdbmake_pack (packbuf, keylen);
	cdbmake_pack (packbuf + 4, datalen);
	if (-1 == uoio_write_mem (io, packbuf, 8)) writeerror ();

	h = CDBMAKE_HASHSTART;
	for (i = 0; i < (int) keylen; ++i) {
		h = cdbmake_hashadd (h, key[i]);
	}
	if (-1 == uoio_write_mem (io, key, keylen)) writeerror ();
	if (-1 == uoio_write_mem (io, data, datalen)) writeerror ();
	if (!cdbmake_add (cdbm, h, pos, malloc)) nomem ();
	pos = safeadd (pos, (uint32) 8);
	pos = safeadd (pos, (uint32) keylen);
	pos = safeadd (pos, (uint32) datalen);
}

static uogetopt_t myopts[]={    {0,0,0,0,0,0,0} };

static void
add_hosts(uoio_t *io_tmp)
{
	struct myhost *h;
	uostr_t data;
	data.data=0;
	/* go through IP addresses */
	for (h=host_anker;h;h=h->next) {
		char *q;
		struct varlist *v;
		uostr_cut(&data,0);
		for (v=h->vars;v;v=v->next) {
			uostr_xadd_cstr(&data,v->name);
			uostr_xadd_cstr(&data,"=");
			uostr_xadd_cstr(&data,v->value);
			uostr_x0(&data);
		}
		while (1) {
			char *perc;
			char *end;
			struct varlist *w;
			static uostr_t tmp=UOSTR_INIT;
			perc=data.data;
			while(*perc!='=') perc++;
			while (perc<data.data+data.len-1) {
				if (*perc=='$' && perc[1]=='{') 
					break;
				perc++;
			}
			if (perc>=data.data+data.len-1) 
				break;
			end=strchr(perc,'}');
			if (!end) { parser_lineno=v->lineno; die_line(0,"unfinished ${ in",perc); }
			uostr_xdup_mem(&tmp,perc+2,end-perc-2);
			uostr_x0(&tmp);
			for (w=h->vars;w;w=w->next) {
				if (0==strcmp(w->name,tmp.data)) {
					static uostr_t tmp2=UOSTR_INIT;
					uostr_xdup_mem(&tmp2,data.data,perc-data.data);
					uostr_xadd_cstr(&tmp2,w->value);
					uostr_xadd_mem(&tmp2,end+1,data.data+data.len-end-2);
					uostr_x0(&tmp2);
					uostr_xdup_uostr(&data,&tmp2);
					break;
				}
			}
			if (!w) {
				uostr_cut(&tmp,-1);
				uostr_xdup_mem(&tmp,"${",2);
				uostr_xadd_mem(&tmp,perc+2,end-perc-2);
				uostr_xadd_mem(&tmp,"}",2);
				parser_lineno=v->lineno;
				die_line(0,"didn't find variable",tmp.data);
			}
		}
		q=h->name;
		while (*q) {
			char *r;
			for (r=q; *r!=','&& *r;r++) /* nothing */;
			if (q!=r) {
				char *slash;
				char *dash;
				char old_r=*r;
				*r=0;
				slash=strchr(q,'/');
				dash=strchr(q,'-');
				if (slash) {
					struct in_addr in;
					unsigned long ul;
					unsigned long mask=0; /* get rid of warning */
					*slash++=0;
					if (0==inet_aton (q, &in)) format("not an IP address");
					in.s_addr=ntohl(in.s_addr);
					if (0==str2ulong(slash,&ul,0)) format("not a number after /");
					if (ul>32) format("/number too large");
					switch((int)ul) {
					case 20: mask=0xFFFFF000; break;
					case 21: mask=0xFFFFF800; break;
					case 22: mask=0xFFFFFC00; break;
					case 23: mask=0xFFFFFE00; break;
					case 24: mask=0xFFFFFF00; break;
					case 25: mask=0xFFFFFF80; break;
					case 26: mask=0xFFFFFFC0; break;
					case 27: mask=0xFFFFFFE0; break;
					case 28: mask=0xFFFFFFF0; break;
					case 29: mask=0xFFFFFFF8; break;
					case 30: mask=0xFFFFFFFC; break;
					case 31: mask=0xFFFFFFFE; break;
					case 32: mask=0xFFFFFFFF; break;
					default: mask_bad_range("/number <20"); break;
					}
					for (ul=(in.s_addr & mask); (ul & mask) == (in.s_addr & mask); ul++) {
						struct in_addr in2;
						const char *s;
						in2.s_addr=htonl(ul);
						s=inet_ntoa(in2);
						add_one(io_tmp,s,strlen(s),data.data,data.len);
					}
				} else if (dash) {
					/* substitute [???.][A]-[Z][.] */
					static uostr_t s=UOSTR_INIT;
					int start,end;
					char *dot;
					*dash++=0;
					dot=strrchr(q,'.');
					uostr_cut(&s,0);
					if (dot) {
						unsigned long ul;
						dot++;
						uostr_xadd_mem(&s,q,dot-q); /* include . */
						if (0==str2ulong(dot,&ul,10))
							start=0;
						else
							start=ul;
					} else {
						unsigned long ul;
						if (0==str2ulong(q,&ul,10))
							start=0;
						else
							start=ul;
					}
					/* now get end of range */
					{
						size_t x;
						unsigned long ul;
						x=str2ulong(dash,&ul,10);
						if (x==0 || ul >255 ) end=255;
						else end=ul;
						dash+=x;
					}
					/* loop through the range */
					{
						int i;
						size_t mpos;
						mpos=s.len;
						for (i=start;i<=end;i++)
						{
							char numbuf[STR_ULONG];
							str_ulong(numbuf,i);
							uostr_xadd_cstr(&s,numbuf);
							uostr_xadd_cstr(&s,dash);
							add_one(io_tmp,s.data,s.len,data.data,data.len);
							uostr_cut(&s,mpos);
						}
					}
				} else 
					add_one(io_tmp,q,r-q,data.data,data.len);
				*r=old_r;
			}
			if (!*r) q=r;
			else q=r+1;
		}
	}
}


int 
main (int argc, char **argv)
{
	char *fntemp;
	char *fn;
	uoio_t io_i;
	uoio_t io_t;
	int fd;
	uint32 len;
	uint32 u;
	int i;
	static uostr_t inputdata=UOSTR_INIT;
#ifdef HAVE_LIBEFENCE
    /* hack around efence :-) */
	{int fd1,fd2;void *waste;
	 if (-1==(fd1=open("/dev/null",O_WRONLY))) _exit(0);
	 if (-1==(fd2=dup(2))) _exit(0);
	 if (-1==(dup2(fd1,2))) _exit(0);
	 waste=malloc(1);
	 if (-1==(dup2(fd2,2))) _exit(0);
	 close(fd1); close(fd2);
	}
#endif

	argv0 = strrchr(argv[0],'/');
	if (!argv0++) argv0=argv[0];

	uoio_assign_w (&io_e, 2, write, 0);

	fn = argv[1];
	if (!fn)
		usage ();

	uogetopt("utftpd_make",PACKAGE,VERSION,&argc,argv,uogetopt_out,"usage: utftpd_make cdbfile tmpfile [configfile]",myopts,
		"  Input will be read from stdin if no configuration file is specified\n"
		"  on the command line.\n"
		"  cdbfile and tmpfile have to be located on the same filesystem.\n"
		"  Report bugs to uwe-utftpd@bulkmail.ohse.de"
		);

	fntemp = argv[2];
	if (!fntemp)
		usage ();
	if (argc != 3 && argc != 4)
		usage ();


	cdbm=(struct cdbmake *) malloc(sizeof(struct cdbmake));
	if (!cdbm) nomem();
	cdbmake_init (cdbm);

	if (argc==4) {
		fd=open(argv[3],O_RDONLY);
		if (fd==-1) die(errno,"cannot open input file");
		uoio_assign_r (&io_i, fd, read, 0);
	} else
		uoio_assign_r (&io_i, 0, read, 0);
	fd = open (fntemp, O_RDWR | O_TRUNC | O_CREAT
#if !defined(HAVE_FSYNC) && defined(O_SYNC)
		| O_SYNC
#endif
		, 0644);
	if (fd == -1) die (errno, "cannot open temporary file");
	uoio_assign_w (&io_t, fd, write, 0);

	for (i = 0; i < (int) sizeof (cdbm->final); ++i)
		if (-1 == uoio_write_mem (&io_t, " ", 1))
			writeerror ();

	pos = sizeof (cdbm->final);

	while (1) {
		ssize_t datalen;
		char *q;
		datalen = uoio_getdelim_zc (&io_i, &q, '\n');
		if (datalen == 0)
			break;
		if (datalen == -1) format ("read error");
		if (q[datalen-1]!='\n') format ("no linefeed at end of input");
		uostr_xadd_mem(&inputdata,q,datalen);
	}
	uostr_x0(&inputdata);
	my_parser(inputdata.data);
	/* now we have some hosts in host_anker */
	add_hosts(&io_t);
	if (!cdbmake_split (cdbm, malloc))
		nomem ();

	for (i = 0; i < 256; ++i) {
		len = cdbmake_throw (cdbm, pos, i);
		for (u = 0; u < len; ++u) {
			cdbmake_pack (packbuf, cdbm->hash[u].h);
			cdbmake_pack (packbuf + 4, cdbm->hash[u].p);
			if (-1 == uoio_write_mem (&io_t, packbuf, 8))
				writeerror ();
			pos = safeadd (pos, (uint32) 8);
		}
	}
	if (-1 == uoio_flush (&io_t))
		writeerror ();
	uoio_destroy (&io_t);
	if (-1 == lseek (fd, 0, SEEK_SET))
		die (errno, "unable to seek");
	uoio_assign_w (&io_t, fd, write, 0);
	if (-1 == uoio_write_mem (&io_t, cdbm->final, sizeof (cdbm->final)))
		writeerror ();
	if (-1 == uoio_flush (&io_t))
		writeerror ();
	uoio_destroy (&io_t);

#ifdef HAVE_FSYNC
	if (fsync (fd) == -1) die (errno, "unable to fsync");
#else
# ifndef (O_SYNC)
	sync();
# endif
#endif
	if (close (fd) == -1) die (errno, "unable to close");

	if (rename (fntemp, fn))
		die (errno, "unable to rename");
	exit (0);
}


