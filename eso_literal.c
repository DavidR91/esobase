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

int run_literal(em_state* state, const char* code, int index, int len) {
   
    int size_to_skip = 0;

    char current_code = tolower(code[index]);
    log_verbose("DEBUG VERBOSE\t\tLiteral start '%c'\n", current_code);

    switch(current_code) {
        case '?': 
        {
            int top = stack_push(state);
            
            char v = tolower(safe_get(code, index+1, len));

            state->stack[top].u.v_bool = (v == 'y' || v == 't');
            state->stack[top].code = current_code;

            log_verbose("DEBUG VERBOSE\t\tPush bool literal %c\n", v);
        }
        return 1;

        case '1': 
        {
            char* test = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(code, index, len, state, "Could not find a complete literal for byte: Did you forget to terminate it?");
            }

            uint8_t v = atoi(test);
            free(test);

            int top = stack_push(state);
            state->stack[top].code = current_code;
            state->stack[top].u.v_byte = v;

            log_verbose("DEBUG VERBOSE\t\tPush u8 literal %d\n", v);
        }
        return size_to_skip;

        case '2': 
        {
            char* test = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(code, index, len, state, "Could not find a complete literal for int16: Did you forget to terminate it?");
            }

            uint16_t v = atoi(test);
            free(test);

            int top = stack_push(state);
            state->stack[top].code = current_code;
            state->stack[top].u.v_int32 = v;

            log_verbose("DEBUG VERBOSE\t\tPush u16 literal %d\n", v);
        }
        return size_to_skip;

        case '4': 
        {
            char* test = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(code, index, len, state, "Could not find a complete literal for int32: Did you forget to terminate it?");
            }

            uint32_t v = atoi(test);
            free(test);

            int top = stack_push(state);
            state->stack[top].code = current_code;
            state->stack[top].u.v_int32 = v;

            log_verbose("DEBUG VERBOSE\t\tPush u32 literal %d\n", v);
        }
        return size_to_skip;

        case '8': 
        {
            char* test = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(code, index, len, state, "Could not find a complete literal for int64: Did you forget to terminate it?");
            }

            uint64_t v = strtol(test, NULL, 10);
            free(test);

            int top = stack_push(state);
            state->stack[top].code = current_code;
            state->stack[top].u.v_int64 = v;

            log_verbose("DEBUG VERBOSE\t\tPush u64 literal %llu\n", v);
        }
        return size_to_skip;

        case 'f':  
        {
            char* test = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(code, index, len, state, "Could not find a complete literal for float32: Did you forget to terminate it?");
            }

            float v = strtod(test, NULL);
            free(test);

            int top = stack_push(state);
            state->stack[top].code = current_code;
            state->stack[top].u.v_float = v;

            log_verbose("DEBUG VERBOSE\t\tPush float literal %f\n", v);
        }
        return size_to_skip;

        case 'd':  
        {
            char* test = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(code, index, len, state, "Could not find a complete literal for float64: Did you forget to terminate it?");
            }

            double v = strtod(test, NULL);
            free(test);

            int top = stack_push(state);
            state->stack[top].code = current_code;
            state->stack[top].u.v_double = v;

            log_verbose("DEBUG VERBOSE\t\tPush double literal %f\n", v);
        }
        return size_to_skip;

        case 's': 
        {
            // Just leaking this right now, need a string table
            //
            char* text = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (text == NULL) {
                em_panic(code, index, len, state, "Could not find a complete literal for string: Did you forget to terminate it?");
            }

            int top = stack_push(state);
            state->stack[top].code = current_code;
            state->stack[top].u.v_ptr = text; 
            state->stack[top].size = strlen(text) + 1; //alloc until is always NUL terminated

            log_verbose("DEBUG VERBOSE\t\tPush string literal %s skip %d\n", text, size_to_skip);
        }
        return size_to_skip;

        case '*': break;
        case '^': break;

        default:
            em_panic(code, index, len, state, "Unknown literal instruction %c", current_code);
            return 0;
    }

    return 0;
}