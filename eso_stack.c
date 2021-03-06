#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h> 
#include <stdbool.h>

#include "eso_vm.h"
#include "eso_log.h"
#include "eso_debug.h"

// Note that this totally erases any content in the address 
int stack_push(em_state* state) {

    if ((state->stack_ptr + 1) >= state->stack_size) {
        em_panic(state, "Stack overflow (%d maximum of %d)", state->stack_ptr, state->stack_size);
    }

    state->stack_ptr++;

    memset(&state->stack[state->stack_ptr], 0, sizeof(em_stack_item));
    log_verbose("Stack push result index = %d\n", state->stack_ptr);
    return state->stack_ptr;
}

// Move every item in the stack up by one, freeing a slot at the bottom.
// Use stack_push_shift_up to call stack_push properly
void stack_shift_up(em_state* state, int index) {

    if (index < 0) {
        return;
    }

    memcpy(&state->stack[index + 1], &state->stack[index], sizeof(em_stack_item));
    memset(&state->stack[index], 0, sizeof(em_stack_item));
    state->stack[index].code = '?';

    return stack_shift_up(state, index - 1);
}

void stack_push_shift_up(em_state* state) {

    stack_push(state);
    stack_shift_up(state, state->stack_ptr);
}

void stack_shift_down(em_state* state, int index) {

    state->stack[index].code = '?';

    memcpy(&state->stack[index], &state->stack[index + 1], sizeof(em_stack_item));

    if (index >= state->stack_ptr) {
        return;
    }

    stack_shift_down(state, index + 1);
}


// Move up every item in the stack until we hit a specific minus index and return
// that entry (now that it's free to use because the other elements have been shifted)
em_stack_item* stack_shift_up_index(em_state* state, int index, int stop_at_minus) {
    
    if (index + 1 > state->stack_ptr) {
        stack_push(state);
    }

    memcpy(&state->stack[index + 1], &state->stack[index], sizeof(em_stack_item));
    memset(&state->stack[index], 0, sizeof(em_stack_item));
    state->stack[index].code = '?';

    if (index == state->stack_ptr - stop_at_minus) {
        return &state->stack[index];
    }

    return stack_shift_up_index(state, index - 1, stop_at_minus);
}

em_stack_item* stack_insert(em_state* state, int minus) {

    int minus_as_index = state->stack_ptr - minus; // e.g. 3  -4 = -1

    // Insert enough entries at the bottom of the stack if we're short
    int missing_entries = 0;

    if (minus_as_index < 0) {
        missing_entries = abs(minus_as_index);  
    } 

    log_verbose("%d missing stack elements for -%d\n", missing_entries, minus);

    if (missing_entries > 0) {

        for (int missing  = 1; missing <= missing_entries; missing++) {
            stack_push_shift_up(state);
        }

        return &state->stack[state->stack_ptr - minus];

    } else {
        return stack_shift_up_index(state, state->stack_ptr, minus);
    }
}

void stack_drop(em_state* state, int minus) {
    log_verbose("Drop %d\n", -minus);
    stack_shift_down(state, state->stack_ptr - minus);
    state->stack_ptr--;
}

em_stack_item* stack_pop(em_state* state) {
    if (state->stack_ptr < 0) {
        return NULL;
    } else {
        em_stack_item* top = &state->stack[state->stack_ptr];

        if (is_code_using_managed_memory(top->code) && top->u.v_mptr != state->null) {

            log_verbose("Stack pop %d is \033[0;31mremoving reference to managed memory\033[0;0m\n", state->stack_ptr);
        
            free_managed_ptr(state, top->u.v_mptr);
        }

        state->stack_ptr--;

        log_verbose("Stack pop result index = %d\n", state->stack_ptr);
        
        return top;
    }
}

