// TODO
// Control flow breaks line number (loops)
// Control flow should not seek to comments or the same symbol when set
// Padding for UDTs so they are compatible with C
// String tables
// Not leaking everywhere
// callable functions (?)
// Signed is broken (unsigned everywhere)

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h> 
#include <stdbool.h>
#include <time.h>

#include "eso_vm.h"
#include "eso_udt.h"
#include "eso_log.h"
#include "eso_parse.h"
#include "eso_stack.h"
#include "eso_debug.h"
#include "eso_literal.h"
#include "eso_boolean.h"
#include "eso_memory.h"
#include "eso_debug.h"
#include "eso_controlflow.h"

void run_file(const char* file);
void run(const char* filename, const char* i, int len);

int main(int argc, char** argv) {

    time(NULL);

    if (argc < 2) {
        fprintf(stderr, "At least one argument required\n");
        exit(0);
    }

    run_file(argv[1]);

    return 0;
}

void run_file(const char* file) {
    FILE* input = fopen(file, "r");

    if (input == NULL) {
        fprintf(stderr, "Cannot find or open %s\n", file);
        exit(1);
        return;
    }

    fseek(input, 0, SEEK_END);

    int length = ftell(input);

    rewind(input);

    char* file_content = em_perma_alloc(length+1);
    memset(file_content, 0, length+1);

    if (fread(file_content, 1, length, input) != length) {
        fprintf(stderr, "Could not successfully read the entire length of %s\n", file);
        exit(1);
    }

    file_content[length] = 0;

    run(file, file_content, strlen(file_content));

    fclose(input);
}

void run(const char* filename, const char* code, int len) {

    bool premature_exit = false;

    em_state state;
    memset(&state, 0, sizeof(em_state));
    state.stack_size = 4096;
    state.stack_ptr = -1;
    state.stack = em_perma_alloc(sizeof(em_stack_item) * state.stack_size);
    state.filename = filename;
    memset(state.stack, 0, sizeof(em_stack_item) * state.stack_size);

    state.control_flow_token = '@';
    state.type_ptr = -1;
    state.max_types = 255;
    state.types = em_perma_alloc(sizeof(em_type_definition) * state.max_types);
    memset(state.types, 0, sizeof(em_type_definition) * state.max_types);

    state.max_pointers = 2048;
    state.pointers = em_perma_alloc(sizeof(em_managed_ptr) * state.max_pointers);
    memset(state.pointers, 0, sizeof(em_managed_ptr) * state.max_pointers);
    state.pointer_ptr = -1;

    ESOMODE mode = EM_MEMORY;

    for (int i = 0; i < len; i++) {

        if (premature_exit) {
            break;
        }

        if (code[i] == '#') {
            log_verbose("Terminating at comment at index %d\n", i);

            for(int current = i; current < len; current++) {
                if (code[current] == '\n') {
                    i = current - 1;
                    break;
                }
            }

            continue;
        }

        if (code[i] == '\n') {
            state.file_line++;
            continue;
        }

        if (isspace(code[i]) || code[i] == '\r') {
            continue;
        }

        // 'Free letters' i.e. top level mode changes
        switch(tolower(code[i])) {

            // Change mode
            case 'm': {
                char new_mode = safe_get(code, i+1, len);
                state.last_mode_change = i;
                switch(tolower(new_mode)) {
                    case 'b': mode = EM_BOOLEAN; break;
                    case 'm': mode = EM_MEMORY; break;
                    case 'l': mode = EM_LITERAL; break;
                    case 's': mode = EM_STACK; break;
                    case 'c': mode = EM_CC; break;
                    case 'd': mode = EM_DEBUG; break;
                    case 'u': mode = EM_UDT; break;
                    case 'f': mode = EM_CONTROL_FLOW; break;
                default:
                    em_panic(code, i, len, &state, "Unknown mode change '%c'\n", new_mode);
                }

                log_verbose("\033[0;31m%c %c\033[0;0m (Mode change)\n", code[i], new_mode);

                i++;
                continue;
            }
            break;

            default:
            {               
                int skip = 0;

                // Allow modes to eat text
                switch(mode) {
                    case EM_MEMORY: skip = run_memory(&state, code, i, len); break;
                    case EM_LITERAL: skip = run_literal(&state, code, i, len); break;
                    case EM_STACK: skip = run_stack(&state, code, i, len); break;
                    case EM_CC: break;
                    case EM_DEBUG: skip = run_debug(&state, code, i, len); break;
                    case EM_BOOLEAN: skip = run_boolean(&state, code, i, len); break;
                    case EM_UDT: skip = run_udt(&state, code, i, len); break;

                    // Unlike other modes control flow directly sets where
                    // we resume from
                    case EM_CONTROL_FLOW: 
                        i = run_control(&state, code, i, len);
                        continue;
                }

                log_verbose("Skipping %d characters\n", skip);

                if (skip >= 0) {
                    i += skip;
                } else {
                    log_verbose("EOF reached detected by skip of %d\n", skip);
                    premature_exit = true;
                }

                break;
            }
        }       
       
    }
}