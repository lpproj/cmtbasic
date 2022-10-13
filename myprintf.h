#include "xprintf.h"

#undef printf
#define printf xprintf
#undef sprintf
#define sprintf xsprintf

