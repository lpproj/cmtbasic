#ifndef BARE_STRING_H
#define BARE_STRING_H

# ifndef BARE_DEFS_H
#  include "_defs.h"
# endif

# define _INC_STRING
# define __STRING_H
# define _STRING_H_INCLUDED

# ifdef __cplusplus
extern "C" {
# endif

char * bare_strcpy(char *, const char *);
unsigned bare_strlen(const char *);
char * bare_strchr(const char *, int);

int bare_strcmp(const char *s1, const char *s2);
int bare_strcasecmp(const char *s1, const char *s2);

void bare_memset(void *, int, unsigned);
# define bare_bzero(m,n) bare_memset(m,0,n)

void bare_memcpy(void *, const void *, unsigned);
void bare_fmemcpy(void BARE_FAR *, const void BARE_FAR *, unsigned);
int bare_memcmp(const void *, const void *, unsigned);

# if 0
#  define sprintf bare_sprintf
#  define snprintf bare_snprintf
# endif

# define strcpy bare_strcpy
# define strlen bare_strlen
# define strchr bare_strchr
# define strcmp bare_strcmp
# define strcasecmp bare_strcasecmp
# define memset bare_memset
# define bzero(m,n) bare_memset(m,0,n)
# define memcpy bare_memcpy
# define _fmemcpy bare_fmemcpy
# define memcmp bare_memcmp

# ifdef __cplusplus
}
# endif

#endif
