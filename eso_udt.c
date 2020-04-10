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

int run_udt(em_state* state, const char* code, int index, int len) {
   
    int size_to_skip = 0;

    char current_code = tolower(code[index]);
    log_verbose("DEBUG VERBOSE\t\tLiteral start '%c'\n", current_code);

    switch(current_code) {

        // Create
        case 'c':
        { 
            char* name = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (name == NULL) {
                em_panic(code, index, len, state, "Could not find a complete name for creating type: Did you forget to terminate it?");
            }

            em_type_definition* definition = NULL;

            for(int t = 0; t <= state->type_ptr; t++) {
                if (strcmp(state->types[t].name, name) == 0) {
                    definition = &state->types[t];
                    break;
                }
            }

            if (definition == NULL) {
                em_panic(code, index, len, state, "Could not find type '%s' to create", name);
            }

            int top = stack_push(state);
            state->stack[top].signage = false;
            state->stack[top].code = 'u';
            state->stack[top].type = 0;
            state->stack[top].size = definition->size;
            state->stack[top].u.v_ptr = malloc(definition->size);
            memset(state->stack[top].u.v_ptr, 0, definition->size);
        }
        break;

        // Get a field from a UDT and push it onto the stack
        case 'g':
        {
            char* name = alloc_until(code, index+1, len, ';', true, &size_to_skip);

             if (name == NULL) {
                 em_panic(code, index, len, state, "Could not find a complete name for a field to retrieve: Did you forget to terminate it?");
             }

            // Object of type must be on stack top
            em_stack_item* of_type = stack_top(state);

            if (of_type == NULL || of_type->code != 'u') {
                em_panic(code, index, len, state, "Expected a u at stack top to get field value");
            }  

            em_type_definition* definition = &state->types[of_type->type];

            int field_bytes_start = 0;
            int field_size = 0;
            char field_code = '?';

            // Find the field by name
            for(int i = 0; i < strlen(definition->types); i++) {
                field_code = definition->types[i];
                field_size = code_sizeof(field_code);

                if (strcmp(definition->field_names[i], name) == 0) {
                    break;
                }

                field_bytes_start += code_sizeof(field_code);
            }

            switch(field_code) {
                case '?':  
                {
                    int top = stack_push(state);
                    state->stack[top].signage = false;
                    state->stack[top].u.v_bool = *(bool*)(of_type->u.v_ptr + field_bytes_start);
                    state->stack[top].code = field_code;
                }
                break;
                case '2':  
                {
                    int top = stack_push(state);
                    state->stack[top].signage = false;
                    state->stack[top].u.v_int16 = *(uint16_t*)(of_type->u.v_ptr + field_bytes_start);
                    state->stack[top].code = field_code;
                }
                break;
                case '4':  
                {
                    int top = stack_push(state);
                    state->stack[top].signage = false;
                    state->stack[top].u.v_int32 = *(uint32_t*)(of_type->u.v_ptr + field_bytes_start);
                    state->stack[top].code = field_code;
                }
                break;
            }

        }
        return size_to_skip;

        // Define
        case 'd': 
        {
            em_stack_item* name = stack_top(state);
            em_stack_item* field_qty = stack_top_minus(state, 1);

            if (name == NULL || name->code != 's') {
                em_panic(code, index, len, state, "Expected an s at stack top to define the name of the type");
            }  

            if (field_qty == NULL || field_qty->code != '4') {
                em_panic(code, index, len, state, "Expected a 4 at stack top - 1 to define the quantity of fields for the type");
            }

            if (field_qty->u.v_int32 <= 0) {
                em_panic(code, index, len, state, "Not allowed to declare type %s with no fields", name->u.v_ptr);
            }

            state->type_ptr++;

            em_type_definition* new_type = &state->types[state->type_ptr];
            new_type->name = name->u.v_ptr;
            new_type->types = malloc(field_qty->u.v_int32 + 1);
            memset(new_type->types, 0, field_qty->u.v_int32 + 1);

            new_type->field_names = malloc(sizeof(char*) * field_qty->u.v_int32);
            memset(new_type->field_names, 0, sizeof(char*) * field_qty->u.v_int32);

            // We need a TYPE and NAME for each field
            int minus = 1 + (field_qty->u.v_int32 * 2);

            // Where we write the type code per field
            int type_code_ptr = 0;

            for (int field = 1; field <= field_qty->u.v_int32; field++) {
                em_stack_item* field_name = stack_top_minus(state, minus);

                if (field_name == NULL || field_name->code != 's') {
                    em_panic(code, index, len, state, "Expected an s at stack top - %d to define the name of field %d of %s", minus, field, name->u.v_ptr);
                }

                new_type->field_names[field-1] = field_name->u.v_ptr;
                minus--;

                em_stack_item* field_type = stack_top_minus(state, minus);

                if (field_qty == NULL) {
                    em_panic(code, index, len, state, "Expected an item of any type at stack top - %d to define the type of field %d of %s", minus, field, name->u.v_ptr);
                }

                new_type->types[type_code_ptr] = field_type->code;
                type_code_ptr++;
                minus--;
            }

            // Calculate total size 
            for(int i = 0; i < field_qty->u.v_int32; i++) {
                new_type->size += code_sizeof(new_type->types[i]);
            }

            stack_pop_by(state, 2 + (field_qty->u.v_int32 * 2));
        }
        return size_to_skip;


        default:
            em_panic(code, index, len, state, "Unknown UDT instruction %c", current_code);
            return 0;
    }

    return size_to_skip;
}
