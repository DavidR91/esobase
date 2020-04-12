#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h> 

void log_verbose(const char* format, ...) {
    va_list argptr;
    va_start(argptr, format);
    fprintf(stderr, "[VRB] ");
   	vfprintf(stderr, format, argptr);
    va_end(argptr);
}
