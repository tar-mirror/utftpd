#include "cdbmake.h"

/* placed in the public domain by dbj@pobox.com,
 * ANSIfied by uwe@ohse.de
 */

uint32 
cdbmake_hashadd(uint32 h,unsigned int c)
{
  h += (h << 5);
  h ^= (uint32) (unsigned char) c;
  return h;
}
