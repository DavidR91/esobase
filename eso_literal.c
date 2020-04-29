#include "eso_vm.h"
#include "eso_log.h"
#include "eso_parse.h"
#include "eso_stack.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h> 
#include <stdbool.h>

int run_literal(em_state* state) {
   
    int size_to_skip = 0;

    char current_code = tolower(state->code[state->index]);
    log_verbose("\033[0;31m%c\033[0;0m (Literal)\n", current_code);
    log_ingestion(current_code);
    
    switch(current_code) {
        case '?': 
        {
            int top = stack_push(state);
            
            char v = tolower(safe_get(state->code, state->index+1, state->len));

            state->stack[top].u.v_bool = (v == 'y' || v == 't');
            state->stack[top].code = current_code;

            log_verbose("Push bool literal %c\n", v);
        }
        return 1;

        case '1': 
        {
            char* test = alloc_until(state, state->code, state->index+1, state->len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(state, "Could not find a complete literal for byte: Did you forget to terminate it?");
            }

            uint8_t v = 0;

            if (test[0] == 'u' && strlen(test) > 1) {

                // Specifying a character code
                v = atoi(test + 1);
            } else {

                // Use ASCII value as-is
                v = test[0];
            }

            em_parser_free(state, test);

            int top = stack_push(state);
            state->stack[top].code = current_code;
            state->stack[top].u.v_byte = v;

            log_verbose("Push u8 literal %d\n", v);
        }
        return size_to_skip;

        case '2': 
        {
            char* test = alloc_until(state, state->code, state->index+1, state->len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(state, "Could not find a complete literal for int16: Did you forget to terminate it?");
            }

            uint16_t v = atoi(test);
            em_parser_free(state, test);

            int top = stack_push(state);
            state->stack[top].code = current_code;
            state->stack[top].u.v_int32 = v;

            log_verbose("Push u16 literal %d\n", v);
        }
        return size_to_skip;

        case '4': 
        {
            char* test = alloc_until(state, state->code, state->index+1, state->len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(state, "Could not find a complete literal for int32: Did you forget to terminate it?");
            }

            uint32_t v = atoi(test);
            em_parser_free(state, test);

            int top = stack_push(state);
            state->stack[top].code = current_code;
            state->stack[top].u.v_int32 = v;

            log_verbose("Push u32 literal %d\n", v);
        }
        return size_to_skip;

        case '8': 
        {
            char* test = alloc_until(state, state->code, state->index+1, state->len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(state, "Could not find a complete literal for int64: Did you forget to terminate it?");
            }

            uint64_t v = strtol(test, NULL, 10);
            em_parser_free(state, test);

            int top = stack_push(state);
            state->stack[top].code = current_code;
            state->stack[top].u.v_int64 = v;

            log_verbose("Push u64 literal %llu\n", v);
        }
        return size_to_skip;

        case 'f':  
        {
            char* test = alloc_until(state, state->code, state->index+1, state->len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(state, "Could not find a complete literal for float32: Did you forget to terminate it?");
            }

            float v = strtod(test, NULL);
            em_parser_free(state, test);

            int top = stack_push(state);
            state->stack[top].code = current_code;
            state->stack[top].u.v_float = v;

            log_verbose("Push float literal %f\n", v);
        }
        return size_to_skip;

        case 'd':  
        {
            char* test = alloc_until(state, state->code, state->index+1, state->len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(state, "Could not find a complete literal for float64: Did you forget to terminate it?");
            }

            double v = strtod(test, NULL);
            em_parser_free(state, test);

            int top = stack_push(state);
            state->stack[top].code = current_code;
            state->stack[top].u.v_double = v;

            log_verbose("Push double literal %f\n", v);
        }
        return size_to_skip;

        case 's': 
        {
            char* text = alloc_until(state, state->code, state->index+1, state->len, ';', true, &size_to_skip);

            if (text == NULL) {
                em_panic(state, "Could not find a complete literal for string: Did you forget to terminate it?");
            }

            em_managed_ptr* mptr = create_managed_ptr(state);
            mptr->raw = text;
            mptr->size = strlen(text) + 1; //alloc until is always NUL terminated
            mptr->concrete_type = NULL;
            em_add_reference(state, mptr); // Stack holds a reference

            // Usercode now owns this (bookkeeping)
            em_transfer_alloc_parser_usercode(state, mptr->size);

            log_verbose("String push created %db managed memory for value \"%s\"\n", mptr->size, mptr->raw);

            int top = stack_push(state);
            state->stack[top].code = current_code;
            state->stack[top].u.v_mptr = mptr; 

            log_verbose("Push string literal %s skip %d\n", text, size_to_skip);
        }
        return size_to_skip;

        case '*': break;

        default:
            em_panic(state, "Unknown literal instruction %c", current_code);
            return 0;
    }

    return 0;
}