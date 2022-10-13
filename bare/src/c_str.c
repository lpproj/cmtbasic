#include "stdlib.h"
#include "string.h"

char * bare_strcpy(char *dest, const char *s)
{
    char *d = dest;
    while((*d++ = *s++) != '\0')
        ;
    return dest;
}

unsigned bare_strlen(const char *s)
{
    unsigned n = 0;
    while(s[n]) ++n;
    return n;
}

int bare_strcmp(const char *s1, const char *s2)
{
    int rc;

    while(1) {
        unsigned char c1 = *s1;
        unsigned char c2 = *s2;
        rc = (int)c1 - (int)c2;
        if (rc != 0 || c1 == 0) break;
        ++s1;
        ++s2;
    }
    return rc;
}

int bare_strcasecmp(const char *s1, const char *s2)
{
    int rc;

    while(1) {
        unsigned char c1 = *s1;
        unsigned char c2 = *s2;
        if (c1 >= 'a' && c1 <= 'z') c1 -= 'a' - 'A';
        if (c2 >= 'a' && c2 <= 'z') c2 -= 'a' - 'A';
        rc = (int)c1 - (int)c2;
        if (rc != 0 || c1 == 0) break;
        ++s1;
        ++s2;
    }
    return rc;
}

char * bare_strchr(const char *s, int c)
{
    while(1) {
        char sc = *s;
        if (sc == (char)c) return (char *)s;
        if (sc == '\0') break;
        ++s;
    }

    return NULL;
}

void bare_memset(void *m, int c, unsigned length)
{
    char *s = m;
    while(length--) *s++ = (char)c;
}

void bare_memcpy(void *dest, const void *src, unsigned length)
{
    register unsigned i;
    for(i=0; i<length; ++i) {
        *((char *)dest + i) = *((char *)src + i);
    }
}

void bare_fmemcpy(void BARE_FAR *dest, const void BARE_FAR *src, unsigned length)
{
    unsigned char BARE_FAR *d = dest;
    const unsigned char BARE_FAR *s = src;
    while(length > 0) {
        *d++ = *s++;
        --length;
    }
}

int bare_memcmp(const void *m1, const void *m2, unsigned length)
{
    const unsigned char *s1 = m1;
    const unsigned char *s2 = m2;
    int r = 0;

    while(length-- > 0) {
        r = (int)(*s1++) - (int)(*s2++);
        if (r != 0) break;
    }
    return r;
}


