#include "uostr.h"

uostr_t *
uostr_add_char(uostr_t *u,const char c)
{
	return uostr_add_mem(u,&c,1);
}
uostr_t *
uostr_xadd_char(uostr_t *u, const char v)
{ 
	uostr_t *r=uostr_add_mem(u,&v,1);
	if (!r) uostr_xallocerr("uostr_xadd_char");
	return r;
}

uostr_t *
uostr_dup_char(uostr_t *u,const char c)
{
	return uostr_dup_mem(u,&c,1);
}
uostr_t *
uostr_xdup_char (uostr_t * u, const char v)
{
	uostr_t *r = uostr_dup_mem (u, &v, 1);
	if (!r) uostr_xallocerr ("uostr_xdup_char");
	return r;
}
