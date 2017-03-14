#ifndef UTFTPD_MAKE_H
#define UTFTPD_MAKE_H

struct varlist {
	char *name;
	char *value;
	int lineno;
	struct varlist *next;
};
struct myhost {
	char *name;
	struct varlist *vars;
	struct myhost *next;
};
extern struct myhost *host_anker;


extern int parser_lineno;
extern int my_parser(const char *what);
extern void die_line(int,const char *,const char *);

#endif

