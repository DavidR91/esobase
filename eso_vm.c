#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h> 
#include <stdbool.h>

#include "eso_vm.h"
#include "eso_log.h"
#include "eso_debug.h"
#include "em_c_bindings.h"

em_state* create_state(const char* filename) {

    em_state* state = em_perma_alloc(NULL, sizeof(em_state));
    memset(state, 0, sizeof(em_state));
    state->memory_permanent.allocated = sizeof(em_state); 
    state->memory_permanent.peak_allocated = sizeof(em_state); 
    state->stack_size = 4096;
    state->stack_ptr = -1;
    state->stack = em_perma_alloc(state, sizeof(em_stack_item) * state->stack_size);
    state->filename = filename;
    memset(state->stack, 0, sizeof(em_stack_item) * state->stack_size);

    state->control_flow_token = '@';
    state->type_ptr = -1;
    state->max_types = 255;
    state->types = em_perma_alloc(state, sizeof(em_type_definition) * state->max_types);
    memset(state->types, 0, sizeof(em_type_definition) * state->max_types);

    state->c_binding_ptr = -1;
    state->max_c_bindings = 128;
    state->c_bindings = em_perma_alloc(state, sizeof(em_c_binding) * state->max_c_bindings);
    memset(state->c_bindings, 0, sizeof(em_c_binding) * state->max_c_bindings);

    state->mode = EM_MEMORY;

    // Setup null
    state->null = em_perma_alloc(state, sizeof(em_managed_ptr));
    memset(state->null, 0, sizeof(em_managed_ptr));
    state->null->size = 0;
    state->null->raw = (void*)0xDEADBEEF;
    state->null->concrete_type =  (void*)0xDEADBEEF;
    state->null->references = 65535;

    em_bind_c_default(state);

    return state;
}

void em_add_reference(em_state* state, em_managed_ptr* mptr) {

    if (mptr == state->null) {
        em_panic(state, "Attempting to add reference to the null pointer %p", state->null);
    }

    if (mptr->references >= 65534) {
        em_panic(state, "Attempting to add 65535 references to object");
    }

    mptr->references++;
}

void* em_perma_alloc(em_state* state, size_t size) {
    if (state != NULL) {
        state->memory_permanent.allocated += size;

        if (state->memory_permanent.allocated > state->memory_permanent.peak_allocated) {
            state->memory_permanent.peak_allocated = state->memory_permanent.allocated;
        }
    }

    return malloc(size);
}

void* em_usercode_alloc(em_state* state, size_t size, bool bookkeep_as_overhead) {

    log_verbose("USERCODE ALLOCATE %d %db\n", size, state->memory_usercode.allocated);

    if (state != NULL) {

        if (bookkeep_as_overhead) {
             state->memory_usercode.overhead += size;

            if (state->memory_usercode.overhead > state->memory_usercode.peak_overhead) {
                state->memory_usercode.peak_overhead = state->memory_usercode.overhead;
            }
        } else {

            state->memory_usercode.allocated += size;
           
            if (state->memory_usercode.allocated > state->memory_usercode.peak_allocated) {
                state->memory_usercode.peak_allocated = state->memory_usercode.allocated;
            }
        }

    }

    return malloc(size);
}

void em_usercode_free(em_state* state, void* ptr, size_t size, bool bookkeep_as_overhead) {

    log_verbose("USERCODE FREE %d %db\n", size, state->memory_usercode.allocated);

    if (state != NULL) {

        if (bookkeep_as_overhead) {
            state->memory_usercode.overhead -= size;
        } else {
            state->memory_usercode.allocated -= size;
        }
    }

    free(ptr);
}

void* em_parser_alloc(em_state* state, size_t size) {

    if (state != NULL) {
        state->memory_parser.allocated += size;

        if (state->memory_parser.allocated > state->memory_parser.peak_allocated) {
            state->memory_parser.peak_allocated = state->memory_parser.allocated;
        }
    }

    return malloc(size);
}

void em_transfer_alloc_parser_usercode(em_state* state, size_t size) {
    state->memory_parser.allocated -= size;
    state->memory_usercode.allocated += size;

     if (state->memory_usercode.allocated > state->memory_usercode.peak_allocated) {
        state->memory_usercode.peak_allocated = state->memory_usercode.allocated;
    }
}

void em_parser_free(em_state* state, void* ptr) {
    free(ptr);
}

