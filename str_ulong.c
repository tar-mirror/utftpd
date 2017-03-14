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
#include "str_ulong.h"

size_t
str_ulong_base(char *s, unsigned long u, unsigned int base)
{
	const char *b="0123456789abcdefghijklmnopqrstuvwxyz";
	unsigned int len=1;
	unsigned long tmp=u;
	char *end;
	while (tmp>=base) {
		len++;
		tmp/=base;
	}
	if (!s)
		return len;
	end=s=s+len;
	while (u>=base) {
		s--;
		*s=b[u%base];
		u/=base;
	}
	s--;
	*s=b[u%base];
	*end=0;
	return len;
}

#ifdef TEST
int main(int argc, char **argv)
{
	char b[STR_ULONG];
	size_t l;
	l=str_ulong_base(b, strtoul(argv[1],0,0), strtoul(argv[2],0,0));
	write(1,b,l);
	write(1,"\n",1);
	exit(1);
}
#endif
