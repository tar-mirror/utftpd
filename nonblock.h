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

/* make fd nonblocking (on) or blocking (off)
 * return -1 on error, 0 on success 
 */
#ifndef P__
#define P__(x) x
#endif
int nonblock P__ ((int fd, int on));
