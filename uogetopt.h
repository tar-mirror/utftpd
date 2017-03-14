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
#ifndef UOGETOPT_H
#define UOGETOPT_H

#define UOGO_FLAG   0   /* (*var) shall be "val": this means _no_ argument */
#define UOGO_FLAGOR 1   /* (*var) shall be "(*arg)|val": this means _no_ argument */
#define UOGO_STRING 2   /* (*var) shall be pointer to argument */
#define UOGO_ULONG  4   /* (*var)=strtoul(argument) */
#define UOGO_LONG   5   /* (*var)=strtol(argument) */
#define UOGO_CALLBACK 6 /* var is function ptr: int (*fn)(uogetopt_t *, const char *) */
#define UOGO_NOOPT  7   /* this is not really an option */
#define UOGO_PRINT  8   /* just print the help text and exit */
/* to OR into the argtype */
#define UOGO_HIDDEN  0x10000 /* do not show this option in --help or --longhelp */

typedef struct {
	char shortname;
	const char *longname;
	int argtype; /* 0 or 1 no argument, set (*arg) from val. */
	void *var; /* mandatory */
	int val; /* val for *var in case of type 0 or 1 */
	const char *help;
	const char *paraname;
} uogetopt_t;

#ifndef P__
#define P__(x) x
#endif
void uogetopt P__((
	const char *prog, const char *package, const char *version,
	int *argc, char **argv, /* note: "int *" */
	void (*out)(int iserr,const char *), 
	const char *head,
	uogetopt_t *,
	const char *tail));

void uogetopt_out P__((int iserr,const char *s));

#endif
