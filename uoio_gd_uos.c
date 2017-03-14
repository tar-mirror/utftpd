#include <errno.h>
#include "uostr.h"
#include "uoio.h"

ssize_t
uoio_getdelim_uostr (uoio_t * u, uostr_t *s, int delim)
{
	char *p;
	void *q;
	ssize_t r=uoio_getdelim_zc(u,&p,delim);
	if (r<0)
		return r;
	if (r==0) {
		uostr_fcut(s);
		return r;
	}
	q=uostr_dup_mem(s,p,r);
	if (!q) {
		errno=ENOMEM;
		return -1;
	}
	return r;
}

