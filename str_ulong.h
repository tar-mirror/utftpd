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

#ifndef STR_ULONG_H
#define STR_ULONG_H

#define STR_ULONG 64 /* 64 bytes, far more than needed for 2^128 */
#include <stddef.h> /* need size_t */
#ifndef P__
#define P__(x) x
#endif

size_t str_ulong_base P__((char *s, unsigned long u, unsigned int base));
#define str_ulong(s,u) str_ulong_base(s,u,10)

#endif
