/*
 * Copyright (C) 1998-1999 Uwe Ohse
 * 
 * This source is public domain: use it as you please.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * Contact: uwe@ohse.de, Uwe Ohse @ DU3 (mausnet)
 */
#include <sys/types.h>
#include <fcntl.h>
#include "nonblock.h"

/* love compatability */
#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

int
nonblock(int fd, int on)
{
	int st;
	st=fcntl(fd,F_GETFL);
	if (st==-1)
		return -1;
	if (on) {
		if (st & O_NONBLOCK)
			return 0;
		return fcntl(fd,F_SETFL, st | O_NONBLOCK);
	}
	if (st & O_NONBLOCK)
		return fcntl(fd,F_SETFL,st & ~(O_NONBLOCK));
	return 0;
}
