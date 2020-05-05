// TODO
// Global callstack with flags such as 'unsafe'
// Permit arbitrary byte get/set on various memory data when unsafe set
// When inspecting types show a byte map including padding
// Better error handling esp. inside of C calls
// Newline and escape processing
// How to unwrap c strings out ahead of c calls?
// Control flow should not seek to comments or the same symbol when set
// Signed is broken (unsigned everywhere)
// Fix perma allocs so they are freeable before exit

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
#include "eso_c.h"
#include <string.h>

em_state* run_file(const char* file, bool do_assert_no_leak);
void run(em_state* state);
void repl(em_state* state);

int main(int argc, char** argv) {

    if (argc < 2) {
        log_printf("REPL mode\n");
        em_state* state = create_state("stdin");
        repl(state);
    }

    bool interactive_mode = (argc >= 3 && strcmp(argv[2], "--i") == 0);

    em_state* state = run_file(argv[1], !interactive_mode); // Only do leak check if not interactive

    if (state != NULL && interactive_mode) {
        repl(state);
    }

    return 0;
}

void repl(em_state* state) {

    char* line = NULL;
    size_t unhelpful_junk = 0;
    size_t line_len = 0;

    log_printf(">> ");

    while((line_len = getline(&line, &unhelpful_junk, stdin)) != -1) {

        state->code = line;
        state->len = line_len;
        state->index = 0;

        run(state);
        log_printf("\n>> ");
    }

    exit(0);
}


em_state* run_file(const char* file, bool do_assert_no_leak) {
    FILE* input = fopen(file, "rb");

    if (input == NULL) {
        log_printf( "Cannot find or open %s\n", file);
        exit(1);
        return NULL;
    }

    fseek(input, 0, SEEK_END);

    int length = ftell(input);

    rewind(input);

    em_state* state = create_state(file);

    char* file_content = em_perma_alloc(state, length+1);
    memset(file_content, 0, length+1);

    if (fread(file_content, 1, length, input) != length) {
        log_printf( "Could not successfully read the entire length of %s\n", file);
        exit(1);
    }

    file_content[length] = 0;   

    state->code = file_content;
    state->len = strlen(file_content);

    run(state);

    if (do_assert_no_leak) {
        assert_no_leak(state);
    }

    fclose(input);
    return state;
}

void run(em_state* state) {

    bool premature_exit = false;

    for (int i = 0; i < state->len; i++) {

        state->index = i;

        if (premature_exit) {
            break;
        }

        if (state->code[i] == '#') {
            log_verbose("Terminating at comment at index %d\n", i);

            for(int current = i; current < state->len; current++) {
                if (state->code[current] == '\n') {
                    i = current - 1;
                    break;
                }
            }

            continue;
        }

        if (isspace(state->code[i]) || state->code[i] == '\r') {
            continue;
        }

        // 'Free letters' i.e. top level mode changes
        switch(tolower(state->code[i])) {

            // Change mode
            case 'm': {

                log_ingestion(state->code[i]);

                char new_mode = safe_get(state->code, i+1, state->len);
                state->last_mode_change = i;

                log_ingestion(new_mode);

                switch(tolower(new_mode)) {
                    case 'b': state->mode = EM_BOOLEAN; break;
                    case 'm': state->mode = EM_MEMORY; break;
                    case 'l': state->mode = EM_LITERAL; break;
                    case 's': state->mode = EM_STACK; break;
                    case 'c': state->mode = EM_CC; break;
                    case 'd': state->mode = EM_DEBUG; break;
                    case 'u': state->mode = EM_UDT; break;
                    case 'f': state->mode = EM_CONTROL_FLOW; break;
                default:
                    em_panic(state, "Unknown mode change '%c'\n", new_mode);
                    break;
                }

                log_verbose("\033[0;31m%c %c\033[0;0m (Mode change)\n", state->code[i], new_mode);

                i++;
                continue;
            }
            break;

            default:
            {               
                int skip = 0;

                #ifdef ESO_VERBOSE_DEBUG
                    log_verbose("---------------------------- %d:%d\n", calculate_file_line(state), calculate_file_column(state));
                #endif

                // Allow modes to eat text
                switch(state->mode) {
                    case EM_MEMORY: skip = run_memory(state); break;
                    case EM_LITERAL: skip = run_literal(state); break;
                    case EM_STACK: skip = run_stack(state); break;
                    case EM_CC: skip = run_c(state); break;
                    case EM_DEBUG: skip = run_debug(state); break;
                    case EM_BOOLEAN: skip = run_boolean(state); break;
                    case EM_UDT: skip = run_udt(state); break;

                    // Unlike other modes control flow directly sets where
                    // we resume from
                    case EM_CONTROL_FLOW: 
                        i = run_control(state);

                        if (i == -1) {
                            premature_exit = true;
                        }

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