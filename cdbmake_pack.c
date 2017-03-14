#include "cdbmake.h"

/* placed in the public domain by dbj@pobox.com,
 * ANSIfied by uwe@ohse.de
 */

void 
cdbmake_pack(unsigned char *buf,uint32 num)
{
  *buf++ = num; num >>= 8;
  *buf++ = num; num >>= 8;
  *buf++ = num; num >>= 8;
  *buf = num;
}
