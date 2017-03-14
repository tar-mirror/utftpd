/* wait_check.c: wait, check and log */

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

#include <netinet/in.h>
#include <syslog.h>
#include <sys/wait.h>
#include <unistd.h>

#include "str_ulong.h"
#include "utftpd.h" /* get my prototype */

int
wait_check_and_log(pid_t pid)
{
	int status;
	pid=waitpid(pid,&status,0);
	if (WIFEXITED(status)) {
		char nb[STR_ULONG];
		int ec=WEXITSTATUS(status);
		if (ec!=0) {
			str_ulong(nb,ec);
			syslog(LOG_ERR,"child terminated with code %s",nb);
			return -1;
		}
	} else if (WIFSIGNALED(status)) {
		char nb[STR_ULONG];
		int sig=WTERMSIG(status);
		str_ulong(nb,sig);
		syslog(LOG_ERR,"child terminated by signal %s",nb);
		return -1;
	}
	return 0;
}
