#include "eso_vm.h"
#include "eso_log.h"
#include "eso_parse.h"
#include "eso_stack.h"
#include "eso_log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h> 
#include <stdbool.h>

int run_memory(em_state* state, const char* code, int index, int len) {

    char current_code = tolower(code[index]);
    log_verbose("DEBUG VERBOSE\t\tMemory start '%c'\n", current_code);

    switch(current_code) {

        // alloc
        case 'a':
        {
            // We need an int on the stack
            em_stack_item* top = stack_top(state);

            if (top == NULL) {
                em_panic(code, index, len, state, "Insufficient arguments to memory allocation: requires integer size on stack top");
            }

            uint32_t real_size = 0;

            switch(top->code) {
                case '1': real_size = top->u.v_byte; break;
                case '2': real_size = top->u.v_int16; break;
                case '4': real_size = top->u.v_int32; break;
                case '8': real_size = top->u.v_int64; break;
                default: em_panic(code, index, len, state, "Memory allocation requires integer number on top of stack - found %c\n", top->code);
            }

            void* arb = malloc(real_size);
            memset(arb, 0, real_size);

            stack_pop(state);

            int ptr = stack_push(state);

            state->stack[ptr].signage = false;
            state->stack[ptr].code = '*';
            state->stack[ptr].u.v_ptr = arb;
            state->stack[ptr].size = real_size;

            log_verbose("DEBUG VERBOSE\t\tAllocated %db of memory @ %p\n", real_size, arb);

            return 0;
        }

        // free
        case 'f': 
        {
            // We need a MANAGED pointer on top of the stack
            em_stack_item* top = stack_top(state);

            if (top == NULL) {
                em_panic(code, index, len, state, "Insufficient arguments to memory free: requires pointer on stack top");
            }

            if (top->code != '*') {
                em_panic(code, index, len, state, "Memory free requires pointer top of stack - found %c\n", top->code);            
            }

            if (top->size == 0) {
                em_panic(code, index, len, state, "Attempting to free allocation of zero size: Unlikely to be legitimate allocation");
            }

            log_verbose("DEBUG VERBOSE\t\tFreeing %db of memory @ %p\n", top->size, top->u.v_ptr);
            free(top->u.v_ptr);

            top->signage = false;
            top->code = 0;
            top->u.v_ptr = 0;
            top->size = 0;

            stack_pop(state);   
            return 0;
        }

        // set
        case 's': 
        {

            return 0;
        }


        default:
            em_panic(code, index, len, state, "Unknown memory instruction %c", current_code);
            return 0;

    }
}