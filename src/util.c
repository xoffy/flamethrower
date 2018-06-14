
#include <stdio.h>
#include <stdarg.h>

#include "util.h"

void u_error(const char *fmt, ...) {
    va_list args;
    
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

