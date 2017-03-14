#ifndef CDBMAKE_H
#define CDBMAKE_H

/* placed in the public domain by dbj@pobox.com,
 * ANSIfied by uwe@ohse.de
 */

/* #include "uint32.h" */
#include "typesize.h"
#define uint32 uo_uint32

#define CDBMAKE_HPLIST 1000

struct cdbmake_hp { uint32 h; uint32 p; } ;

struct cdbmake_hplist {
  struct cdbmake_hp hp[CDBMAKE_HPLIST];
  struct cdbmake_hplist *next;
  int num;
} ;

struct cdbmake {
  unsigned char final[2048];
  uint32 count[256];
  uint32 start[256];
  struct cdbmake_hplist *head;
  struct cdbmake_hp *split; /* includes space for hash */
  struct cdbmake_hp *hash;
  uint32 numentries;
} ;

extern void cdbmake_pack P__((unsigned char *,uint32));
#define CDBMAKE_HASHSTART ((uint32) 5381)
extern uint32 cdbmake_hashadd P__((uint32,unsigned int));

extern void cdbmake_init P__((struct cdbmake *));
extern int cdbmake_add P__((struct cdbmake *cdbm,uint32 h,uint32 p, void *(*alloc)(size_t)));
extern int cdbmake_split P__((struct cdbmake *cdbm, void *(*alloc)(size_t)));
extern uint32 cdbmake_throw P__((struct cdbmake *cdbm,uint32 pos,int b));

#endif
