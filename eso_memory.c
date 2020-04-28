#include "eso_vm.h"
#include "eso_log.h"
#include "eso_debug.h"
#include "eso_parse.h"
#include "eso_stack.h"
#include "eso_log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h> 
#include <stdbool.h>

int run_memory(em_state* state) {

    char current_code = tolower(state->code[state->index]);
    log_verbose("\033[0;31m%c\033[0;0m (Memory)\n", current_code);
    log_ingestion(current_code);
    
    switch(current_code) {

        // allocate flat block (as opposed to an array)
        case 'x':
        {
            // We need an int on the stack
            em_stack_item* top = stack_top(state);

            if (top == NULL) {
                em_panic(state, "Insufficient arguments to memory allocation: requires integer size on stack top");
            }

            uint32_t real_size = 0;

            switch(top->code) {
                case '1': real_size = top->u.v_byte; break;
                case '2': real_size = top->u.v_int16; break;
                case '4': real_size = top->u.v_int32; break;
                case '8': real_size = top->u.v_int64; break;
                default: em_panic(state, "Memory allocation requires integer number on top of stack - found %c\n", top->code);
            }

            void* arb = em_usercode_alloc(state, real_size, false);
            memset(arb, 0, real_size);

            // Create a managed pointer
            em_managed_ptr* mptr = create_managed_ptr(state);
            mptr->size = real_size;
            mptr->raw = arb;
            mptr->concrete_type = NULL;
            em_add_reference(state, mptr); // Stack holds a reference

            stack_pop(state);

            int ptr = stack_push(state);
            state->stack[ptr].code = '*';
            state->stack[ptr].u.v_mptr = mptr;

            log_verbose("Allocated %db of memory @ %p\n", real_size, arb);

            return 0;
        }

        // allocate an array 
        // - size 
        // - type code
        case 'a':
        {
            em_stack_item* type_code_stack = stack_top(state);

            if (type_code_stack == NULL || type_code_stack->code != '1') {
                em_panic(state, "Insufficient arguments to array allocation: Expected a type code (1248fdsu*) at stack top");
            }

            em_stack_item* count_stack = stack_top_minus(state, 1);

            if (count_stack == NULL) {
                em_panic(state, "Insufficient arguments to array allocation: Expected an array size specified as 1, 2, 4 or 8 at stack top - 1");
            }

            uint32_t element_size = 0;
            uint32_t element_count = 0;

            element_size = code_sizeof(type_code_stack->u.v_byte);

            if (element_size == 0) {
                em_panic(state, "Cannot construct array of unknown type '%c' (%x)", type_code_stack->u.v_byte,  type_code_stack->u.v_byte);
            }

            switch(count_stack->code) {
                case '1': element_count = count_stack->u.v_byte; break;
                case '2': element_count = count_stack->u.v_int16; break;
                case '4': element_count = count_stack->u.v_int32; break;
                case '8': element_count = count_stack->u.v_int64; break;
                default: em_panic(state, "Array allocation requires integer number on stack top -1. Found %c\n", count_stack->code);
            }

            uint32_t real_size = element_size * element_count;

            void* arb = em_usercode_alloc(state, real_size, false);
            memset(arb, 0, real_size);

            // Create a managed pointer
            em_managed_ptr* mptr = create_managed_ptr(state);
            mptr->size = real_size;
            mptr->raw = arb;
            mptr->concrete_type = NULL;
            em_add_reference(state, mptr); // Stack holds a reference

            // We're an array
            mptr->is_array = true;
            mptr->array_element_size = element_size;
            mptr->array_element_code = type_code_stack->u.v_byte;

            if (is_code_using_managed_memory(mptr->array_element_code)) {
                for(uint32_t i= 0; i < element_count; i++) {
                    (((em_managed_ptr**) arb)[i]) = state->null;
                }
            }

            stack_pop(state);
            stack_pop(state);

            int ptr = stack_push(state);
            state->stack[ptr].code = '*';
            state->stack[ptr].u.v_mptr = mptr;

            log_verbose("Allocated %db of memory @ %p as array\n", real_size, arb);
        }
        return 0;

        // Length in bytes
        case 'l':
        {
            em_stack_item* top = stack_top(state);

            if (top == NULL) {
                em_panic(state, "Insufficient arguments to memory length: requires item of any type on stack top");
            }

            int stack_item = stack_push(state);

            if (is_code_using_managed_memory(top->code)) {
                state->stack[stack_item].code = '4';
                state->stack[stack_item].u.v_int32 = top->u.v_mptr->size;
            } else {
                state->stack[stack_item].code = '4';
                state->stack[stack_item].u.v_int32 = code_sizeof(top->code);
            }

            return 0;
        }

        // Elements count based on a type size
        case 'e':
        {
            em_stack_item* top = stack_top(state);

            if (top == NULL) {
                em_panic(state, "Insufficient arguments to array elements count: requires array at top of stack");
            }

            if (top->code != '*' || !top->u.v_mptr->is_array) {
                em_panic(state, "Array element count requires a * of array type on stack top");
            }
            
            int stack_item = stack_push(state);
            state->stack[stack_item].code = '4';
            state->stack[stack_item].u.v_int32 =  top->u.v_mptr->size / top->u.v_mptr->array_element_size;

            return 0;
        }
        return 0;

        // Source, source offset bytes, count bytes, Destnation, destination offset bytes
        case 'c': 
        {
            em_stack_item* source = stack_top_minus(state, 4);
            em_stack_item* source_offset = stack_top_minus(state, 3);
            em_stack_item* byte_count = stack_top_minus(state, 2);
            em_stack_item* destination = stack_top_minus(state, 1);
            em_stack_item* destination_offset = stack_top(state);

            if (source == NULL || !is_code_using_managed_memory(source->code)) {
                em_panic(state, "Memory copy requires source at stack-4 of code s, u or *");
            }

            // Don't allow on arrays
            if (source->u.v_mptr->is_array) {
                em_panic(state, "Arbitrary copy source cannot be an array");
            }

            if (source_offset == NULL || source_offset->code != '4') {
                em_panic(state, "Memory copy requires source offset bytes at stack-3 of code 4");
            }

            if (byte_count == NULL || byte_count->code != '4') {
                em_panic(state, "Memory copy requires byte copy quantity at stack-2 of code 4");
            }

            if (destination == NULL || !is_code_using_managed_memory(destination->code)) {
                em_panic(state, "Memory copy requires destination at stack-1 of code s, u or *");
            }

            if (destination->u.v_mptr->is_array) {
                em_panic(state, "Arbitrary copy destination cannot be an array");
            }

            if (destination_offset == NULL || destination_offset->code != '4') {
                em_panic(state, "Memory copy requires destination offset bytes at stack top");
            }

            // Is source offset in bounds?
            int src_offset = source_offset->u.v_int32;
            int count = byte_count->u.v_int32;
            int dest_offset = destination_offset->u.v_int32;
            int src_size = source->u.v_mptr->size;
            int dest_size = destination->u.v_mptr->size;

            if (source->code == 's' && (src_offset >= src_size - 1 || src_offset + count > src_size-1)) {
                em_panic(state, "Memory copy source offset +%db for allocation of %db involves a string null terminator which is not permitted", src_offset, src_size);
            }

            if (destination->code == 's' && (dest_offset >= dest_size - 1 || dest_offset + count > dest_size - 1)) {
                em_panic(state, "Memory copy destination offset +%db for allocation of %db involves a string null terminator which is not permitted", dest_offset, dest_size);
            }

            if (src_offset < 0 || src_offset >= src_size) {
                em_panic(state, "Memory copy source offset +%db is out of bounds for source of size %db", src_offset, src_size);
            }

            if (src_offset + count > src_size) {
                em_panic(state, "Memory copy offset %db is valid in source but there are not %db bytes available to copy (source is only %db in length)", src_offset, count, src_size);
            }

            if (dest_offset < 0 || dest_offset >= dest_size) {
                em_panic(state, "Memory copy destination offset +%db is out of bounds for destination of size %db", dest_offset, dest_size);
            }

            if (dest_offset + count > dest_size) {
                em_panic(state, "Memory copy offset %db is valid in destination but there are not %db bytes space to copy into (destination is only %db in length)", dest_offset, count, dest_size);
            }

            // We're done all we can: Hit it
            memcpy(destination->u.v_mptr->raw + dest_offset, source->u.v_mptr->raw + src_offset, count);

            // If this is a string ensure the null terminator remains
            if (destination->code == 's') {
                *((char*) destination->u.v_mptr->raw + (destination->u.v_mptr->size - 1)) = 0;
            }

            for(int i = 1; i <= 5; i++) {
                stack_pop(state);
            }

            return 0;
        }

        // Set at byte offset
        case 's': 
        {
            em_stack_item* destination = stack_top_minus(state, 2);
            em_stack_item* destination_offset = stack_top_minus(state, 1);
            
            if (destination == NULL || !is_code_using_managed_memory(destination->code)) {
                em_panic(state, "Memory set offset requires destination at stack-2 of code s, u or *");
            }

            if (destination_offset == NULL || destination_offset->code != '4') {
                em_panic(state, "Memory set offset requires offset bytes at stack-1 of code 4");
            }

            int dest_offset = destination_offset->u.v_int32;
            int dest_size = destination->u.v_mptr->size;

            if (destination->u.v_mptr->is_array) {

                int array_index = dest_offset;
                int array_size = dest_size / destination->u.v_mptr->array_element_size;

                em_stack_item* source = stack_top(state);

                if (source == NULL || source->code != destination->u.v_mptr->array_element_code) {
                    em_panic(state, "Memory set offset requires a value of type %c at stack top to set into array of type %c",
                        destination->u.v_mptr->array_element_code, destination->u.v_mptr->array_element_code);
                }

                 if (array_index < 0 || array_index >= array_size) {
                    em_panic(state, "Array index %d out of bounds for array of size [%d] (%db)", array_index, array_size, dest_size);
                }

                switch(destination->u.v_mptr->array_element_code) {
                    case '1': 
                        (((uint8_t*) destination->u.v_mptr->raw)[array_index]) = source->u.v_byte;
                    break;
                    case '2':
                        (((uint16_t*) destination->u.v_mptr->raw)[array_index]) = source->u.v_int16;
                    break;
                    case '4':
                        (((uint32_t*) destination->u.v_mptr->raw)[array_index]) = source->u.v_int32;
                    break;
                    case '8':
                        (((uint64_t*) destination->u.v_mptr->raw)[array_index]) = source->u.v_int64;  
                    break;
                    case 'f': 
                        (((float*) destination->u.v_mptr->raw)[array_index]) = source->u.v_float;
                    break;
                    case 'd': 
                        (((double*) destination->u.v_mptr->raw)[array_index]) = source->u.v_double;
                    break;

                    case 'u':
                    case '*':
                    case 's':                        
                    break;
                }

            } else { 

                em_stack_item* byte = stack_top(state);

                if (byte == NULL || byte->code != '1') {
                    em_panic(state, "Memory set byte offset requires a byte value (1) at stack top");
                }

                if (destination->code == 's' && dest_offset == dest_size - 1) {
                    em_panic(state, "Memory set destination offset +%db attempts to change the null terminator of a string which is not permitted. The total allocation is %db in length but last valid string byte is at +%db", dest_offset, dest_size, dest_size - 2);
                }

                if (dest_offset < 0 || dest_offset >= dest_size) {
                    em_panic(state, "Memory set destination offset +%db is out of bounds for allocation of size %db", dest_offset, dest_size);
                }

                *(char*) (destination->u.v_mptr->raw + destination_offset->u.v_int32) = byte->u.v_byte;
            }

            for(int i = 1; i <= 2; i++) {
                stack_pop(state);
            } 

        }
        return 0;

        // Set at byte offset
        case 'g': 
        {
            em_stack_item* destination = stack_top_minus(state, 1);
            em_stack_item* destination_offset = stack_top(state);

            if (destination == NULL || !is_code_using_managed_memory(destination->code)) {
                em_panic(state, "Memory get offset requires destination at stack-1 of code s, u or *");
            }

            if (destination_offset == NULL || destination_offset->code != '4') {
                em_panic(state, "Memory get offset requires offset bytes at stack top of code 4");
            }

            int dest_offset = destination_offset->u.v_int32;
            int dest_size = destination->u.v_mptr->size;

            if (destination->u.v_mptr->is_array) {

                int array_index = dest_offset;
                int array_size = dest_size / destination->u.v_mptr->array_element_size;

                 if (array_index < 0 || array_index >= array_size) {
                    em_panic(state, "Array index %d out of bounds for array of size [%d] (%db)", array_index, array_size, dest_size);
                }

                int stack_item = stack_push(state);
                state->stack[stack_item].code = destination->u.v_mptr->array_element_code;
               
                switch(destination->u.v_mptr->array_element_code) {
                    case '1': 
                        state->stack[stack_item].u.v_byte = (((uint8_t*) destination->u.v_mptr->raw)[array_index]);
                    break;
                    case '2':  
                        state->stack[stack_item].u.v_int16 = (((uint16_t*) destination->u.v_mptr->raw)[array_index]);
                    break;
                    case '4':  
                        state->stack[stack_item].u.v_int32 = (((uint32_t*) destination->u.v_mptr->raw)[array_index]);
                    break;
                    case '8':  
                        state->stack[stack_item].u.v_int64 = (((uint64_t*) destination->u.v_mptr->raw)[array_index]);
                    break;
                    case 'f': 
                        state->stack[stack_item].u.v_float = (((float*) destination->u.v_mptr->raw)[array_index]);
                    break;
                    case 'd': 
                        state->stack[stack_item].u.v_double = (((double*) destination->u.v_mptr->raw)[array_index]);
                    break;

                    case 'u':
                    case '*':
                    case 's':
                        state->stack[stack_item].u.v_mptr = (((em_managed_ptr**) destination->u.v_mptr->raw)[array_index]);
                    break;
                }

            } else {

                if (dest_offset < 0 || dest_offset >= dest_size) {
                    em_panic(state, "Memory get destination offset +%db is out of bounds for allocation of size %db", dest_offset, dest_size);
                }

                int stack_item = stack_push(state);
                state->stack[stack_item].code = '1';
                state->stack[stack_item].u.v_byte = *(char*) (destination->u.v_mptr->raw + destination_offset->u.v_int32);

            }

            stack_pop_preserve_top(state, 1);
        }
        return 0;

        default:
            em_panic(state, "Unknown memory instruction %c", current_code);
            return 0;

    }
}