void em_panic(em_state* state, const char* format, ...) {
    va_list argptr;
    va_start(argptr, format);
    log_printf( "\n%s\n", "\033[0;31m* * * * PANIC * * * *\n\n");
    vfprintf(stdout, format, argptr);

    log_printf( "\n\n");
    
    if (state != NULL) {
        dump_instructions(state);
    } else {
        log_printf( "(No machine state and no instructions available)\n");
    }

    log_printf( "%s", "\033[0m\n\n");

    if (state != NULL) {
        dump_stack(state);
    } else {
        log_printf( "(No machine state and no stack available)\n");
    }

    log_printf( "\n");
    va_end(argptr);
    exit(1);
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
        case 's': return sizeof(em_managed_ptr*); break;
        case '*': return sizeof(void*); break;
        case '^': return sizeof(void*); break;
        case 'u': return sizeof(em_managed_ptr*); break;

    }
    return 0;
}


char safe_get(const char* code, int index, int len) {
    if (index < 0 || index >= len) {
        em_panic(NULL, "Unexpectedly out of bounds safe-getting single character from index %d with code of length %d", index, len);
        exit(1);
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

bool is_code_using_managed_memory(char code) {
    switch(code) {
        case 'u':
        case 's':
        case '*':
            return true;
    }

    return false;
}

em_type_definition* create_new_type(em_state* state) {

   if ((state->type_ptr+1) >= state->max_types) {
        em_panic(state, "Type definition overflow (%d types defined maximum of %d)", state->type_ptr, state->max_types);
    }

    state->type_ptr++;
    memset(&state->types[state->type_ptr], 0, sizeof(em_type_definition));
    return &state->types[state->type_ptr];
}

em_managed_ptr* create_managed_ptr(em_state* state) {
    em_managed_ptr* ptr = em_usercode_alloc(state, sizeof(em_managed_ptr), true); // Pure overhead
    memset(ptr, 0, sizeof(em_managed_ptr));
    return ptr;
}

void free_managed_ptr(em_state* state, em_managed_ptr* mptr) {

    if (mptr == state->null) {
        em_panic(state, "Attempting to free the null pointer %p", state->null);
    }

    if (mptr->size == 0) {
        em_panic(state, "Attempting to free allocation of zero size: Unlikely to be legitimate allocation");
    }

    mptr->references--;
    log_verbose("Freeing %p now at %d references\n", mptr->raw, mptr->references);

    if (mptr->references <= 0) {

        // If we're actually a reference of something else, then free that
        log_verbose("Freeing %db of memory @ %p\n", mptr->size, mptr->raw);

        if (mptr->concrete_type != NULL) {
            log_verbose("Need to free fields of concrete type %s\n", mptr->concrete_type->name);

            for(int field = 0; field < strlen(mptr->concrete_type->types); field++) {

                if (is_code_using_managed_memory(mptr->concrete_type->types[field])) {

                    log_verbose("Freeing field %s of %s (%p +%db)\n", mptr->concrete_type->field_names[field], mptr->concrete_type->name, mptr->raw, mptr->concrete_type->start_offset_bytes[field]);

                    em_managed_ptr* to_free = *(em_managed_ptr**)(mptr->raw + mptr->concrete_type->start_offset_bytes[field]);

                    log_verbose("Resolves to %p\n", to_free);

                    if (to_free == state->null) {
                        log_verbose("Resolved to NULL: Not initialized nothing to free\n", to_free);
                    } else {
                        free_managed_ptr(state, to_free);
                    }
                }
            }
        }

        em_usercode_free(state, mptr->raw, mptr->size, false); // Real memory
        memset(mptr, 0, sizeof(em_managed_ptr));       
        em_usercode_free(state, mptr, sizeof(em_managed_ptr), true); // Overhead
    }
}

uint32_t calculate_file_line(em_state* state) {
    uint32_t line = 1;
    for(int i = 0; i < state->index; i++) {
        if (state->code[i] == '\n') {
            line++;
        }
    }
    return line;
}

uint32_t calculate_file_column(em_state* state) {
    uint32_t column = 1;
    for(int i = 0; i < state->index; i++) {
        if (state->code[i] == '\n') {
            column = 1;
        } else {
            column++;
        }
    }
    return column;
}

void em_bind_c_call(em_state* state, char* name, em_c_call call) {
     state->c_binding_ptr++;

     em_c_binding* bind = &state->c_bindings[state->c_binding_ptr];

     bind->name = em_perma_alloc(state, strlen(name) + 1);
     memset(bind->name, 0, strlen(name) + 1);
     memcpy(bind->name, name, strlen(name));

     bind->bound = call;

     log_verbose("Bound C method '%s' @ %p\n", bind->name, call);
}