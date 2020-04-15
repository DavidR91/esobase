#include <stdint.h>
#include <stdio.h>
#include "eso_align.h"
#include "eso_stack.h"
#include "eso_vm.h"

#define ALIGNED_BY_HOST

uint32_t align_type_size(char code) {

    #ifdef ALIGNED_BY_HOST
        switch(code) {
            case '?': return _Alignof(bool);
            case '1': return _Alignof(uint8_t);
            case '2': return _Alignof(uint16_t);
            case '4': return _Alignof(uint32_t);
            case '8': return _Alignof(uint64_t);
            case 'f': return _Alignof(float);
            case 'd': return _Alignof(double);
            case 's': return _Alignof(void*);
            case '*': return _Alignof(void*);
            case 'u': return _Alignof(void*);

            default:
                return 1;
        }
    #else

        switch(code) {
            case '?': 
            case '1': 
                return 1;
            case '2': 
                return 2;
            case '4': 
                return 4;
            case '8': 
                return 8;
            case 'f': 
                return 4;
            case 'd': 
                return 8;
            case 's': 
            case '*': 
            case 'u': 
                return 8;
            default:
                return 1;
        }
    #endif

}

uint32_t calculate_padding(char code, int starting_byte) {
    uint32_t align = align_type_size(code);
    uint32_t prepadding = 0;

    if (starting_byte % align == 0) {
        return 0;
    } else {
        while(starting_byte % align != 0) {
            starting_byte++;
            prepadding++;
        }
    }

    return prepadding;
} 

uint32_t calculate_padding_overall(int current_size) {
    uint32_t additional_bytes = 0;

    while ((current_size + additional_bytes) % 4 != 0) { // Struct size alignment <------
        additional_bytes++;
    }

    return current_size + additional_bytes;
}

// Assuming the stack is in the form
// FIELD NAME
// FIELD TYPE
//
// iterate through each field and calculate a total size assuming each field gets
// appropriately padded
uint32_t calculate_aligned_struct_size(em_state* state, em_stack_item* field_qty) {

    // We need a TYPE and NAME for each field
    int minus = 1 + (field_qty->u.v_int32 * 2);

    uint32_t byte = 0;

    for (int field = 1; field <= field_qty->u.v_int32; field++) {

        minus--;

        em_stack_item* field_type = stack_top_minus(state, minus);

        if (field_qty == NULL) {
            return -1;
        }

        uint32_t padding = calculate_padding(field_type->code, byte);

        minus--;
        byte += (padding + code_sizeof(field_type->code));
    }

    return calculate_padding_overall(byte);
}


