/*
 * Copyright (C) 1994-1999 Uwe Ohse
 * 
 * placed in the public domain.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * Contact: uwe@ohse.de
 */
#ifndef ATTRIBS_H
#define ATTRIBS_H

#ifdef __GNUC__

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 5)
/* note: this one relies on HAVE_ATTRIBUTE_SECTION, which you have to provide,
 * otherwise this macro do nothing ...
 */
# if !defined(PROFILING) && !defined(DEBUGGING) && defined(HAVE_ATTRIBUTE_SECTION)
#   define UO_ATTRIB_SECTION(x) __attribute__((section(#x)))
# endif
#endif

/* die beiden folgenden werden nur definiert, wenn die Funktionalität
 * verfügbar ist. "#define dies [leer]" macht hier keinen Sinn, weil 
 * der entsprechende Code nie ausgeführt würde. 
 * Also: Mit Vorsicht benutzen.
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#  define UO_ATTRIB_CONSTRUCTOR  __attribute__((__constructor__))
#  define UO_ATTRIB_DESTRUCTOR  __attribute__((__destructor__))
#endif

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#  define UO_ATTRIB_CONST  __attribute__((__const__))
#endif
	/* gcc.info sagt, noreturn wäre ab 2.5 verfügbar. HPUX-gcc 2.5.8
	 * kann es noch nicht - what's this?
	 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 5)
# define UO_ATTRIB_NORET  __attribute__((__noreturn__))
#endif
	/* 
	 * checked Formatstring (Argument Nr. "formatnr"). Der erste 
	 * Parameter des Formatstrings ist Argument Nr. "firstargnr".
	 * für vprintf und co, wo das nicht möglich ist, ist firstargnr
	 * auf 0 zu setzen -> nur Formatstring wird geprüft.
	 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 5)
# define UO_ATTRIB_PRINTF(formatnr,firstargnr)  \
	__attribute__((__format__ (printf,formatnr,firstargnr)))
#endif

#define UO_ATTRIB_INLINE __inline__

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 6)
#define UO_ATTRIB_UNUSED __attribute__((__unused__))
#endif

#endif /* GNU-C */

#ifndef UO_ATTRIB_CONST
#define UO_ATTRIB_CONST
#endif
#ifndef UO_ATTRIB_SECTION
#define UO_ATTRIB_SECTION(x)
#endif
#ifndef UO_ATTRIB_CONST
#define UO_ATTRIB_CONST
#endif
#ifndef UO_ATTRIB_NORET
#define UO_ATTRIB_NORET
#endif
#ifndef UO_ATTRIB_PRINTF
#define UO_ATTRIB_PRINTF(x,y)
#endif
#ifndef UO_ATTRIB_INLINE
#define UO_ATTRIB_INLINE
#endif
#ifndef UO_ATTRIB_UNUSED
#define UO_ATTRIB_UNUSED
#endif

#endif
