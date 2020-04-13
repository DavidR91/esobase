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

void print_memory_use(em_state* state) {

    printf("PERMANENT\t\tA:%10db\tP:%10db\n\n",
        state->memory_permanent.allocated,
        state->memory_permanent.peak_allocated);

    printf("PARSER   \t\tA:%10db\tP:%10db\n\n",
        state->memory_parser.allocated,
        state->memory_parser.peak_allocated);

    printf("USERCODE \t\tA:%10db\tP:%10db\t\n\t\t\tO:%10db\tP:%10db\n",
        state->memory_usercode.allocated,
        state->memory_usercode.peak_allocated,
        state->memory_usercode.overhead,
        state->memory_usercode.peak_overhead);
}

void assert_no_leak(em_state* state) {

    if (state->memory_usercode.allocated != 0) {
        fprintf(stderr, "\033[0;31m********\n\n\nUSERCODE MEMORY LEAK (%db)\n\n\n********\033[0;0m\n", state->memory_usercode.allocated);
        print_memory_use(state);
        exit(1);
    }

    printf("Leak check: OK\n");
}

int run_debug(em_state* state, const char* code, int index, int len) {

    char current_code = tolower(code[index]);

    log_verbose("\033[0;31m%c\033[0;0m (Debug)\n", current_code);

    switch(current_code) {

        // Stack
        case 's':
        dump_stack(state);
        return 0;

        // Trace
        case 't':
        dump_instructions(code, index, len, state);
        break;

        // Type map
        case 'u':
        dump_types(state);
        break;

        case 'r': 
        print_memory_use(state);
        break;

        // Assert that the two stack items present contain equivalent values
        case 'a':
        {
            em_stack_item* one = stack_top_minus(state, 1);
            em_stack_item* two = stack_top(state);

            if (one == NULL || two == NULL) {
                em_panic(code, index, len, state, "Operation assert requires two items on the stack to compare");
            }

            if (one->code == 's' && two->code == 's') {
                // Do a string compare
                if (strcmp((const char*)one->u.v_mptr->raw, (const char*)two->u.v_mptr->raw) != 0) {
                    em_panic(code, index, len, state, "Assertion failed (string compare)");
                }
            } else {
                if (memcmp(&one->u, &two->u, sizeof(one->u)) != 0) {
                    em_panic(code, index, len, state, "Assertion failed (%p vs. %p)", &one->u, &two->u);
                }
            }

            stack_pop(state);
            stack_pop(state);
            
            log_verbose("Assertion successful\n");
        }
        break;

        case 'p':
        dump_pointers(state);
        return 0;

        default:
            em_panic(code, index, len, state, "Unknown debug instruction %c", current_code);
            return 0;

    }

    return 0;
}


void dump_stack_item(em_state* state, em_stack_item* item, int top_index) {

    const char* type_colour = code_colour_code(item->code);
    const char* reset_colour = "\033[0m";

    fprintf(stdout, "[%d]\t%s %c %s", top_index, type_colour, item->code, reset_colour);

    fprintf(stdout, "\t=\t%s", type_colour);

    switch(item->code) {
        case '?': fprintf(stdout, "%d", item->u.v_bool); break;
        case '1': fprintf(stdout, "%d", item->u.v_byte);break;
        case '2': fprintf(stdout, "%d", item->u.v_int16);break;
        case '4': fprintf(stdout, "%d", item->u.v_int32);break;
        case '8': fprintf(stdout, "%llu", item->u.v_int64);break;
        case 'f': fprintf(stdout, "%f", item->u.v_float);break;
        case 'd': fprintf(stdout, "%f", item->u.v_double);break;
        case 's': fprintf(stdout, "\"%s\\0\" %p length %d refcount %d", item->u.v_mptr->raw, item->u.v_mptr->raw, item->u.v_mptr->size, item->u.v_mptr->references);break;
        case '*': fprintf(stdout, "%p length %d refcount %d", item->u.v_mptr->raw, item->u.v_mptr->size, item->u.v_mptr->references); break;
        case '^': break;
        case 'u': 
        {
            // Find relevant type
            em_type_definition* type = item->u.v_mptr->concrete_type;

            fprintf(stdout, "%s (%db) references %d", type->name, item->u.v_mptr->size, item->u.v_mptr->references);
        }
        break;
    }

    fprintf(stdout, "%s", reset_colour);
    fprintf(stdout, "\n");
}

void dump_pointers(em_state* state) {
    
    // TODO Reachability
}

void dump_stack(em_state* state) {
    fprintf(stdout, "=== Bottom of stack ===\n");

    int stack_start = 0;
    int stack_end = state->stack_ptr;

    if (stack_end > 32) {
        stack_start = stack_end - 32;
        fprintf(stdout, "[.... TRUNCATED ....]\n");
    }

    for (int i = stack_start; i <= stack_end; i++) {
        dump_stack_item(state, &state->stack[i], i - state->stack_ptr);
    }
    fprintf(stdout, "=== Top of stack ===\n");
}

void dump_types(em_state* state) {
    for(int i = 0; i <= state->type_ptr; i++) {

        em_type_definition* type = &state->types[i];
        printf("\033[0;96m%s\033[0m (%db in size)\n", type->name, type->size);

        for(int t = 0; t < strlen(type->types); t++) {
            printf("%s%c", code_colour_code(type->types[t]), type->types[t]);
        }
        printf("\033[0m\n\n");

         for(int t = 0; t < strlen(type->types); t++) {
            printf("%s%c\033[0m\t\033[0;96m%s\n\033[0m", code_colour_code(type->types[t]), type->types[t], type->field_names[t]);
        }
        printf("\n");
    }
}

void dump_instructions(const char* code, int index, int len, em_state* state) {

    int lengthcontext = index + 32; 

    if (lengthcontext >= len) {
        lengthcontext = len;
    }

    int precontext = index - 32;

    if (precontext <= 0) {
        precontext = 0;
    }

    // Clamp the precontext so we start AFTER any newline
    for(int i = index; i >= precontext; i--) {
        if (code[i] == '\n') {
            precontext = i + 1;
            break;
        }
    }

    for(int i = index; i < lengthcontext; i++) {
        if (code[i] == '\n') {
            lengthcontext = i;
            break;
        }
    }

    const char* type_colour = NULL;

    fprintf(stdout, "\t");

    for (int i = precontext; i < lengthcontext; i++) {

        // Guess at top level codes
         switch (code[i]) {
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

        if (i == index) {
            type_colour = "\033[0;31m";
        }

        fprintf(stdout, "%s", type_colour);
        fprintf(stdout, "%c ", code[i]);
    }

    fprintf(stdout, "\n");
    fprintf(stdout, "\033[0m");
    fprintf(stdout, "\t");

    for (int i = precontext; i < lengthcontext; i++) {

        if (i == index) {
            fprintf(stdout, "%s", "\033[0;31m^^\033[0m");
            continue;
        }
        
        if (i == state->last_mode_change) {
            fprintf(stdout, "%s", "\033[0;31m| \033[0m");
            continue;
        }

        if (i > state->last_mode_change && i < index &&
            index >= precontext && index < lengthcontext &&
            state->last_mode_change >= precontext && state->last_mode_change < lengthcontext) {
            fprintf(stdout, "%s", "--");
            continue;
        }

        fprintf(stdout, "%s", "  ");       

    }

    fprintf(stdout, "\n\n(%s line %d col %d)\n", state->filename, (state->file_line+1),index + 1);
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

