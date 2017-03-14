#ifndef CONFIG_H_ALREADY_INCLUDED
#define CONFIG_H_ALREADY_INCLUDED
@TOP@
#undef SYS_TIME_WITHOUT_SYS_SELECT

#undef UNISTD_WITHOUT_SYS_UIO

/* Define to the name of the distribution.  */
#undef PACKAGE

/* The concatenation of the strings PACKAGE, "-", and VERSION.  */
#undef PACKAGE_VERSION

/* Define to the version of the distribution.  */
#undef VERSION

#undef HAVE_LIBEFENCE

/* note: i do _not_ understand why autoconf forces me to add them
 * here. AC_CHECK_SIZEOF should do that.
 * Some day someone needs to come up with something better than
 * autoconf and automake.
 */
#undef SIZEOF_SHORT
#undef SIZEOF_INT
#undef SIZEOF_LONG
#undef SIZEOF_LONG_LONG
#undef SIZEOF_UNSIGNED_SHORT
#undef SIZEOF_UNSIGNED_INT
#undef SIZEOF_UNSIGNED_LONG
#undef SIZEOF_UNSIGNED_LONG_LONG

/* define to the type to replace ssize_t */
#undef ssize_t
/* define to the type to replace socklen_t */
#undef socklen_t

/* Define to 1 if ANSI function prototypes are usable.  */
#undef PROTOTYPES


@BOTTOM@
/* include sys/types.h to get a definition of P__ if it's 
 * there (BSDI at least). */
#include <sys/types.h>
#ifndef P__
#if defined (__GNUC__) || (defined (__STDC__) && __STDC__)
#define P__(args) args
#else
#define P__(args) ()
#endif  /* GCC.  */
#endif  /* Not P__.  */

#endif /* CONFIG_H_ALREADY_INCLUDED */

