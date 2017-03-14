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
 * 
 * As a special exception this source may be used as part of the
 * SRS project by CORE/Computer Service Langenbach
 * regardless of the copyright they choose.
 * 
 * Contact: uwe@ohse.de
 */
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef INADDR_NONE
/* slowlaris ... */
#define INADDR_NONE             ((unsigned long) 0xffffffff)
#endif

int 
inet_aton (name,addr)
	const char *name;
	struct in_addr *addr;
{
	struct in_addr val;
	val.s_addr=inet_addr(name);
	if (val.s_addr==INADDR_NONE)
		return 0;
	*addr=val;
	return 1;
}

