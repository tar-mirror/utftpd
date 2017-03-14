
/*  A Bison parser, made from parser.y
 by  GNU Bison version 1.27
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	SYM_CLASS	257
#define	STRING	258
#define	SYM_OVERRIDE	259
#define	SYM_PLUSGLEICH	260
#define	SYM_CLIENT	261

#line 17 "parser.y"

#include "config.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "uostr.h"
#include "utftpd_make.h"
#include "str_ulong.h"
#include <stdlib.h> /* malloc */
#include <string.h>
#include <unistd.h>

static int yyerror(const char *s);
static int yylex(void);
/* static int parse_ip_list (char *val, int *count, int *size, ip_list_t ** ar); */
static int yyparse(void);
static int in_oldstyle=0;

static const char *lex_ptr;
int parser_lineno;
static const char *last_symbol_name;

static uostr_t this_string=UOSTR_INIT;

static void *my_malloc(size_t x) {void *p=malloc(x); if (!p) die_line(0,"out of memory",0); return p;}

struct myhost *host_anker;
struct myclass {
	char *name;
	struct varlist *vars;
	struct myclass *next;
};
static struct myclass *class_anker;
static void 
start_classdef(const char *name)
{
	struct myclass *n;
	size_t x;
	for (n=class_anker;n;n=n->next) {
		if (0==strcmp(name,n->name)) 
			die_line(0,"duplicate class name",name);
	}
	n=(struct myclass *) my_malloc(sizeof(struct myclass));
	x=strlen(name)+1;
	n->name=(char *) my_malloc(x);
	memcpy(n->name,name,x);
	n->vars=0;
	n->next=class_anker;
	class_anker=n;
}
static void
add_to_class_varlist(int art, const char *name, const char *val)
{
	struct varlist *v,*w=NULL;
	size_t x;
	for (v=class_anker->vars;v;v=v->next)
	{
		w=v;
		if (0==strcmp(v->name,name)) {
			if (art=='=') die_line(0,"duplicate assignment to variable ",name);
			if (art==SYM_PLUSGLEICH) {
				uostr_t s;
				s.data=0;
				uostr_xdup_cstr(&s,v->value);
				uostr_xadd_cstr(&s,val);
				uostr_x0(&s);
				free(v->value);
				v->value=s.data;
				v->lineno=parser_lineno;
				uostr_forget(&s);
				return;
			} else { /* override */
				x=strlen(val)+1;
				free(v->value);
				v->value=(char *) my_malloc(x);
				v->lineno=parser_lineno;
				memcpy(v->value,val,x);
				return;
			}
		}
	}
	v=(struct varlist *) my_malloc(sizeof(struct varlist));
	x=strlen(name)+1;
	v->name=(char *)my_malloc(x);
	memcpy(v->name,name,x);
	x=strlen(val)+1;
	v->value=(char *)my_malloc(x);
	v->lineno=parser_lineno;
	memcpy(v->value,val,x);
	v->next=0;
	if (w) w->next=v;
	else class_anker->vars=v;
}
/* conflict resolution is bark */
static void
do_class_inherit(const char *cl)
{
	struct myclass *mc;
	for (mc=class_anker;mc;mc=mc->next) {
		if (0==strcmp(cl,mc->name)) {
			struct varlist *v;
			for(v=mc->vars;v;v=v->next) {
				add_to_class_varlist('=', v->name,v->value);
			}
			return;
		}
	}
	die_line(0,"unknown class",cl);
}

