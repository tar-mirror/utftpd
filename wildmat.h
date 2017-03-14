#ifndef wildmat_h
#define wildmat_h

extern int wildmat P__((const char *pattern, const char *text));
extern int dir_wildmat P__((const char *pattern, const char *text)); /* trailing /anything allowed */
#endif
