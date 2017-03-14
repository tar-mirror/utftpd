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

uostr_t *
uostr_add_uostr(uostr_t *u,const uostr_t *v)
{
	return uostr_add_mem(u,v->data,v->len);
}
uostr_t *
uostr_xadd_uostr(uostr_t *u, const uostr_t *v)
{ uostr_t *r=uostr_add_mem(u,v->data,v->len); if (!r) uostr_xallocerr("uostr_xadd_uostr"); return r;}

uostr_t *
uostr_dup_uostr(uostr_t *u,const uostr_t *v)
{
	return uostr_dup_mem(u,v->data,v->len);
}
uostr_t *
uostr_xdup_uostr(uostr_t *u, const uostr_t * v)
{ uostr_t *r=uostr_dup_mem(u,v->data,v->len); if (!r) uostr_xallocerr("uostr_xdup_uostr");return r; }
