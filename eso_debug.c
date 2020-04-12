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

int run_debug(em_state* state, const char* code, int index, int len) {

    char current_code = tolower(code[index]);
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
            
            log_verbose("DEBUG VERBOSE\t\tAssertion successful\n");
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
        case 's': fprintf(stdout, "\"%s\\0\" %p length %d", item->u.v_mptr->raw, item->u.v_mptr->raw, item->u.v_mptr->size);break;
        case '*': fprintf(stdout, "%p length %d", item->u.v_mptr->raw, item->u.v_mptr->size); break;
        case '^': break;
        case 'u': 
        {
            // Find relevant type
            em_type_definition* type = item->u.v_mptr->concrete_type;

            fprintf(stdout, "%s (%db)", type->name, item->u.v_mptr->size);
        }
        break;
    }

    fprintf(stdout, "%s", reset_colour);
    fprintf(stdout, "\n");
}

void dump_pointers(em_state* state) {
    for(int i = 0; i <= state->pointer_ptr; i++) {
        em_managed_ptr* ptr = &state->pointers[i];

        if (ptr->dead)
        {
            printf("%2d) *** DEAD *** \n", i);
        }
        else {
            printf("%2d) Alive \n", i);
        }

        printf("\traw = %p size = %db\n", ptr->raw, ptr->size);
        printf("\tfree on pop = %d type = %p\n", ptr->free_on_stack_pop, ptr->concrete_type);

        if (ptr->concrete_type != NULL) {
            printf("\t\033[0;96m%s (%s)\033[0m size = %db\n", ptr->concrete_type->name, ptr->concrete_type->types, ptr->concrete_type->size);
        }

        printf("\n");
    }
}

void dump_stack(em_state* state) {
    fprintf(stdout, "=== Bottom of stack ===\n");
    for (int i = 0; i <= state->stack_ptr; i++) {
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