static void 
start_hostdef(const char *name)
{
	struct myhost *n;
	size_t x;
	for (n=host_anker;n;n=n->next) {
		if (0==strcmp(name,n->name)) 
			die_line(0,"duplicate host name",name);
	}
	n=(struct myhost *) my_malloc(sizeof(struct myhost));
	x=strlen(name)+1;
	n->name=(char *)my_malloc(x);
	memcpy(n->name,name,x);
	n->vars=0;
	n->next=host_anker;
	host_anker=n;
}
static void
add_to_host_varlist(int type, const char *name, const char *val)
{
	struct varlist *v,*w=NULL;
	size_t x;
	for (v=host_anker->vars;v;v=v->next)
	{
		w=v;
		if (0==strcmp(v->name,name)) {
			if (type=='=') die_line(0,"duplicate assignment to host variable",name);
			if (type==SYM_PLUSGLEICH) {
				uostr_t s;
				s.data=0;
				uostr_xdup_cstr(&s,v->value);
				uostr_xadd_cstr(&s,val);
				uostr_x0(&s);
				free(v->value);
				v->lineno=parser_lineno;
				v->value=s.data;
				uostr_forget(&s);
				return;
			} else { /* override */
				x=strlen(val)+1;
				free(v->value);
				v->value=(char *) my_malloc(x);
				v->lineno=parser_lineno;
				memcpy(v->value,val,x);
				return;
			}
		}
	}
	v=(struct varlist *) my_malloc(sizeof(struct varlist));
	x=strlen(name)+1;
	v->name=(char *) my_malloc(x);
	memcpy(v->name,name,x);
	x=strlen(val)+1;
	v->lineno=parser_lineno;
	v->value=(char *) my_malloc(x);
	memcpy(v->value,val,x);

	v->next=0;
	if (w) w->next=v;
	else host_anker->vars=v;
}
static void
do_host_inherit(const char *cl)
{
	struct myclass *mc;
	for (mc=class_anker;mc;mc=mc->next) {
		if (0==strcmp(cl,mc->name)) {
			struct varlist *v;
			for(v=mc->vars;v;v=v->next) {
				add_to_host_varlist('=', v->name,v->value);
			}
			return;
		}
	}
	die_line(0,"unknown class",cl);
}


#line 209 "parser.y"
typedef union {
	int integer;
	char *string;
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		77
#define	YYFLAG		-32768
#define	YYNTBASE	17

#define YYTRANSLATE(x) ((unsigned)(x) <= 261 ? yytranslate[x] : 38)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    12,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    13,
    14,     2,     2,    16,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     6,     5,     2,
    15,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     3,     2,     4,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     7,     8,     9,    10,
    11
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     3,     6,     9,    12,    13,    14,    20,    22,    25,
    30,    34,    35,    41,    42,    45,    49,    51,    53,    57,
    58,    62,    66,    71,    73,    74,    79,    80,    86,    90,
    92,    93,    96,    98,    99,   104,   106,   110,   111,   115,
   119
};

static const short yyrhs[] = {    30,
    17,     0,    22,    17,     0,     5,    17,     0,    18,    17,
     0,     0,     0,     8,    19,     6,    20,    12,     0,    21,
     0,    20,    21,     0,     8,    13,     8,    14,     0,     8,
    13,    14,     0,     0,    11,     8,    23,    24,    25,     0,
     0,     6,    28,     0,     3,    26,     4,     0,     5,     0,
    27,     0,    27,     5,    26,     0,     0,     8,    15,     8,
     0,     8,    10,     8,     0,     9,     8,    15,     8,     0,
     8,     0,     0,     8,    29,    16,    28,     0,     0,     7,
     8,    31,    33,    32,     0,     3,    36,     4,     0,     5,
     0,     0,     6,    34,     0,     8,     0,     0,     8,    35,
    16,    34,     0,    37,     0,    37,     5,    36,     0,     0,
     8,    15,     8,     0,     8,    10,     8,     0,     9,     8,
    15,     8,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   218,   219,   220,   221,   222,   224,   224,   226,   227,   229,
   230,   233,   233,   235,   236,   238,   239,   241,   242,   243,
   245,   247,   248,   250,   251,   251,   254,   254,   256,   257,
   259,   260,   262,   263,   263,   265,   266,   267,   269,   271,
   272
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","'{'","'}'",
"';'","':'","SYM_CLASS","STRING","SYM_OVERRIDE","SYM_PLUSGLEICH","SYM_CLIENT",
"'\\n'","'('","')'","'='","','","wholefile","oldstyle","@1","oldstylevars","oldstylevar",
"hostdef","@2","maybehostinherit","hostdefmain","hostdefvars","hostdefvar","hostdefinherit",
"@3","classdef","@4","classdefmain","maybeclassinherit","inheritdef","@5","classdefargs",
"classdefarg", NULL
};
#endif

