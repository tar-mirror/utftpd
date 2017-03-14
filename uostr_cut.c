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

/* should use ssize_t here, but that's not portable, and requiring
 * "config.h" and autoconf for every application doesn't seem to
 * be appropriate.
 */
uostr_t *
uostr_cut(uostr_t *u,long new_len)
{	
	if (!u->data) {
		if (new_len!=0) return 0;
		u->len=0;
		return u;
	}
	if (new_len<0) { /* -x: cut by x */
		new_len=-new_len;
		if ((size_t)new_len>u->len) u->len=0;
		else u->len-=new_len;
		u->data[u->len]='Z'; /* clever idea stolen from djb */
		return u;
	}
	if ((size_t)new_len<=u->len) {
		u->len=new_len;
		u->data[u->len]='Z'; /* clever idea stolen from djb */
		return u;
	}
	return 0;
}
