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

/* isn't is horrible? */

#ifndef TIMESELECTSYSDEP_H
#define TIMESELECTSYSDEP_H
#include "config.h"
/*
AUTOCONF AC_HEADER_TIME
AUTOCONF AC_CHECK_HEADERS(sys/select.h)
AUTOCONF UO_HEADER_SYS_SELECT
*/
#include <sys/types.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
# define SYS_TIME_INCLUDED
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
#  define SYS_TIME_INCLUDED
# else
#  include <time.h>
# endif
#endif
#ifdef SYS_TIME_WITHOUT_SYS_SELECT
# undef HAVE_SYS_SELECT_H
#endif

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#else
extern int select();
#endif
#ifdef __BEOS__
#include <socket.h>
#endif

#endif
