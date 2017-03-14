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
#ifndef UOSTR_H
#define UOSTR_H

#ifndef P__
#define P__(x) x
#endif
#include <stddef.h>

typedef struct uostr_t {
	char *data;
	size_t size;
	size_t len;
} uostr_t;
#define UOSTR_INIT {0,0,0} /* really only care about data */

uostr_t *uostr_alloc P__((void)); /* mallocs a uostr_t and inits with 0 */
void uostr_free P__((uostr_t *)); /* free(uostr_t), after free(uostr_t->data) */
void uostr_freedata P__((uostr_t *)); /* free(uostr_t->data) */
void uostr_forget P__((uostr_t *)); /* get around "reusing uostr" check */
extern void (*uostr_xallocfn) P__((const char *)); /* called by x-functions in case of oom */
void uostr_xallocerr P__((const char *fn)); /* internal function, leave alone */

/* be careful - if u->data is NULL then u->len and u->size need not to contain any information */
/* both are boolean: 0/NULL is "error" */
#define uostr_needmore(u,bytes) \
	(!(u)->data ? uostr_allocmore((u),(bytes)) : \
		(((bytes)+(u)->len>(u)->size) ? uostr_allocmore((u),(bytes)) : u))
#define uostr_xneedmore(u,bytes) \
	(!(u)->data ? uostr_allocmore((u),(bytes)) : \
		(((bytes)+(u)->len>(u)->size) ? uostr_allocmore((u),(bytes)) : u))
#define uostr_need(u,bytes) \
	(!(u)->data ? uostr_allocmore((u),(bytes)) : \
		(((bytes)>(u)->size) ? uostr_allocmore((u),(bytes)-(u)->size) : u))
#define uostr_xneed(u,bytes) \
	(!((u)->data) ? uostr_allocmore((u),(bytes)) : \
		(((bytes)>(u)->size) ? uostr_allocmore((u),(bytes)-(u)->size) : u))
uostr_t * uostr_allocmore P__((uostr_t *u, size_t bytes));
uostr_t * uostr_xallocmore P__((uostr_t *u, size_t bytes));

#define UOSTR_EMPTY(x) ((x)->data?(x)->len==0:1)
#define UOSTR_EMPTY0(x) ((x)->data?(x)->len==1 && (x)->data[0]==0? 1: (x)->len==0 ? 1 : 0 : 1)


#define uostr_0(u) uostr_add_char(u,0)
#define uostr_x0(u) uostr_xadd_char(u,0)

uostr_t *uostr_dup_cstr P__((uostr_t *,const char *)); /* strdup */
uostr_t *uostr_dup_cstrmulti P__((uostr_t *u,...)); /* dup first, concat rest */
uostr_t *uostr_dup_char P__((uostr_t *,const char)); /* char2uostr_t */
uostr_t *uostr_dup_uostr P__((uostr_t *,const uostr_t *));
uostr_t *uostr_dup_mem P__((uostr_t *,const char *, size_t len)); /* */
uostr_t *uostr_add_cstr P__((uostr_t *,const char *)); /* */
uostr_t *uostr_add_cstrmulti P__((uostr_t *u,...));
uostr_t *uostr_add_char P__((uostr_t *,const char)); /* */
uostr_t *uostr_add_uostr P__((uostr_t *,const uostr_t *)); /* */
uostr_t *uostr_add_mem P__((uostr_t *,const char *, size_t len)); /* */

uostr_t *uostr_xdup_cstr P__((uostr_t *,const char *)); /* strdup */
uostr_t *uostr_xdup_cstrmulti P__((uostr_t *u,...)); /* dup first, concat rest */
uostr_t *uostr_xdup_char P__((uostr_t *,const char)); /* char2uostr_t */
uostr_t *uostr_xdup_uostr P__((uostr_t *,const uostr_t *));
uostr_t *uostr_xdup_mem P__((uostr_t *,const char *, size_t len)); /* */
uostr_t *uostr_xadd_cstr P__((uostr_t *,const char *)); /* */
uostr_t *uostr_xadd_cstrmulti P__((uostr_t *u,...));
uostr_t *uostr_xadd_char P__((uostr_t *,const char)); /* */
uostr_t *uostr_xadd_uostr P__((uostr_t *,const uostr_t *)); /* */
uostr_t *uostr_xadd_mem P__((uostr_t *,const char *, size_t len)); /* */

uostr_t *uostr_cut P__((uostr_t *,long new_len)); /* !!! long, may be negativ (cut by -x chars) */
#define uostr_fcut(u) do {(u)->len=0; if ((u)->data) (u)->data[0]='Z';} while(0)


#endif
