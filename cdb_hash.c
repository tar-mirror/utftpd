#include "cdb.h"

/* placed in the public domain by dbj@pobox.com,
 * ANSIfied by uwe@ohse.de
 */

uint32 
cdb_hash(const unsigned char *buf, unsigned int len)
{
  uint32 h;

  h = 5381;
  while (len) {
    --len;
    h += (h << 5);
    h ^= (uint32) *buf++;
  }
  return h;
}