em_stack_item* stack_pop_preserve_top(em_state* state, int count) {
    if (state->stack_ptr < 0) {
        return NULL;
    } else {
        int original_stack_ptr = state->stack_ptr;
        em_stack_item* top = &state->stack[original_stack_ptr];

        int ptr = state->stack_ptr - 1;
        int popped = 0;

        while (ptr >= 0 && popped < count) {

            em_stack_item* popping = &state->stack[ptr];

            if (is_code_using_managed_memory(popping->code) && popping->u.v_mptr != state->null) {

                log_verbose("Stack pop %d is \033[0;31mremoving reference to managed memory\033[0;0m\n", ptr);
            
                free_managed_ptr(state, popping->u.v_mptr);
            }

            memset(&state->stack[ptr], 0, sizeof(em_stack_item));

            ptr--;
            popped++;
        }

        state->stack_ptr = ptr + 1;
        memcpy(&state->stack[state->stack_ptr], &state->stack[original_stack_ptr], sizeof(em_stack_item));

        // Ensure old stack data doesn't hang around
        memset(&state->stack[original_stack_ptr], 0, sizeof(em_stack_item));
            
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
        case '^':
            return true;
        default:
            return false;
    }
}


int run_stack(em_state* state) {

    char current_code = tolower(state->code[state->index]);

    log_verbose("\033[0;31m%c\033[0;0m (Stack)\n", current_code);
    log_ingestion(current_code);

    switch(current_code) {

        // pop
        case 'p':         
            log_verbose("Stack pop\n");
            
            if (stack_pop(state) == NULL) {
                em_panic(state, "Cannot pop from stack: stack is empty");
            }

            return 0;

        // Duplicate top of stack
        case 'd':
        {
            em_stack_item* dup = stack_top(state);

            if (dup == NULL) {
                em_panic(state, "Cannot duplicate stack top: stack is empty");
            }

            int ptr = stack_push(state);

            // If it's a managed memory object just create a reference
            if (is_code_using_managed_memory(dup->code)) {
                em_managed_ptr* reference = dup->u.v_mptr; 

                if (reference != state->null) {
                    em_add_reference(state, reference); // Stack holds a reference
                }

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
                em_panic(state, "Cannot copy stack item: Need offset value at stack top");
            }

            if (!is_code_numeric(offset->code)) {
                em_panic(state, "Cannot copy stack item: Offset at stack top must be of numeric type");
            }

            uint32_t minus = 0; // Ignore the offset itself

            switch(offset->code) {
                case '1': minus += offset->u.v_byte; break;
                case '2': minus += offset->u.v_int16; break;
                case '4': minus += offset->u.v_int32; break;
                case '8': 
                     em_panic(state, "64 bit stack offsets are not currently supported (Attempt to copy with %llu)", offset->u.v_int64);
                break;
                default: 
                    em_panic(state, "Unhandled type %c in determining stack copy offset", offset->code);
                break;
            }

            // Now actually try to get the item to copy
            em_stack_item* to_copy = stack_top_minus(state, minus);

            if (to_copy == NULL) {
                em_panic(state, "Cannot copy stack item: Stack tells us to retrieve item at -%d to copy but no such item exists", minus);
            }

            // Pop the offset info
            stack_pop(state);

            int ptr = stack_push(state);

            // Copy is a reference 
            if (is_code_using_managed_memory(to_copy->code)) {
                em_managed_ptr* reference = to_copy->u.v_mptr; 

                if (reference != state->null) {
                    em_add_reference(state, reference); // Stack holds a reference
                }

                state->stack[ptr].u.v_mptr = reference;
                state->stack[ptr].code = to_copy->code;

            } else {
                memcpy(&state->stack[ptr], to_copy, sizeof(em_stack_item));
            }
        }
        break;

        case 'q': 
        {
            // We need an offset at the top of the stack
            em_stack_item* offset = stack_top(state);

            if (offset == NULL) {
                em_panic(state, "Cannot pop beneath top: Need number at stack top");
            }

            if (!is_code_numeric(offset->code)) {
                em_panic(state, "Cannot pop beneath top: Item at stack top must be of numeric type");
            }

            uint32_t quantity = 0; 

            switch(offset->code) {
                case '1': quantity = offset->u.v_byte; break;
                case '2': quantity = offset->u.v_int16; break;
                case '4': quantity = offset->u.v_int32; break;
                case '8': quantity = offset->u.v_int64; break;
                default: 
                    em_panic(state, "Unhandled type %c in determining stack pop quantity", offset->code);
                break;
            }

            stack_pop(state);

            stack_pop_preserve_top(state, quantity);
        }
        break;

        default:
            em_panic(state, "Unknown stack instruction %c", current_code);
            return 0;
    }

    return 0;
}