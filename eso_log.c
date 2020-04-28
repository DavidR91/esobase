#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h> 
#include "eso_log.h"

void log_printf(const char* format, ...) {
    va_list argptr;
    va_start(argptr, format);
   vfprintf(stdout, format, argptr);
    va_end(argptr);
}

void log_verbose(const char* format, ...) {
    va_list argptr;
    va_start(argptr, format);

    #ifdef ESO_VERBOSE_DEBUG
     fprintf(stdout, "[VRB] ");
     vfprintf(stdout, format, argptr);
     #endif
    va_end(argptr);
}

void log_ingestion(char character) {
    //printf("%c", character);
}

void log_ingestion_marker(const char* format, ...) {
    va_list argptr;
    va_start(argptr, format);
    // vfprintf(stdout, format, argptr);
    va_end(argptr);
}