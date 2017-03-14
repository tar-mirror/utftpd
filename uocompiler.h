/*
 * Copyright (C) 1999 Uwe Ohse
 * 
 * This source is public domain: do with it whatever you want.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * Contact: uwe@ohse.de
 */
#ifndef UOCOMPILER_H
#define UOCOMPILER_H

/* include sys/types.h to get a definition of P__ if it's 
 * there (BSDI at least). */
#include <sys/types.h>

#ifndef P__
#if defined (__GNUC__) || (defined (__STDC__) && __STDC__)
#define P__(args) args
#else
#define P__(args) ()
#endif  /* GCC.  */
#endif  /* Not P__.  */

#endif
