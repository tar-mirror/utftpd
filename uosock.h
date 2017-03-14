#ifndef UOSOCK_H
#define UOSOCK_H

#include <sys/socket.h>

#ifndef P__
#define P__(x) x
#endif

int uosock_connect P__((struct sockaddr_in *target, int timeout));
int parse_ip_port(const char *input, struct in_addr *ina, unsigned short *port, const char *proto);


#endif
