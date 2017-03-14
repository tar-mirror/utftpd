/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

%{
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

%}
%union {
	int integer;
	char *string;
}
%token '{' '}' ';' ':' SYM_CLASS STRING SYM_OVERRIDE SYM_PLUSGLEICH SYM_CLIENT
%type <string> STRING
%start wholefile

%%
wholefile: classdef wholefile
		| hostdef wholefile
		| ';' wholefile 
		| oldstyle wholefile
		| 
		;
oldstyle: STRING { start_hostdef($1); in_oldstyle=1;} ':' oldstylevars '\n'
		;
oldstylevars: oldstylevar 
		| oldstylevars oldstylevar
		;
oldstylevar: STRING '(' STRING ')' { add_to_host_varlist(SYM_OVERRIDE,$1,$3); }
		| STRING '(' ')' { add_to_host_varlist(SYM_OVERRIDE,$1,""); }
		;

hostdef:  SYM_CLIENT STRING {start_hostdef($2);} maybehostinherit hostdefmain
	;
maybehostinherit: 
	| ':' hostdefinherit
	;
hostdefmain: '{' hostdefvars '}'
		| ';'
		;
hostdefvars: hostdefvar 
		| hostdefvar ';' hostdefvars
		|
		;
hostdefvar:
		  STRING '=' STRING  { add_to_host_varlist('=',$1,$3); }
		| STRING SYM_PLUSGLEICH STRING  { add_to_host_varlist(SYM_PLUSGLEICH,$1,$3); }
		| SYM_OVERRIDE STRING '=' STRING  { add_to_host_varlist(SYM_OVERRIDE,$2,$4); }
		;
hostdefinherit: STRING  {do_host_inherit($1);}
		| STRING {do_host_inherit($1); } ',' hostdefinherit
		;

classdef: SYM_CLASS STRING {start_classdef($2)} maybeclassinherit classdefmain
		;
classdefmain: '{' classdefargs '}'
		| ';'
		;
maybeclassinherit:
		|  ':' inheritdef
		;
inheritdef: STRING  {do_class_inherit($1);}
		| STRING {do_class_inherit($1); } ',' inheritdef 
		;
classdefargs: classdefarg 
		| classdefarg ';' classdefargs
		|
		;
classdefarg: 
		  STRING '=' STRING { add_to_class_varlist('=',$1,$3); } 
		| STRING SYM_PLUSGLEICH STRING  { add_to_class_varlist(SYM_PLUSGLEICH,$1,$3); }
		| SYM_OVERRIDE STRING '=' STRING  { add_to_class_varlist(SYM_OVERRIDE,$2,$4); } 
		;
%%
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
