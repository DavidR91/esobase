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
    state->stack_ptr++;
    memset(&state->stack[state->stack_ptr], 0, sizeof(em_stack_item));
    return state->stack_ptr;
}

em_stack_item* stack_pop(em_state* state) {
    if (state->stack_ptr < 0) {
        return NULL;
    } else {
        em_stack_item* top = &state->stack[state->stack_ptr];

        state->stack_ptr--;

        return top;
    }
}

em_stack_item* stack_pop_by(em_state* state, int amount) {
    if (state->stack_ptr < 0) {
        return NULL;
    } else {
        em_stack_item* top = &state->stack[state->stack_ptr];

        state->stack_ptr -= amount;

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

     log_verbose("DEBUG VERBOSE\t\tStack start '%c'\n", current_code);

    switch(current_code) {

        // pop
        case 'p':         
            log_verbose("DEBUG VERBOSE\t\tStack pop\n");
            
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
            memcpy(&state->stack[ptr], dup, sizeof(em_stack_item));
        }
        break;
    }

    return 0;
}