static const short yyr1[] = {     0,
    17,    17,    17,    17,    17,    19,    18,    20,    20,    21,
    21,    23,    22,    24,    24,    25,    25,    26,    26,    26,
    27,    27,    27,    28,    29,    28,    31,    30,    32,    32,
    33,    33,    34,    35,    34,    36,    36,    36,    37,    37,
    37
};

static const short yyr2[] = {     0,
     2,     2,     2,     2,     0,     0,     5,     1,     2,     4,
     3,     0,     5,     0,     2,     3,     1,     1,     3,     0,
     3,     3,     4,     1,     0,     4,     0,     5,     3,     1,
     0,     2,     1,     0,     4,     1,     3,     0,     3,     3,
     4
};

static const short yydefact[] = {     5,
     5,     0,     6,     0,     5,     5,     5,     3,    27,     0,
    12,     4,     2,     1,    31,     0,    14,     0,     0,     0,
     0,     8,     0,     0,    33,    32,    38,    30,    28,     0,
     7,     9,    24,    15,    20,    17,    13,     0,     0,     0,
     0,    36,     0,    11,     0,     0,     0,     0,    18,     0,
     0,     0,     0,    29,    38,    10,     0,     0,     0,     0,
    16,    20,    35,    40,    39,     0,    37,    26,    22,    21,
     0,    19,    41,    23,     0,     0,     0
};

static const short yydefgoto[] = {     8,
     5,    10,    21,    22,     6,    17,    24,    37,    48,    49,
    34,    45,     7,    15,    29,    19,    26,    38,    41,    42
};

static const short yypact[] = {     3,
     3,    -7,-32768,     8,     3,     3,     3,-32768,-32768,     7,
-32768,-32768,-32768,-32768,    21,    20,    23,    22,    -1,    18,
    10,-32768,    24,    16,    19,-32768,    15,-32768,-32768,    -5,
-32768,-32768,    25,-32768,    17,-32768,-32768,    26,     2,    28,
    29,    32,    30,-32768,    27,     5,    31,    34,    35,    22,
    37,    38,    33,-32768,    15,-32768,    24,    39,    41,    36,
-32768,    17,-32768,-32768,-32768,    42,-32768,-32768,-32768,-32768,
    44,-32768,-32768,-32768,    53,    54,-32768
};

static const short yypgoto[] = {     0,
-32768,-32768,-32768,    13,-32768,-32768,-32768,-32768,    -6,-32768,
    -2,-32768,-32768,-32768,-32768,-32768,     9,-32768,     6,-32768
};


#define	YYLAST		61


static const short yytable[] = {    75,
     9,    27,    43,    28,    12,    13,    14,     1,    44,     2,
     3,    51,    16,     4,    58,    11,    52,    20,    35,    59,
    36,    31,    39,    40,    46,    47,    18,    20,    23,    25,
    30,    33,    54,    32,   -34,    53,    55,    61,    60,    62,
   -25,    50,    57,    56,    64,    65,    69,    66,    70,    73,
    71,    74,    76,    77,    68,    72,     0,     0,    63,     0,
    67
};

