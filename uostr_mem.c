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
#include "uostr.h"

#ifdef __GNUC__
#ifndef __cplusplus
inline /* just too aggressive */
#endif
#endif              
  uostr_t *
uostr_add_mem(uostr_t *u, const char *v, size_t len)
{
	char *p;
	int runs;
	if (!uostr_needmore(u,len+1)) return 0;
	if (!len) goto out;
	p=u->data+u->len;
	runs=(len+7)/8;
	switch ((int)(len%8)) {
		while (runs) {
	case 0: /* HPUX 9.00 cc barfed about "default" here: 'default' should appear at most once in 'switch' */
			*p++=*v++;
	case 7: *p++=*v++;
	case 6: *p++=*v++;
	case 5: *p++=*v++;
	case 4: *p++=*v++;
	case 3: *p++=*v++;
	case 2: *p++=*v++;
	case 1: *p++=*v++;
		--runs;
		} /* while */
	} /* case duff */
  out:
	u->len+=len;
	u->data[u->len]='Z'; /* clever idea stolen from djb */
	return u;
}
uostr_t *
uostr_xadd_mem(uostr_t *u, const char *v, size_t bytes)
{ uostr_t *r=uostr_add_mem(u,v,bytes); if (!r) uostr_xallocerr("uostr_xadd_mem"); return r; }


uostr_t *
uostr_dup_mem(uostr_t *u,const char *v, size_t len)
{
	u->len=0;
	return uostr_add_mem(u,v,len);
}
uostr_t *
uostr_xdup_mem(uostr_t *u, const char *v, size_t bytes)
{ uostr_t *r; u->len=0;r=uostr_add_mem(u,v,bytes); if (!r) uostr_xallocerr("uostr_xdup_mem"); return r; }


