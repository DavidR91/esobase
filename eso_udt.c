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
                em_panic(code, index, len, state, "Expected a u at stack top to get field value from type");
            }  

            em_type_definition* definition = &state->types[of_type->type];

            int field_bytes_start = 0;
            int field_size = 0;
            char field_code = '?';
            bool found_field = false;

            // Find the field by name
            for(int i = 0; i < strlen(definition->types); i++) {
                field_code = definition->types[i];
                field_size = code_sizeof(field_code);

                if (strcmp(definition->field_names[i], name) == 0) {
                    found_field = true;
                    break;
                }

                field_bytes_start += code_sizeof(field_code);
            }

            if (!found_field) {
                em_panic(code, index, len, state, "No such field %s on type %s", name, definition->name);
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
                case '1':  
                {
                    int top = stack_push(state);
                    state->stack[top].signage = false;
                    state->stack[top].u.v_byte = *(uint8_t*)(of_type->u.v_ptr + field_bytes_start);
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
                case '8':  
                {
                    int top = stack_push(state);
                    state->stack[top].signage = false;
                    state->stack[top].u.v_int64 = *(uint64_t*)(of_type->u.v_ptr + field_bytes_start);
                    state->stack[top].code = field_code;
                }
                break;
                case 'f':  
                {
                    int top = stack_push(state);
                    state->stack[top].signage = false;
                    state->stack[top].u.v_float = *(float*)(of_type->u.v_ptr + field_bytes_start);
                    state->stack[top].code = field_code;
                }
                break;
                case 'd':  
                {
                    int top = stack_push(state);
                    state->stack[top].signage = false;
                    state->stack[top].u.v_double = *(double*)(of_type->u.v_ptr + field_bytes_start);
                    state->stack[top].code = field_code;
                }
                break;
                case 's': 
                {
                    int top = stack_push(state);
                    state->stack[top].signage = false;
                    state->stack[top].u.v_ptr = (void*)(*(const char**)(of_type->u.v_ptr + field_bytes_start));
                    state->stack[top].code = field_code;
                }
                break;
                default:
                em_panic(code, index, len, state, "Getting field %s of type %c not currently supported", name, field_code);
                break;
            }

        }
        return size_to_skip;

        // Set a field in a UDT from the stack
        case 's':
        {
            char* name = alloc_until(code, index+1, len, ';', true, &size_to_skip);

             if (name == NULL) {
                 em_panic(code, index, len, state, "Could not find a complete name for a field to set: Did you forget to terminate it?");
             }

            // Object of type must be below value to set on stack
            em_stack_item* of_type = stack_top_minus(state, 1);

            if (of_type == NULL || of_type->code != 'u') {
                em_panic(code, index, len, state, "Expected a u at stack top - 1 to set field value into type");
            }  

            em_type_definition* definition = &state->types[of_type->type];

            int field_bytes_start = 0;
            int field_size = 0;
            char field_code = '?';
            bool found_field = false;

            // Find the field by name
            for(int i = 0; i < strlen(definition->types); i++) {
                field_code = definition->types[i];
                field_size = code_sizeof(field_code);

                if (strcmp(definition->field_names[i], name) == 0) {
                    found_field = true;
                    break;
                }

                field_bytes_start += code_sizeof(field_code);
            }

            if (!found_field) {
                em_panic(code, index, len, state, "No such field %s on type %s", name, definition->name);
            }

            // Item on stack top must be the same type as the field we want to se
            em_stack_item* top = stack_top(state);

            if (top->code != field_code) {
                em_panic(code, index, len, state, "Setting field %s requires a value of type %c on top of the stack", name, field_code);
            }  

            switch(top->code) {
                case '?':
                *(bool*)(of_type->u.v_ptr + field_bytes_start) = top->u.v_bool;
                break;
                case '1': 
                *(uint8_t*)(of_type->u.v_ptr + field_bytes_start) = top->u.v_byte;
                break;
                case '2':  
                *(uint16_t*)(of_type->u.v_ptr + field_bytes_start) = top->u.v_int16;
                break;
                case '4':  
                *(uint32_t*)(of_type->u.v_ptr + field_bytes_start) = top->u.v_int32;
                break;
                case '8':  
                *(uint64_t*)(of_type->u.v_ptr + field_bytes_start) = top->u.v_int64;
                break;
                case 'f':  
                *(float*)(of_type->u.v_ptr + field_bytes_start) = top->u.v_float;
                break;
                case 'd':  
                *(double*)(of_type->u.v_ptr + field_bytes_start) = top->u.v_double;
                break;
                case 's':
                *(const char**)(of_type->u.v_ptr + field_bytes_start) = top->u.v_ptr;
                break;
                default: 
                em_panic(code, index, len, state, "Setting field %s of type %c not currently supported", name, field_code);
                break;
            }

            stack_pop(state);

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