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
 */
#include "config.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "str2num.h"
#include "uosock.h"

static int
lookup_host(const char *host, struct in_addr *ina)
{
	int v;
	struct hostent *he;
	v = inet_aton (host, ina);
	if (0 == v) {
		/* XXX use real DNS! */
		he = gethostbyname (host);
		if (!he) return -1;
		memcpy (&ina->s_addr, he->h_addr, sizeof (ina->s_addr));
	}
	return 0;
}

/* 
 * a function to parse HOST:PORT specifications, which can deal with:
 *   HOST:PORT
 *   :PORT
 *   HOST:
 *   HOST
 *   :
 * HOST may be an IP address or a host name (which will be resolved through
 * gethostbyname). 
 * PORT may be a number or a service name (only in that case to "proto" parameter
 * will be used).
 *
 * Note that you should initialize "ina" and "port" to useful default values before
 * calling this function.
 */
int 
parse_ip_port(const char *input, struct in_addr *ina, unsigned short *port, const char *proto)
{
	char *dp;
	int r;
	long lo;
	dp=strchr(input,':');
	if (!dp) /* HOST */
		return lookup_host(input,ina);
	if (dp==input) /* :PORT */
		ina->s_addr=INADDR_ANY;
	else { /* HOST: or HOST:port */
		char *cop;
		cop=(char *)malloc(dp-input+1);
		if (!cop) {errno=ENOMEM; return -1; }
		memcpy(cop,input,dp-input);
		cop[dp-input]=0;
		r=lookup_host(cop,ina);
		free(cop);
		if (r==-1)
			return r;
	}
	dp++;
	if (!*dp)
		return 0;
	if (str2long(dp,&lo,10) > 0) {
		*port=htons(lo);
	} else {
		struct servent *se;
		se=getservbyname(dp,proto);
		if (se) 
			*port=se->s_port;
		else
			return -1;
	}
	return 0;
}
