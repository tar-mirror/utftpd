/* this is placed into the public domain */

#include <stdio.h>
#include "cdb.h"

/* basically this is cdb_dump by djb, with a few changes to print the 
 * utftpd configuration file format
 */

static void format(void)
{
  fputs("cdbdump: fatal: bad database format\n",stderr);
  exit(1);
}

static void readerror(void)
{
  if (ferror(stdin)) { perror("cdbdump: fatal: unable to read"); exit(111); }
  format();
}

int main(void)
{
  uint32 eod;
  uint32 pos;
  uint32 klen;
  uint32 dlen;
  unsigned char buf[8];
  int i;
  int c;

  if (fread(buf,1,4,stdin) < 4) readerror();
  eod = cdb_unpack(buf);
  for (i = 4;i < 2048;++i) if (getchar() == EOF) readerror();

  pos = 2048;
  while (pos < eod) {
    int eq_found=0;
    if (eod - pos < 8) format();
    pos += 8;
    if (fread(buf,1,8,stdin) < 8) readerror();
    klen = cdb_unpack(buf);
    dlen = cdb_unpack(buf + 4);
    if (eod - pos < klen) format();
    pos += klen;
    if (eod - pos < dlen) format();
    pos += dlen;
    fputs("client ",stdout);
    while (klen) {
      --klen;
      c = getchar();
      if (c == EOF) readerror();
      putchar(c);
    }
    fputs(" {\n  ",stdout);
    while (dlen) {
      --dlen;
      c = getchar();
      if (c == EOF) readerror();
	  if (c=='\0') {
    	fputs("\";\n  ",stdout);
		eq_found=0;
	  } else if (c=='=' && !eq_found) {
        putchar(c);
        putchar('"');
		eq_found=1;
	  } else
        putchar(c);
    }
   	fputs("}\n",stdout);
  }
  return 0;
}