static const short yycheck[] = {     0,
     8,     3,     8,     5,     5,     6,     7,     5,    14,     7,
     8,    10,     6,    11,    10,     8,    15,     8,     3,    15,
     5,    12,     8,     9,     8,     9,     6,     8,     6,     8,
    13,     8,     4,    21,    16,     8,     5,     4,     8,     5,
    16,    16,    16,    14,     8,     8,     8,    15,     8,     8,
    15,     8,     0,     0,    57,    62,    -1,    -1,    50,    -1,
    55
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/local/share/bison.simple"
/* This file comes from bison-1.27.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 216 "/usr/local/share/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 6:
#line 224 "parser.y"
{ start_hostdef(yyvsp[0].string); in_oldstyle=1;;
    break;}
case 10:
#line 229 "parser.y"
{ add_to_host_varlist(SYM_OVERRIDE,yyvsp[-3].string,yyvsp[-1].string); ;
    break;}
case 11:
#line 230 "parser.y"
{ add_to_host_varlist(SYM_OVERRIDE,yyvsp[-2].string,""); ;
    break;}
case 12:
#line 233 "parser.y"
{start_hostdef(yyvsp[0].string);;
    break;}
case 21:
#line 246 "parser.y"
{ add_to_host_varlist('=',yyvsp[-2].string,yyvsp[0].string); ;
    break;}
case 22:
#line 247 "parser.y"
{ add_to_host_varlist(SYM_PLUSGLEICH,yyvsp[-2].string,yyvsp[0].string); ;
    break;}
case 23:
#line 248 "parser.y"
{ add_to_host_varlist(SYM_OVERRIDE,yyvsp[-2].string,yyvsp[0].string); ;
    break;}
case 24:
#line 250 "parser.y"
{do_host_inherit(yyvsp[0].string);;
    break;}
case 25:
#line 251 "parser.y"
{do_host_inherit(yyvsp[0].string); ;
    break;}
case 27:
#line 254 "parser.y"
{start_classdef(yyvsp[0].string);
    break;}
case 33:
#line 262 "parser.y"
{do_class_inherit(yyvsp[0].string);;
    break;}
case 34:
#line 263 "parser.y"
{do_class_inherit(yyvsp[0].string); ;
    break;}
case 39:
#line 270 "parser.y"
{ add_to_class_varlist('=',yyvsp[-2].string,yyvsp[0].string); ;
    break;}
case 40:
#line 271 "parser.y"
{ add_to_class_varlist(SYM_PLUSGLEICH,yyvsp[-2].string,yyvsp[0].string); ;
    break;}
case 41:
#line 272 "parser.y"
{ add_to_class_varlist(SYM_OVERRIDE,yyvsp[-2].string,yyvsp[0].string); ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 542 "/usr/local/share/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}
#line 274 "parser.y"

#include "ctype.h"

int
my_parser(const char *s)
{
	lex_ptr=s;
	parser_lineno=1;
	return yyparse();
}

static int 
yylex(void) 
{
	const char *specials="{},=;+:";
	const char *oldstyle_specials="()";
	if (!*lex_ptr)
		return EOF;
	do {
		while (isspace((unsigned char) *lex_ptr)) {
			if (*lex_ptr=='\n') {
				if (in_oldstyle) {in_oldstyle=0; return '\n';}
				parser_lineno++;
			}
			lex_ptr++;
		}
		/* # comments */
		if (*lex_ptr=='#') {
			while (*lex_ptr && *lex_ptr!='\n')
				lex_ptr++;
		}
		/* // comments */
		if (*lex_ptr=='/' && lex_ptr[1]=='/') {
			while (*lex_ptr && *lex_ptr!='\n')
				lex_ptr++;
		}
		if (*lex_ptr=='/' && lex_ptr[1]=='*') {
			while (*lex_ptr && lex_ptr[1] && *lex_ptr!='*' && lex_ptr[1]!='/')
				lex_ptr++;
		}
	} while (isspace((unsigned char) *lex_ptr));
	if (!*lex_ptr)
		return EOF;

	last_symbol_name=NULL;
	if (*lex_ptr == '(') { lex_ptr++; last_symbol_name="("; return '('; }
	if (*lex_ptr == ')') { lex_ptr++; last_symbol_name=")"; return ')'; }
	if (!in_oldstyle && *lex_ptr == '{') { lex_ptr++; last_symbol_name="{"; return '{'; }
	if (!in_oldstyle && *lex_ptr == '}') { lex_ptr++; last_symbol_name="}"; return '}'; }
	if (!in_oldstyle && *lex_ptr == ';') { lex_ptr++; last_symbol_name=";"; return ';'; }
	if (!in_oldstyle && *lex_ptr == ',') { lex_ptr++; last_symbol_name=","; return ','; }
	if (in_oldstyle<2 && *lex_ptr == ':') { if (in_oldstyle) in_oldstyle++; lex_ptr++; last_symbol_name=":"; return ':'; }
	if (!in_oldstyle && *lex_ptr == '=') { lex_ptr++; last_symbol_name="="; return '='; }
