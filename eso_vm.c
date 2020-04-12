#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h> 
#include <stdbool.h>

#include "eso_vm.h"
#include "eso_log.h"
#include "eso_debug.h"

void em_panic(const char* code, int index, int len, em_state* state, const char* format, ...) {
    va_list argptr;
    va_start(argptr, format);
    fprintf(stderr, "\n%s\n", "\033[0;31m* * * * PANIC * * * *\n\n");
    vfprintf(stderr, format, argptr);

    fprintf(stderr, "\n\n");
    dump_instructions(code, index, len, state);
    fprintf(stderr, "%s", "\033[0m\n\n");
    dump_stack(state);
    fprintf(stderr, "\n");
    va_end(argptr);
    abort();
}

size_t code_sizeof(char code) {
    switch(code) {
        case '?': return sizeof(bool); break;
        case '1': return sizeof(uint8_t); break;
        case '2': return sizeof(uint16_t); break;
        case '4': return sizeof(uint32_t); break;
        case '8': return sizeof(uint64_t); break;
        case 'f': return sizeof(float); break;
        case 'd': return sizeof(double); break;
        case 's': return sizeof(char*); break;
        case '*': return sizeof(void*); break;
        case '^': return sizeof(void*); break;
    }
    return 0;
}


char safe_get(const char* code, int index, int len) {
    if (index < 0 || index >= len) {
        abort();
    }

    return code[index];
}

bool is_code_numeric(char code) {
    switch(code) {
        case '1':
        case '2':
        case '4':
        case '8':
            return true;
    }

    return false;
}

em_type_definition* create_new_type(em_state* state) {
    state->type_ptr++;
    memset(&state->types[state->type_ptr], 0, sizeof(em_type_definition));
    return &state->types[state->type_ptr];
}

em_managed_ptr* create_managed_ptr(em_state* state) {
    state->pointer_ptr++;
    memset(&state->pointers[state->pointer_ptr], 0, sizeof(em_managed_ptr));
    return &state->pointers[state->pointer_ptr];
}

void free_managed_ptr(const char* code, int index, int len, em_state* state, em_managed_ptr* mptr) {

    if (mptr->size == 0) {
        em_panic(code, index, len, state, "Attempting to free allocation of zero size: Unlikely to be legitimate allocation");
    }

    log_verbose("DEBUG VERBOSE\t\tFreeing %db of memory @ %p\n", mptr->size, mptr->raw);

    memset(mptr, 0, sizeof(em_managed_ptr));
    mptr->dead = true;
}