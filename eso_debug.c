#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h> 
#include <stdbool.h>

#include "eso_vm.h"
#include "eso_stack.h"
#include "eso_log.h"
#include "eso_debug.h"
#include "eso_parse.h"

void print_memory_use(em_state* state) {

    log_printf("PERMANENT\t\tA:%10db\tP:%10db\n\n",
        state->memory_permanent.allocated,
        state->memory_permanent.peak_allocated);

    log_printf("PARSER   \t\tA:%10db\tP:%10db\n\n",
        state->memory_parser.allocated,
        state->memory_parser.peak_allocated);

    log_printf("USERCODE \t\tA:%10db\tP:%10db\t\n\t\t\tO:%10db\tP:%10db\n",
        state->memory_usercode.allocated,
        state->memory_usercode.peak_allocated,
        state->memory_usercode.overhead,
        state->memory_usercode.peak_overhead);
}

void assert_no_leak(em_state* state) {

    if (state->memory_usercode.allocated != 0) {
        log_printf( "\033[0;31m********\n\n\nUSERCODE MEMORY LEAK (%db)\n\n\n********\033[0;0m\n", state->memory_usercode.allocated);
        print_memory_use(state);
        exit(1);
    }

    log_printf("Leak check: OK\n");
}

void inspect_pointer(em_managed_ptr* ptr, int tab_start) {
    if (ptr->concrete_type == NULL) {

        for (int t = 0; t < tab_start; t++) {
            log_printf("\t");
        }

        log_printf("> (No type) %p (%db) refcount %d\n", 
            ptr->raw,
            ptr->size, 
            ptr->references);

        if (ptr->is_array) {

            log_printf("Array [%d] each element %db in size\n",                 
                ptr->size/ptr->array_element_size,
                ptr->array_element_size);

        } else {
            log_printf("Not an array\n");
        }

    } else {

        for (int t = 0; t < tab_start; t++) {
            log_printf("\t");
        }

        log_printf("\033[0;96m> %s (%db) refcount %d\t(%p)\033[0;0m\n", 
            ptr->concrete_type->name, 
            ptr->size, 
            ptr->references,
            ptr->raw);



        for(int field = 0; field < strlen(ptr->concrete_type->types); field++) {

            for (int t = 0; t < tab_start; t++) {
                log_printf("\t");
            }

            log_printf("%s|-- (%c) [+%-3db] %s\033[0m\n", code_colour_code(
                 ptr->concrete_type->types[field]),
                 ptr->concrete_type->types[field], 
                 ptr->concrete_type->start_offset_bytes[field], 
                 ptr->concrete_type->field_names[field]);
        
            if (is_code_using_managed_memory(ptr->concrete_type->types[field])) {
                em_managed_ptr** field_value = (em_managed_ptr**)(ptr->raw + ptr->concrete_type->start_offset_bytes[field]);

                if (*field_value == NULL) {
                    for (int t = 0; t < (tab_start + 1); t++) {
                        log_printf("\t");
                    }

                    log_printf("\033[0;31m UNINITIALIZED\033[0m\n");
        
                } else {
                    inspect_pointer(*field_value, tab_start+1);
                }
            }
        }

    }
}

