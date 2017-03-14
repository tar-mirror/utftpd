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

/* written mainly to get around the need to include a strtol.c in the distributions */
#include "config.h"
#include "str2num.h"

int 
str2long(const char *s, long *lo, int base)
{
	int sign=0;
	unsigned long ul;
	int len;
	int loff=0;
	long l;
	if (*s=='-') {s++; sign=1; loff++;}
	else if (*s=='+') {s++; loff++;}
	len=str2ulong(s,&ul,base);
	if (len<0) return len;
	if (len==0) {*lo=0;return len+loff;}  /* consistence with str2ulong */
	if (!sign) {
		l=(long)ul; 
		if (l<0 || ul != (unsigned long)l) return -1;
		*lo=l;
		return len+loff;
	}
	l=(long)ul; if (ul != (unsigned long)l) return -1;
	*lo=0-l;
	return len+loff;
}
