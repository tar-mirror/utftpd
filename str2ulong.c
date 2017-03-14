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
/* written mainly to get around the need to include a strtoul.c in the distributions */
#include "config.h"
#include "str2num.h"

int
str2ulong (const char *s, unsigned long *ul, int base)
{
	unsigned long x = 0;
	const char *t = s;
	if (base == 0 && *s == '0') {
		if (s[1] == 'x' || s[1] == 'X') {
			base = 16;
			s += 2;
		} else {
			base = 8;
			s++;
			if (!*s) {
				*ul = 0;
				return 1;
			}
		}
	} else if (base == 0)
		base = 10;
	else if (base == 16 && *s == '0') {		/* as strtoul compatible as possible */
		if ((s[1] == 'x' || s[1] == 'X') && s[2])
			s += 2;
	}
	/* i assume something about processor behaviour in case of overflow ... */
	/* and, of course, about the character set */
	while (*s) {
		unsigned int c = (unsigned char) *s;
		int v;
		unsigned long old;
		if (c < '0' || c > '9') {
			if (c >= 'a' && c <= 'z')
				v = (c - 'a');
			else if (c >= 'A' && c <= 'Z')
				v = (c - 'A');
			else
				goto out;
			v += 10;
		} else
			v = c - '0';
		if (v >= base)
			goto out;
		old = x;
		x *= (unsigned long) base;
		x += (unsigned long) v;
		if (x < old)
			return -1;
		s++;
	}
  out:
	*ul = x;
	return s - t;
}
