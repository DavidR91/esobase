#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h> 
#include <stdbool.h>

#include "eso_vm.h"
#include "eso_log.h"

// Return everything in code until the specified terminator is hit.
// The caller should resume from *resume in order to skip the eaten 
// text and the terminator. NULL returned if terminator not found before
// len
char* alloc_until(em_state* state, const char* code, int index, int len, char terminator, bool eat_whitespace, int* size_to_skip) {

    int total_skip = 0;
    int starting_index = index;

    while(eat_whitespace && isspace(code[starting_index])) {
        total_skip++;
        starting_index++;
    }

    int bytes_consumed = 0;

    for (int i = starting_index ; i < len; i++) {

        log_ingestion(code[i]);
        bytes_consumed++;
        total_skip++;
        if (code[i] == terminator) {
            
            int build_length = bytes_consumed + 1;
            char* built = em_parser_alloc(state, build_length);

            memset(built, 0, build_length);
            memcpy(built, code+starting_index, i - starting_index);

            built[build_length-1] = 0;

            if (size_to_skip != NULL) {
                if (i + 1 < len) {
                    *size_to_skip = total_skip;
                } else {
                    *size_to_skip = -1;
                }
            }

            return built;
        }
    }

    if (size_to_skip != NULL) {
        *size_to_skip = 0;
    }
    return NULL;
}