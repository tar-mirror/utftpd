/* uogetopt.c: somewhat GNU compatible reimplementation of getopt(_long) */

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

/*
 * main differences:
 * + new feature: --version 
 * + new feature: --help  (standard help text, one line per option)
 * + new feature: --help OPTION (maybe somewhat more)
 * + new feature: --longhelp (all --help OPTION)
 * - no "return in order"
 * o really no support for i18n.
 * o small code, about 65% of GNU C library stuff.
 * o it doesn't do black magic, like that GNU stuff.
 *
 * Note that the following statement from the GNU getopt.c was the cause
 * for reinventing the wheel:
 *    Bash 2.0 puts a special variable in the environment for each
 *    command it runs, specifying which ARGV elements are the results of
 *    file name wildcard expansion and therefore should not be
 *    considered as options.
 * i decided that this wheel is to broken to be reused. Think of that
 * "-i" trick. As time passed by i calmed down, but now uogetopt is
 * better than GNU getopt ...
 */

#include "config.h"
#include <string.h>
#include <errno.h>
#include <unistd.h> /* write */
#include <stdlib.h> /* getenv */
#include "uogetopt.h"
#include "str2num.h"

static int paranoid=0;


#define L longname
#define S shortname
#define LLEN 80
#define OFFSET 29 /* keep me < LLEN */

void 
uogetopt_out(int iserr,const char *s)
{
	int fd=1;
	if (iserr)
		fd=2;
	write(fd,s,strlen(s));
}

static void 
uogetopt_outn(void (*out)(int iserr,const char *),
	int iserr,const char *s,const char *t,const char *u,const char *v)
{ out(iserr,s); if (t) out(iserr,t); if (u) out(iserr,u); if (v) out(iserr,v); }
#define O2(E,s,t) do { uogetopt_outn(out,(E),(s),(t),0,0); } while (0)
#define O3(E,s,t,u) do { uogetopt_outn(out,(E),(s),(t),(u),0); } while (0)
#define O4(E,s,t,u,v) do { uogetopt_outn(out,(E),(s),(t),(u),(v)); } while (0)

static void
uogetopt_hlong(void (*out)(int iserr,const char *),const char *argv0, const char *s,uogetopt_t *o)
{
	int len;
	long l;
	len=str2long(s,&l,0);
	if (len<=0) { O4(1,argv0,": ",s," is not an signed long\n");_exit(2);} 
	*(long *)o->var=l;
}

static void
uogetopt_hulong(void (*out)(int iserr,const char *),const char *argv0, const char *s,uogetopt_t *o)
{
	int len;
	unsigned long ul;
	len=str2ulong(s,&ul,0);
	if (len<=0) { O4(1,argv0,": ",s," is not an unsigned long\n");_exit(2);} 
	*(unsigned long *)o->var=ul;
}

static void
uogetopt_describe(void (*out)(int iserr,const char *),uogetopt_t *o, int full)
{
/* 
  -s, --size                 print size of each file, in blocks
  -S                         sort by file size
      --sort=WORD            ctime -c, extension -X, none -U, size -S,
123456789012345678901234567890
I don't really know for what the spaces at the start at the line are
for, but i suppose someone will need them ...
*/
	int l;
	const char *p;
	char *q;
	char buf[LLEN+1];
	char minbuf[2];
	out(0,"  ");
	if (o->S) { buf[0]='-'; buf[1]=o->S; buf[2]=0; out(0,buf); } /* -X */
	else out(0,"  ");

	if (o->S && o->L) out(0,", --");
	else if (o->L)    out(0,"  --");
	else              out(0,"    ");

	l=8;

	if (o->L) {l+=strlen(o->L);out(0,o->L);}

	if (o->argtype!=UOGO_FLAG && o->argtype!=UOGO_FLAGOR) {
		if (o->L) out(0,"=");
		else      out(0," ");
		l++;
		if (o->paraname) {out(0,o->paraname);l+=strlen(o->paraname);}
		else if (o->argtype==UOGO_STRING) {out(0,"STRING");l+=6;}
		else {out(0,"NUMBER");l+=6;}
	}
	/* fill up with spaces (at least one) */
	q=buf; do { *q++=' '; l++; } while (l<OFFSET); *q=0; 
	out(0,buf);
	if (paranoid && l>29) out(0,"\n###time for shorter options?\n");

	/* 1. line of help */
	if (!o->help) {out(0,"???\n"); return; }
	p=strchr(o->help,'\n');
	if (!p || !p[1]) { 
		out(0,o->help); 
		if (!p) out(0,"\n");
		if (paranoid && strlen(o->help)>=51) out(0,"###the last help text is too long for 80 characters screens.\n");
		return;
	}
	minbuf[1]=0;
	p=o->help;
	do {minbuf[0]=*p;out(0,minbuf); } while (*p++!='\n');
	if (paranoid && p-o->help>=51) out(0,"###the last help text is too long for 80 characters screens.\n");

	if (!full) return;
	memset(buf,' ',OFFSET); buf[OFFSET]=0;
	do { 
		const char *r;
		out(0,buf);
		r=p;
		do {minbuf[0]=*p++;out(0,minbuf);} while (*p && p[-1]!='\n');
		if (paranoid && p-r>=51) out(0,"###the last help text is too long for 80 characters screens.\n");
		if (!*p) out(0,"\n");
	} while (*p && p[1]);
}

