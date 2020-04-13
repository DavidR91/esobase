#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h> 
#include <stdbool.h>

#include "eso_vm.h"
#include "eso_log.h"

// Note that this totally erases any content in the address 
int stack_push(em_state* state) {

    if ((state->stack_ptr + 1) >= state->stack_size) {
        em_panic("(Source not available)", 0, 22, state, "Stack overflow (%d maximum of %d)", state->stack_ptr, state->stack_size);
    }

    state->stack_ptr++;

    memset(&state->stack[state->stack_ptr], 0, sizeof(em_stack_item));
    log_verbose("Stack push result index = %d\n", state->stack_ptr);
    return state->stack_ptr;
}

em_stack_item* stack_pop(em_state* state) {
    if (state->stack_ptr < 0) {
        return NULL;
    } else {
        em_stack_item* top = &state->stack[state->stack_ptr];

        if (is_code_using_managed_memory(top->code)) {

            log_verbose("Stack pop %d is \033[0;31mremoving reference to managed memory\033[0;0m\n", state->stack_ptr);
        
            free_managed_ptr("(Source not available)", 0, 22, state, top->u.v_mptr);
        }

        state->stack_ptr--;

        log_verbose("Stack pop result index = %d\n", state->stack_ptr);
        
        return top;
    }
}

em_stack_item* stack_top_minus(em_state* state, int minus) {
    if (state->stack_ptr - minus < 0) {
        return NULL;
    } else {
        return &state->stack[state->stack_ptr - minus];
    }
}

em_stack_item* stack_top(em_state* state) {
    if (state->stack_ptr < 0) {
        return NULL;
    } else {
        return &state->stack[state->stack_ptr];
    }
}

bool is_stack_item_numeric(em_stack_item* item) {

    switch(item->code) {
        case '1': 
        case '2': 
        case '4': 
        case '8': 
        case 'f':
        case 'd':
            return true;
        default:
            return false;
    }
}


int run_stack(em_state* state, const char* code, int index, int len) {

    char current_code = tolower(code[index]);

    log_verbose("\033[0;31m%c\033[0;0m (Stack)\n", current_code);

    switch(current_code) {

        // pop
        case 'p':         
            log_verbose("Stack pop\n");
            
            if (stack_pop(state) == NULL) {
                em_panic(code, index, len, state, "Cannot pop from stack: stack is empty");
            }

            return 0;

        // Duplicate top of stack
        case 'd':
        {
            em_stack_item* dup = stack_top(state);

            if (dup == NULL) {
                em_panic(code, index, len, state, "Cannot duplicate stack top: stack is empty");
            }

            int ptr = stack_push(state);

            // If it's a managed memory object just create a reference
            if (is_code_using_managed_memory(dup->code)) {
                em_managed_ptr* reference = dup->u.v_mptr;
                reference->references++;

                state->stack[ptr].u.v_mptr = reference;
                state->stack[ptr].code = dup->code;

            } else {
                memcpy(&state->stack[ptr], dup, sizeof(em_stack_item));
            }
        }
        break;

        // Create a copy of an item at a specific top minus position
        case 'c': 
        {
            // We need an offset at the top of the stack
            em_stack_item* offset = stack_top(state);

            if (offset == NULL) {
                em_panic(code, index, len, state, "Cannot copy stack item: Need offset value at stack top");
            }

            if (!is_code_numeric(offset->code)) {
                em_panic(code, index, len, state, "Cannot copy stack item: Offset at stack top must be of numeric type");
            }

            uint32_t minus = 0; // Ignore the offset itself

            switch(offset->code) {
                case '1': minus += offset->u.v_byte; break;
                case '2': minus += offset->u.v_int16; break;
                case '4': minus += offset->u.v_int32; break;
                case '8': minus += offset->u.v_int64; break;
                default: 
                    em_panic(code, index, len, state, "Unhandled type %c in determining stack copy offset", offset->code);
                break;
            }

            // Now actually try to get the item to copy
            em_stack_item* to_copy = stack_top_minus(state, minus);

            if (to_copy == NULL) {
                em_panic(code, index, len, state, "Cannot copy stack item: Stack tells us to retrieve item at -%d to copy but no such item exists", minus);
            }

            // Pop the offset info
            stack_pop(state);

            int ptr = stack_push(state);

            // Copy is a reference 
            if (is_code_using_managed_memory(to_copy->code)) {
                em_managed_ptr* reference = to_copy->u.v_mptr;
                reference->references++;

                int ptr = stack_push(state);
                state->stack[ptr].u.v_mptr = reference;
                state->stack[ptr].code = to_copy->code;

            } else {
                memcpy(&state->stack[ptr], to_copy, sizeof(em_stack_item));
            }
        }
        break;
    }

    return 0;
}