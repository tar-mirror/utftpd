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
#include <stdlib.h>
#include <errno.h>
#include <unistd.h> /* write */
#include "uostr.h"

/* core functions */

	/* assume max. overhead of malloc() */
#define OVERHEAD (sizeof(void *)+sizeof(size_t))

#define DEBUG_FREE

#ifdef DEBUG_FREE
int uostr_debug_free=1;
struct deb {
	uostr_t *adr;
	struct deb *next;
};
static struct deb *anker;
#endif

void (*uostr_xallocfn) P__((const char *))=0;
void 
uostr_xallocerr(const char *fn)
{
	if (uostr_xallocfn) uostr_xallocfn(fn);
	else {
		const char *p=fn; while(*p!=0) p++;
		write(2,"out of memory in ",17);
		write(2,fn,p-fn);
		write(2,"\n",1);
	}
	exit(1);
}

uostr_t *
uostr_allocmore(uostr_t *u, size_t bytes)
{
	size_t n;
	size_t t;
	if (!u->data) n=bytes;
	else n=bytes+u->size;
	for (t=32;;t*=2) {
		if (t-OVERHEAD>=n)
			break;
	}
	t-=OVERHEAD;
	if (!u->data) {
		u->len=0;
#ifdef DEBUG_FREE
		if (uostr_debug_free) {
			struct deb *neu,*x;	
			for (x=anker;x;x=x->next) { if (x->adr==u) break; }
			if (x) { write(2,"reusing uostr\n",14); abort(); }
			else {
				neu=(struct deb *) malloc(sizeof(struct deb));
				if (!neu) {
					uostr_debug_free=0;
					while (anker) { struct deb *next; next=anker->next; free(anker); anker=next; }
				} else { neu->adr=u; neu->next=anker; anker=neu; }
			}
		}
#endif
		u->data=(char *) malloc(t);
		if (!u->data) { errno=ENOMEM; return 0;}
	}
	else {
		char *tmp=(char *) realloc(u->data,t);
		if (!tmp) return 0;
		u->data=tmp;
	}
	u->size=t;
	return u;
}
uostr_t *
uostr_xallocmore(uostr_t *u, size_t bytes)
{ uostr_t *r=uostr_allocmore(u,bytes); if (r) return r; uostr_xallocerr("uostr_xallocmore"); return(0); }

void
uostr_freedata(uostr_t *u)
{
	if (u->data) {
#ifdef DEBUG_FREE
		if (uostr_debug_free) {
			struct deb *x,*l;
			for (x=anker,l=0;x;) {
				if (x->adr==u) {
					struct deb *n=x->next;
					free(x);
					if (l) l->next=n;
					else anker=n;
					x=n;
					continue;
				}
				l=x;
				x=x->next;
			}
		}
#endif
		free(u->data);
		u->data=0;
	}
}
void
uostr_forget(uostr_t *u)
{
#ifdef DEBUG_FREE
	if (uostr_debug_free) {
		struct deb *x,*l;
		for (x=anker,l=0;x;) {
			if (x->adr==u) {
				struct deb *n=x->next;
				free(x);
				if (l) l->next=n;
				else anker=n;
				x=n;
				continue;
			}
			l=x;
			x=x->next;
		}
	}
#endif
}
