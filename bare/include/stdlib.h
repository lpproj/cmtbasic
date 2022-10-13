#ifndef BARE_STDLIB_H
#define BARE_STDLIB_H

# ifndef BARE_DEFS_H
#  include "_defs.h"
# endif

# define _INC_STDLIB
# define __STDLIB_H
# define _STDLIB_H_INCLUDED

typedef unsigned size_t;
# define _SIZE_T
# define _SIZE_T_DEFINED
# define _SIZE_T_DEFINED_
# define __SIZE_T_DEFINED

#define NULL ((void *)0)

# define malloc bare_malloc
# define calloc bare_calloc
# define realloc bare_realloc
# define free bare_free
# define exit bare_exit
# define abort bare_abort

#endif
