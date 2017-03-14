#include "cdb.h"

/* placed in the public domain by dbj@pobox.com,
 * ANSIfied by uwe@ohse.de
 */

uint32 
cdb_unpack(unsigned char *buf)
{
  uint32 num;
  num = buf[3]; num <<= 8;
  num += buf[2]; num <<= 8;
  num += buf[1]; num <<= 8;
  num += buf[0];
  return num;
}