#define DOONE(str,ret) \
	if (!strncasecmp(lex_ptr,str,sizeof(str)-1) && !isalnum(lex_ptr[sizeof(str)-1])) { \
		last_symbol_name=str; \
		lex_ptr+=sizeof(str)-1; \
		return ret; \
	} 
	DOONE("class",SYM_CLASS)
	DOONE("client",SYM_CLIENT)
	DOONE("override",SYM_OVERRIDE)
	DOONE("+=",SYM_PLUSGLEICH)
	if (*lex_ptr=='"') {
		const char *x=lex_ptr++;
		while (*lex_ptr && *lex_ptr!='"')
			lex_ptr++;
		uostr_xdup_mem(&this_string,x+1,lex_ptr-x-1);
		uostr_x0(&this_string);
		yylval.string = this_string.data;
		this_string.data=0;
		uostr_forget(&this_string);
		lex_ptr++;
		last_symbol_name="\"string\"";
		return STRING;
	}
	{
		const char *x=lex_ptr++;
		while (*lex_ptr && !isspace((unsigned char) *lex_ptr) && !strchr(in_oldstyle ? oldstyle_specials : specials,*lex_ptr))
			lex_ptr++;
		uostr_xdup_mem(&this_string,x,lex_ptr-x);
		uostr_x0(&this_string);
		yylval.string = this_string.data;
		this_string.data=0;
		uostr_forget(&this_string);
		last_symbol_name="string";
		return STRING;
	}
}

static int 
yyerror(const char *str)
{
	if (!*lex_ptr) {
		die_line(0,str,"error before end of file");
	} else {
		uostr_t s;
		size_t t;
		char *p;
		s.data=0;
		t=strlen(lex_ptr);
		if (t>32)
			t=32;
		uostr_xadd_cstr(&s,"before ");
		uostr_xadd_char(&s,'`');
		uostr_xadd_mem(&s,lex_ptr,t);
		uostr_xadd_char(&s,'\'');
		if (last_symbol_name) {
			uostr_xadd_cstr(&s,", last symbol was ");
			uostr_xadd_cstr(&s,last_symbol_name);
		}
		uostr_x0(&s);
		for (p=s.data;*p;p++) {
			if (*p=='\n') *p=' ';
		}
		die_line(0,str,s.data);
	}
	_exit(1); /* not reached */
}

#if 0
static int
parse_ip_list (char *val, int *count, int *size, ip_list_t ** ar)
{
	struct in_addr ip;
	struct in_addr mask;

	while (val) {
		char *q;
		char *p;
		int allow=1;
		if (*val=='!') {
			val++;
			allow=0;
		}
		if (*count >= *size) {
			ip_list_t *ar2;
			*size += 10;
			ar2 = malloc (*size * sizeof (ip_list_t));
			if (!ar2) {
				int_syslog(LOG_NOTICE, "out of memory");
				return 1;
			}
			if ((*size)-10)
				memcpy (ar2, *ar, sizeof (ip_list_t) * *size);
			*ar = ar2;
		}
		while (isspace ((unsigned char) *val))
			val++;
		if (!*val)
			break;
		p = strpbrk (val, ", ");
		if (p)
			*p++ = 0;
		for (q = val; q && *q; q++) {
			if (*q != '.' && !isdigit ((unsigned char)*q))
				break;
		}
		if (q && *q) {
			if (*q != '/')
				int_syslog(LOG_INFO, "unwanted delimiter %c", *q);
			*q = 0;
		} else
			q=NULL;
		if (0 == inet_aton (val, &ip)) {
			int_syslog(LOG_NOTICE, "cannot understand IP address %s", val);
			return 1;
		}
		if (q) {
			q++;
			if (0 == inet_aton (q, &mask)) {
				int_syslog(LOG_NOTICE, "cannot understand ip address %s", q);
				return 1;
			}
			(*ar)[*count].allow=allow;
			(*ar)[*count].addr = ip;
			(*ar)[(*count)++].mask = mask;
		} else {
			(*ar)[*count].allow=allow;
			(*ar)[*count].addr = ip;
			inet_aton ("255.255.255.255", &mask);
			(*ar)[(*count)++].mask = mask;
		}
		val = p;
	}
	return 0;
}
#endif