static void	
uogetopt_printver(void (*out)(int iserr,const char *),
	const char *prog, const char *package, const char *version)
{
		out(0,prog);
		if (package) { O3(0, " (",package,")"); }
		O3(0," ",version,"\n");
}

static int dummy;
static uogetopt_t hack_longhelp=
    {0,"longhelp",   UOGO_FLAG,&dummy,0,
	     "Show longer help texts for all or one variable\n"
       /* 12345678901234567890123456789012345678901234567890" */
		 "--longhelp with an argument shows the long help\n"
		 "for this option, without arguments it shows the\n"
		 "long description of all options.","[OPTION-NAME]"
	};
static uogetopt_t hack_help=
    {0,"help",   UOGO_FLAG,&dummy,0,
       /* 12345678901234567890123456789012345678901234567890" */
		 "Show a list of options or the long help on one.\n"
		 "--help with an argument shows the long helptext\n"
		 "of that option, without an argument it will list\n"
		 "all options.","[OPTION-NAME]"
	};
static uogetopt_t hack_version=
    {0,"version",   UOGO_FLAG,&dummy,0,
	     "Show version information.",0
	};

static char minus_help[]="--help";
static char minus_version[]="--version";
void 
uogetopt(const char *prog, const char *package, const char *version, 
	int *argc, char **argv, void (*out) P__((int iserr,const char *)), 
	const char *head, uogetopt_t *opts, const char *tail)
{
	int i;
	int posix;
	int newargc;
	int ocount;
	int is_longhelp;
	int h_used=0;
	int v_used=0;
	int ques_used=0;
	int V_used=0;
	uogetopt_t *copyright=0;
	if (!argv[1]) return;
	if (0==strcmp(argv[1],"--uogetopt-paranoid")) 
		{ union { char *c; const char *cc; }d; d.cc="--longhelp"; argv[1]=d.c; paranoid=1; }
	for (i=0;opts[i].S || opts[i].L;i++) {
		if (!opts[i].var && !(opts[i].argtype&UOGO_HIDDEN)) {
			/* no use to waste code for detailed error messages, this only
			 * happens during development.
			 */
			out(1,"NULL variable address in uogetopt()\n");
			_exit(1);
		}
		if (opts[i].L && 0==strcasecmp(opts[i].L,"copyright"))
			copyright=&opts[i];
		switch (opts[i].S) {
		case 'v': v_used=1; break;
		case 'h': h_used=1; break;
		case 'V': V_used=1; break;
		case '?': ques_used=1; break;
		}
	}
	/* try to map -?, -h to --help, -V, -v the --version */
	if (!argv[2] && argv[1][0]=='-') {
		if (argv[1][1]=='h' && !h_used) argv[1]=minus_help;
		if (argv[1][1]=='?' && !ques_used) argv[1]=minus_help;
		if (argv[1][1]=='v' && !v_used) argv[1]=minus_version;
		if (argv[1][1]=='V' && !V_used) argv[1]=minus_version;
	}
	ocount=i;
	if (argv[1] && 0==strcmp(argv[1],"--version")) {
		uogetopt_printver(out,prog,package,version);
		if (copyright && copyright->help) {
			out(0,"\n");
			out(0,copyright->help);
		}
		_exit(0);
	}
	is_longhelp=(0==strcmp(argv[1],"--longhelp"));
	if (argv[1] && (is_longhelp || 0==strcmp(argv[1],"--help"))) {
		if (argv[2]) { 
			uogetopt_t *u=0;
			if (argv[2][0]=='-') argv[2]++;
			if (argv[2][0]=='-' && argv[2][1]) argv[2]++;
			for (i=0;i<ocount;i++) {
				if (opts[i].L && 0==strcmp(opts[i].L,argv[2])) break;
				if (opts[i].S && !argv[2][1] && argv[2][0]==opts[i].S) break;
			}
			if (i!=ocount) u=&opts[i]; 
			else if (0==strcmp(argv[2],"--longhelp") || 0==strcmp(argv[2],"longhelp")) u=&hack_longhelp;
			else if (0==strcmp(argv[2],"--help") || 0==strcmp(argv[2],"help")) u=&hack_help;
			else if (0==strcmp(argv[2],"--version") || 0==strcmp(argv[2],"version")) u=&hack_version;
			if (!u) { (out)(0,"no such option\n"); _exit(1); }
			if (!u->help) { (out)(0,"no help available\n"); _exit(1); }
			uogetopt_describe(out,u,1); /* long help every time */
			_exit(0);
		}
		/* uogetopt_printver(out,prog,package,version); it's against the GNU standards */
		if (head) out(0,head); 
		else { O3(0,"usage: ",prog," [options]\n");}
		out(0,"\n");
		for (i=0;i<ocount;i++) {
			if (!(opts[i].argtype & UOGO_HIDDEN))
				uogetopt_describe(out,&opts[i],is_longhelp);
		}
		uogetopt_describe(out,&hack_version,is_longhelp);
		uogetopt_describe(out,&hack_help,is_longhelp);
		uogetopt_describe(out,&hack_longhelp,is_longhelp);
		if (tail) {out(0,tail); out(0,"\n");}
		_exit(0);
	}

	posix=!!getenv("POSIXLY_CORRECT");
	newargc=1;
	for (i=1;i<*argc;i++) {
		if (*argv[i]!='-') {
			if (posix) { 
		  copyrest:
				while (argv[i]) argv[newargc++]=argv[i++];
				argv[newargc]=0;
				*argc=newargc;
				return;
			}
			argv[newargc++]=argv[i];
			continue;
		}
		if (argv[i][1]=='-') { 
			int j;
			int ioff=1;
			char *para;
			uogetopt_t *o;
			int at;
			para=strchr(argv[i],'=');
			if (para) { *para++=0;ioff=0;}
			else {para=argv[i+1];}
			if (!argv[i][2]) { i++; goto copyrest; }
			o=opts;
			for (j=0;j<ocount;o++,j++)
				if (o->L && 0==strcmp(o->L,argv[i]+2)) break;
			at=o->argtype & (~(UOGO_HIDDEN));
			if (j==ocount || at==UOGO_NOOPT) {O4(1,argv[0],": illegal option -- ",argv[i],"\n");_exit(2);}
			if ((at==UOGO_FLAG || at==UOGO_FLAGOR || at==UOGO_PRINT) && ioff==0) /* --x=y */
				{ O4(1,argv[0],": option doesn't allow an argument -- ",argv[i],"\n");_exit(2);}
			if (at==UOGO_FLAG) { *((int*)o->var)=o->val; continue;}
			if (at==UOGO_FLAGOR) { *((int*)o->var)|=o->val; continue;}
			if (at==UOGO_PRINT) { out(0,o->help); if (o->help[strlen(o->help)-1] != '\n') out(0,"\n"); _exit(0); }
			if (!para) {O4(1,argv[0],": option requires an argument -- ",argv[i]+2,"\n");_exit(2);}
			if (at==UOGO_STRING) { *((char **)o->var)=para; i+=ioff; continue;}
			if (at==UOGO_ULONG) { uogetopt_hulong(out,argv[0],para,o); i+=ioff; continue; }
			if (at==UOGO_LONG) { uogetopt_hlong(out,argv[0],para,o); i+=ioff; continue; }
			if (at==UOGO_CALLBACK) { 
				union disqualify {
					void *v;
#if defined(PROTOTYPES) || defined(__cplusplus)
					void (*fn)(uogetopt_t *,const char *);
#else
					void (*fn)();
#endif
				} dq;
				dq.v=o->var;
				(*dq.fn)(o,para);
				i+=ioff; continue;
			}
			O4(1,argv[0],": internal problem, unknown option type for `",argv[i],"'\n");
			_exit(2);
		} else { 
			int j;
			for (j=1;argv[i][j];j++) { /* all chars */
				char c=argv[i][j];
				int k;
				int nexti;
				char *p;
				char optstr[2];
				uogetopt_t *o;
				int at;
				optstr[0]=c;optstr[1]=0;
				o=opts;
				for (k=0;k<ocount;k++,o++) {
					if (o->S && o->S==c) 
						break;
				}
				at=o->argtype & (~(UOGO_HIDDEN));
				if (k==ocount || at==UOGO_NOOPT) {
					O4(1,argv[0],": illegal option -- ",optstr,"\n");
					_exit(2);
				}
				if (at==UOGO_FLAG) { *((int*)o->var)=o->val; continue;}
				if (at==UOGO_FLAGOR) { *((int*)o->var)|=o->val; continue;}
				if (at==UOGO_PRINT) { out(0,o->help); if (o->help[strlen(o->help)-1] != '\n') out(0,"\n"); _exit(0); }
				/* options with arguments, first get arg */
				nexti=i; p=argv[i]+j+1; 
				if (!*p) { p=argv[i+1];
					if (!p) {O4(1,argv[0],": option requires an argument -- ",optstr,"\n");_exit(2);}
					nexti=i+1; }
				if (at==UOGO_STRING) { *((char **)o->var)=p; i=nexti; break; }
				if (at==UOGO_ULONG) { uogetopt_hulong(out,argv[0],p,o); i=nexti; break; }
				if (at==UOGO_LONG) { uogetopt_hlong(out,argv[0],p,o); i=nexti; break; }
				if (at==UOGO_CALLBACK) { 
					union disqualify {
						void *v;
#if defined(PROTOTYPES) || defined(__cplusplus)
						void (*fn)(uogetopt_t *,const char *);
#else
						void (*fn)();
#endif
					} dq;
					dq.v=o->var;
					(*dq.fn)(o,p);
					i=nexti; break;
				}
				O4(1,argv[0],": internal problem, unknown option type for `",optstr,"'\n");
				_exit(2);
			}
		}
	}
	*argc=newargc;
	argv[newargc]=0;
}