int run_debug(em_state* state) {

    char current_code = tolower(state->code[state->index]);

    log_verbose("\033[0;31m%c\033[0;0m (Debug)\n", current_code);
    log_ingestion(current_code);

    switch(current_code) {

        // Stack
        case 's':
        dump_stack(state);
        return 0;

        // Trace
        case 't':
        dump_instructions(state);
        break;

        // Type map
        case 'u':
        dump_types(state);
        break;

        case 'r': 
        print_memory_use(state);
        break;

        // Inspect stack top
        case 'i':
        {
            em_stack_item* top = stack_top(state);

            if (top == NULL || !is_code_using_managed_memory(top->code)) {
                log_printf("Ignoring debug inspect request: Requires type of u, s or * on stack top\n");
                return 0;
            }

            inspect_pointer(top->u.v_mptr, 0);
        }
        break;

        // Assert line
        case 'l':
        {   
            int size_to_skip  = 0;
            char* test = alloc_until(state, state->code, state->index+1, state->len, ';', true, &size_to_skip);        

            if (test == NULL) {
                em_panic(state, "Could not find a complete literal for line assertion: Did you forget to terminate it?");
            }

            uint32_t v = atoi(test);
            em_parser_free(state, test);
            uint32_t real_line = calculate_file_line(state);

            if (real_line != v) {
                em_panic(state, "Line assertion failed: Expected to be on line %d actually on line %d", v, real_line);
            }

            return size_to_skip;
        }

        // Assert that the two stack items present contain equivalent values
        case 'a':
        {
            em_stack_item* one = stack_top_minus(state, 1);
            em_stack_item* two = stack_top(state);

            if (one == NULL || two == NULL) {
                em_panic(state, "Operation assert requires two items on the stack to compare");
            }

            if (one->code == 's' && two->code == 's') {
                // Do a string compare
                if (strcmp((const char*)one->u.v_mptr->raw, (const char*)two->u.v_mptr->raw) != 0) {
                    em_panic(state, "Assertion failed (string compare)");
                }
            } else {
                if (memcmp(&one->u, &two->u, sizeof(one->u)) != 0) {
                    em_panic(state, "Assertion failed (%p vs. %p)", &one->u, &two->u);
                }
            }

            stack_pop(state);
            stack_pop(state);
            
            log_verbose("Assertion successful\n");
        }
        break;

        default:
            em_panic(state, "Unknown debug instruction %c", current_code);
            return 0;

    }

    return 0;
}

void dump_stack_item(em_state* state, em_stack_item* item, int top_index) {

    const char* type_colour = code_colour_code(item->code);
    const char* reset_colour = "\033[0m";

    log_printf( "[%d]\t%s %c %s", top_index, type_colour, item->code, reset_colour);

    log_printf( "\t=\t%s", type_colour);

    switch(item->code) {
        case '?': log_printf( "%d", item->u.v_bool); break;
        case '1': log_printf( "%d", item->u.v_byte);break;
        case '2': log_printf( "%d", item->u.v_int16);break;
        case '4': log_printf( "%d", item->u.v_int32);break;
        case '8': log_printf( "%llu", item->u.v_int64);break;
        case 'f': log_printf( "%f", item->u.v_float);break;
        case 'd': log_printf( "%f", item->u.v_double);break;
        case 's': log_printf( "\"%s\\0\" %p length %d refcount %d", item->u.v_mptr->raw, item->u.v_mptr->raw, item->u.v_mptr->size, item->u.v_mptr->references);break;
        case '*':  
        {
            if (item->u.v_mptr->is_array) {
                log_printf( "%p length %d (array [%d] elements %db each) each refcount %d", 
                    item->u.v_mptr->raw, 
                    item->u.v_mptr->size, 
                    item->u.v_mptr->size / item->u.v_mptr->array_element_size,
                    item->u.v_mptr->array_element_size,
                    item->u.v_mptr->references); 
            } else {
                log_printf( "%p length %d refcount %d", 
                    item->u.v_mptr->raw, 
                    item->u.v_mptr->size, 
                    item->u.v_mptr->references); 
            }
        }
        break;
        case '^': break;
        case 'u': 
        {
            // Find relevant type
            em_type_definition* type = item->u.v_mptr->concrete_type;

            log_printf( "%s (%db) references %d\n", type->name, item->u.v_mptr->size, item->u.v_mptr->references);

            for(int field = 0; field < strlen(type->types); field++) {
                log_printf("%s\t\t\t |-- (%c) %s\033[0m\n", code_colour_code(type->types[field]), type->types[field], type->field_names[field]);
            }
        }
        break;
    }

    log_printf( "%s", reset_colour);
    log_printf( "\n");
}

