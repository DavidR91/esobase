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
    log_verbose("\033[0;31m%c\033[0;0m (Memory)\n", current_code);

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

            void* arb = em_usercode_alloc(real_size);
            memset(arb, 0, real_size);

            // Create a managed pointer
            em_managed_ptr* mptr = create_managed_ptr(state);
            mptr->size = real_size;
            mptr->raw = arb;
            mptr->concrete_type = NULL;
            mptr->references++; // Stack holds reference

            stack_pop(state);

            int ptr = stack_push(state);
            state->stack[ptr].code = '*';
            state->stack[ptr].u.v_mptr = mptr;

            log_verbose("Allocated %db of memory @ %p\n", real_size, arb);

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