#ifdef TEST

int flag=0;
char *string;
unsigned long ul=17;
signed long lo=18;
uogetopt_t myopts[]={
	{'t',"test",UOGO_FLAG,&flag,1,"test option\n"},
	{'l',"longtest",UOGO_FLAGOR,&flag,2,"test option with some characters\nand more than one line, of which one is really long, of course, just to test some things, like crashs or not crash, life or 42\n"},
	{'L',0,UOGO_FLAG,&flag,2,"test option with some characters\nand more than two lines\nwith not much information\nbut a missing newline at the end: 43"},
	{'M',0,UOGO_FLAG,&flag,2,"test option with some characters\nand two lines: 44\n"},
	{'N',0,UOGO_FLAG,&flag,2,"test option with some characters\nand two lines, no nl at end: 45"},
	{'u',"ulong",UOGO_ULONG,&ul,2,"set a ulong variable\n","Ulong"},
	{'n',"nulong",UOGO_LONG,&lo,2,"set a long variable\n","long"},
	{'s',"string",UOGO_STRING,&string,2,"set a string variable\n","String"},
	{0,0}
};
int 
main(int argc, char **argv)
{

	printf("old argc=%d\n",argc);
	uogetopt("uogetopt","misc","0.1",
		&argc, argv,
		writeerr,
		"head\n",myopts,"tail\n");
	printf("flag=%d\n",flag);
	printf("string=%s\n",string ? string : "NULL");
	printf("ulong=%lu\n",ul);
	printf("long=%ld\n",lo);
	printf("new argc=%d\n",argc);
	{ int i; for (i=0;argv[i];i++) printf("argv%d: %s\n",i,argv[i]);}
	exit(0);
}
#endif
