/*
 * Copyright (C) 1999 Uwe Ohse
 * 
 * placed in the public domain.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * Contact: uwe@ohse.de
 */
#ifndef TYPESIZE_H
#define TYPESIZE_H
#include "config.h"

/* Note: i'm not very happy about this.
 * But some compilers define a int16_t, others dont. 
 * And autoconfs AC_CHECK_TYPE is simply to stupid to get
 * this right.
 */

#ifndef SIZEOF_SHORT
/* no #error, pre ansi compiler tend to warn .. */
error SIZEOF_SHORT not defined, did you include the macros in config.h and acconfig.h?
#endif

/* int16 */
#if SIZEOF_SHORT >= 2 
typedef short uo_int16;
#else
typedef int uo_int16;
#endif

/* uo_int16 */
#if SIZEOF_UNSIGNED_SHORT >= 2 
typedef unsigned short uo_uint16;
#else
typedef unsigned int uo_uint16;
#endif

/* int32 */
#if SIZEOF_SHORT >= 4 
typedef short uo_int32;
#else 
# if SIZEOF_INT >= 4 
typedef int uo_int32;
# else
#  if SIZEOF_LONG >= 4 
typedef long uo_int32;
#  else
#   if SIZEOF_LONG_LONG >= 4 
typedef long long uo_int32;
#   else
error no 32bit integer available
#   endif
#  endif
# endif
#endif

/* uo_int32 */
#if SIZEOF_UNSIGNED_SHORT >= 4 
typedef unsigned short uo_uint32;
#else 
# if SIZEOF_UNSIGNED_INT >= 4 
typedef unsigned int uo_uint32;
# else
#  if SIZEOF_UNSIGNED_LONG >= 4 
typedef unsigned long uo_uint32;
#  else
#   if SIZEOF_UNSIGNED_LONG_LONG >= 4 
typedef unsigned long long uo_uint32;
#   else
error no 32bit unsigned integer available
#   endif
#  endif
# endif
#endif

/* int64 */
#define HAVE_INT64
#if SIZEOF_INT >= 8 
typedef int uo_int64;
#else
# if SIZEOF_LONG >= 8
typedef long uo_int64;
# else
#  if SIZEOF_LONG_LONG >= 8
typedef long long uo_int64;
#  else
#   undef HAVE_INT64
#  endif
# endif
#endif

/* uo_int64 */
#define HAVE_UINT64
#if SIZEOF_UNSIGNED_INT >= 8 
typedef int uo_uint64;
#else
# if SIZEOF_UNSIGNED_LONG >= 8 
typedef long uo_uint64;
# else
#  if SIZEOF_UNSIGNED_LONG_LONG >= 8 
typedef long long uo_uint64;
#  else
#   undef HAVE_INT64
#  endif
# endif
#endif

#endif
