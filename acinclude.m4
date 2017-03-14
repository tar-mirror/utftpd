AC_DEFUN(UO_HEADER_SYS_SELECT,
[AC_CACHE_CHECK([whether sys/time.h and sys/select.h may both be included],
  lrzsz_cv_header_sys_select,
  [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>],
[struct tm *tp;], lrzsz_cv_header_sys_select=yes, lrzsz_cv_header_sys_select=no)])
if test $lrzsz_cv_header_sys_select = no; then
AC_DEFINE(SYS_TIME_WITHOUT_SYS_SELECT)
fi
])

dfn against BeOS brokenness
AC_DEFUN(UO_HEADER_SYS_UIO,
[AC_CACHE_CHECK([whether sys/uio.h and unistd.h may both be included],
  lrzsz_cv_header_sys_uio,
  [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>],
[struct tm *tp;], lrzsz_cv_header_sys_uio=yes, lrzsz_cv_header_sys_uio=no)])
if test $lrzsz_cv_header_sys_uio = no; then
AC_DEFINE(UNISTD_WITHOUT_SYS_UIO)
fi
])

AC_DEFUN(UO_TYPE_SOCKLEN_T,[
AC_CACHE_CHECK([for socklen_t],uo_cv_type_socklen_t,
	[AC_TRY_COMPILE([
#include "confdefs.h"
#include <sys/types.h>
#include <sys/socket.h>
#if STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif
	],[socklen_t x=0;return x;]
	,uo_cv_type_socklen_t=yes,uo_cv_type_socklen_t=no)]
)
if test $uo_cv_type_socklen_t = no; then
cat >> confdefs.h <<\EOF
#define socklen_t int
EOF
fi
]) dnl DEFUN