void dump_stack(em_state* state) {
    log_printf( "=== Bottom of stack ===\n");

    int stack_start = 0;
    int stack_end = state->stack_ptr;

    if (stack_end > 32) {
        stack_start = stack_end - 32;
        log_printf( "[.... TRUNCATED ....]\n");
    }

    for (int i = stack_start; i <= stack_end; i++) {
        dump_stack_item(state, &state->stack[i], i - state->stack_ptr);
    }
    log_printf( "=== Top of stack ===\n");
}

void dump_types(em_state* state) {
    for(int i = 0; i <= state->type_ptr; i++) {

        em_type_definition* type = &state->types[i];
        log_printf("\033[0;96m%s\033[0m (%db in size)\n", type->name, type->size);

        for(int t = 0; t < strlen(type->types); t++) {
            log_printf("%s%c", code_colour_code(type->types[t]), type->types[t]);
        }
        log_printf("\033[0m\n\n");

         for(int t = 0; t < strlen(type->types); t++) {
            log_printf("%s%c\033[0m\t\033[0;96m%s\n\033[0m", code_colour_code(type->types[t]), type->types[t], type->field_names[t]);
        }
        log_printf("\n");
    }
}

void dump_instructions(em_state* state) {

    int lengthcontext = state->index + 32; 

    if (lengthcontext >= state->len) {
        lengthcontext = state->len;
    }

    int precontext = state->index - 32;

    if (precontext <= 0) {
        precontext = 0;
    }

    // Clamp the precontext so we start AFTER any newline
    for(int i = state->index; i >= precontext; i--) {
        if (state->code[i] == '\n') {
            precontext = i + 1;
            break;
        }
    }

    for(int i = state->index; i < lengthcontext; i++) {
        if (state->code[i] == '\n') {
            lengthcontext = i;
            break;
        }
    }

    const char* type_colour = NULL;

    log_printf( "\t");

    for (int i = precontext; i < lengthcontext; i++) {

        // Guess at top level codes
         switch (state->code[i]) {
            case 'm': 
            case 'l':
            case 's':
            case 'c':
            case 'd':
            case 'b':
                type_colour = "\033[0;36m"; 
            break;
            default: type_colour =  "\033[0m"; break;                
        }

        if (i == state->index) {
            type_colour = "\033[0;31m";
        }

        log_printf( "%s", type_colour);
        log_printf( "%c ", state->code[i]);
    }

    log_printf( "\n");
    log_printf( "\033[0m");
    log_printf( "\t");

    for (int i = precontext; i < lengthcontext; i++) {

        if (i == state->index) {
            log_printf( "%s", "\033[0;31m^^\033[0m");
            continue;
        }
        
        if (i == state->last_mode_change) {
            log_printf( "%s", "\033[0;31m| \033[0m");
            continue;
        }

        if (i > state->last_mode_change && i < state->index &&
            state->index >= precontext && state->index < lengthcontext &&
            state->last_mode_change >= precontext && state->last_mode_change < lengthcontext) {
            log_printf( "%s", "--");
            continue;
        }

        log_printf( "%s", "  ");       

    }

    uint32_t real_line = calculate_file_line(state);
    uint32_t real_column = calculate_file_column(state);

    log_printf( "\n\n(%s line %d col %d)\n", state->filename, real_line, real_column);
}

const char* code_colour_code(char code) {
     switch(code) {
        case '?': return "\033[0;93m"; 
        case '1': return "\033[0;33m"; 
        case '2': return "\033[0;32m"; 
        case '4': return "\033[0;32m"; 
        case '8': return "\033[0;32m"; 
        case 'f': return "\033[0;35m"; 
        case 'd': return "\033[0;35m"; 
        case 's': return "\033[0;31m"; 
        case '*': return "\033[0;31m"; 
        case '^': return "\033[0;31m"; 
        case 'u': return "\033[0;96m"; 
    }

    return "";
}

