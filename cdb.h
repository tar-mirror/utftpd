#ifndef CDB_H
#define CDB_H

/* #include "uint32.h" */
#include "typesize.h"
#define uint32 uo_uint32

extern uint32 cdb_hash P__((const unsigned char *, unsigned int len));
extern uint32 cdb_unpack P__((unsigned char *));

extern int cdb_bread P__((int fd, unsigned char *buf,int len));
extern int cdb_seek P__((int fd, const unsigned char *key,unsigned int len, uint32 *dlen));

#endif
