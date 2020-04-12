#include "eso_vm.h"
#include "eso_log.h"
#include "eso_parse.h"
#include "eso_stack.h"
#include "eso_log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h> 
#include <stdbool.h>

int run_boolean(em_state* state, const char* code, int index, int len) {

    char current_code = tolower(code[index]);

    switch(current_code) {
        // add
        case '+': 
        {
            em_stack_item* one = stack_top_minus(state, 1);
            em_stack_item* two = stack_top(state);

            if (one == NULL || two == NULL) {
                em_panic(code, index, len, state, "Operation add requires two items on the stack to add");
            }

            if (!is_stack_item_numeric(one) || !is_stack_item_numeric(two)) {
                em_panic(code, index, len, state, 
                    "Operation add requires two arguments of numeric type. Got %c and %c", one->code, two->code);
            }

            stack_pop(state);
            stack_pop(state);

            switch(one->code) {
                case '1': 
                    switch(two->code) { 
                        case '1':  {            
                            uint8_t op = one->u.v_byte + two->u.v_byte;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '1';
                            state->stack[ptr].u.v_byte = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot add a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break; 
                
                case '2': 
                    switch(two->code) { 
                        case '2':  {    
                            uint16_t op = one->u.v_int16 + two->u.v_int16;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '2';
                            state->stack[ptr].u.v_int16 = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot add a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break;
                
                case '4': 
                    switch(two->code) { 
                        case '4':  {                            
                            uint32_t op = one->u.v_int32 + two->u.v_int32;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '4';
                            state->stack[ptr].u.v_int32 = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot add a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break;
                
                case '8': 
                    switch(two->code) { 
                        case '8':  {                            
                            uint64_t op =one->u.v_int64 + two->u.v_int64;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '8';
                            state->stack[ptr].u.v_int64 = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot add a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break;
                
                case 'f': 
                    switch(two->code) { 
                        case 'f':  {                            
                            float op = one->u.v_float + two->u.v_float;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = 'f';
                            state->stack[ptr].u.v_float = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot add a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }   
                break;

                case 'd': 
                    switch(two->code) { 
                        case 'd':  {                            
                            double op = one->u.v_double + two->u.v_double;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = 'd';
                            state->stack[ptr].u.v_double = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot add a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }   
                break;
            }

        }
        break;

        // and
        case 'a': 
        {
            em_stack_item* one = stack_top_minus(state, 1);
            em_stack_item* two = stack_top(state);

            if (one == NULL || two == NULL) {
                em_panic(code, index, len, state, "Operation and requires two items on the stack");
            }

            if (one->code != '?' || two->code != '?') {
                em_panic(code, index, len, state, 
                    "Operation and requires two arguments of ? (boolean) type. Got %c and %c", one->code, two->code);
            }

            stack_pop(state);
            stack_pop(state);

            bool op = one->u.v_bool && two->u.v_bool;

            int ptr = stack_push(state);
            state->stack[ptr].code = '?';
            state->stack[ptr].u.v_bool = op;
            state->stack[ptr].size = 0;
        }
        break;

        // or
        case 'o': 
        {
            em_stack_item* one = stack_top_minus(state, 1);
            em_stack_item* two = stack_top(state);

            if (one == NULL || two == NULL) {
                em_panic(code, index, len, state, "Operation or requires two items on the stack");
            }

            if (one->code != '?' || two->code != '?') {
                em_panic(code, index, len, state, 
                    "Operation or requires two arguments of ? (boolean) type. Got %c and %c", one->code, two->code);
            }

            stack_pop(state);
            stack_pop(state);

            bool op = one->u.v_bool || two->u.v_bool;

            int ptr = stack_push(state);
            state->stack[ptr].code = '?';
            state->stack[ptr].u.v_bool = op;
            state->stack[ptr].size = 0;
        }
        break;

        // not
        case '!': 
        {
            em_stack_item* one = stack_top(state);
          
            if (one == NULL) {
                em_panic(code, index, len, state, "Operation not requires an item on top of the stack");
            }

            if (one->code != '?') {
                em_panic(code, index, len, state, 
                    "Operation not requires one argument of ? (boolean) type. Got %c", one->code);
            }

            stack_pop(state);

            bool op = !one->u.v_bool;

            int ptr = stack_push(state);
            state->stack[ptr].code = '?';
            state->stack[ptr].u.v_bool = op;
            state->stack[ptr].size = 0;
        }
        break;

        // subtract
        case '-': break;

        // multiply
        case '*': break;

        // divide
        case '/': break;

        // mod
        case '%': break;

        // logical not
        case '~': break;

        // and
        case '&': break;

        // less than
        case '<': 
        {
            em_stack_item* one = stack_top_minus(state, 1);
            em_stack_item* two = stack_top(state);

            if (one == NULL || two == NULL) {
                em_panic(code, index, len, state, "Operation less than requires two items on the stack to compare");
            }

            if (!is_stack_item_numeric(one) || !is_stack_item_numeric(two)) {
                em_panic(code, index, len, state, 
                    "Operation less than requires two arguments of numeric type. Got %c and %c", one->code, two->code);
            }

            stack_pop(state);
            stack_pop(state);

            switch(one->code) {
                case '1': 
                    switch(two->code) { 
                        case '1':  {            
                            bool op = one->u.v_byte < two->u.v_byte;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '?';
                            state->stack[ptr].u.v_bool = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot compare a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break; 
                
                case '2': 
                    switch(two->code) { 
                        case '2':  {    
                            bool op = one->u.v_int16 < two->u.v_int16;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '?';
                            state->stack[ptr].u.v_bool = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot compare a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break;
                
                case '4': 
                    switch(two->code) { 
                        case '4':  {                            
                            bool op = one->u.v_int32 < two->u.v_int32;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '?';
                            state->stack[ptr].u.v_bool = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot compare a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break;
                
                case '8': 
                    switch(two->code) { 
                        case '8':  {                            
                            bool op =one->u.v_int64 < two->u.v_int64;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '?';
                            state->stack[ptr].u.v_bool = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot compare a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break;
                
                case 'f': 
                    switch(two->code) { 
                        case 'f':  {                            
                            bool op = one->u.v_float < two->u.v_float;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '?';
                            state->stack[ptr].u.v_bool = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot compare a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }   
                break;

                case 'd': 
                    switch(two->code) { 
                        case 'd':  {                            
                            bool op = one->u.v_double < two->u.v_double;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '?';
                            state->stack[ptr].u.v_bool = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot compare a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }   
                break;
            }
        }
        break;

        // greater than
        case '>': 
        {
            em_stack_item* one = stack_top_minus(state, 1);
            em_stack_item* two = stack_top(state);

            if (one == NULL || two == NULL) {
                em_panic(code, index, len, state, "Operation greater than requires two items on the stack to compare");
            }

            if (!is_stack_item_numeric(one) || !is_stack_item_numeric(two)) {
                em_panic(code, index, len, state, 
                    "Operation greater than requires two arguments of numeric type. Got %c and %c", one->code, two->code);
            }

            stack_pop(state);
            stack_pop(state);

            switch(one->code) {
                case '1': 
                    switch(two->code) { 
                        case '1':  {            
                            bool op = one->u.v_byte > two->u.v_byte;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '?';
                            state->stack[ptr].u.v_bool = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot compare a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break; 
                
                case '2': 
                    switch(two->code) { 
                        case '2':  {    
                            bool op = one->u.v_int16 > two->u.v_int16;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '?';
                            state->stack[ptr].u.v_bool = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot compare a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break;
                
                case '4': 
                    switch(two->code) { 
                        case '4':  {                            
                            bool op = one->u.v_int32 > two->u.v_int32;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '?';
                            state->stack[ptr].u.v_bool = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot compare a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break;
                
                case '8': 
                    switch(two->code) { 
                        case '8':  {                            
                            bool op =one->u.v_int64 > two->u.v_int64;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '?';
                            state->stack[ptr].u.v_bool = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot compare a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break;
                
                case 'f': 
                    switch(two->code) { 
                        case 'f':  {                            
                            bool op = one->u.v_float > two->u.v_float;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '?';
                            state->stack[ptr].u.v_bool = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot compare a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }   
                break;

                case 'd': 
                    switch(two->code) { 
                        case 'd':  {                            
                            bool op = one->u.v_double > two->u.v_double;
                            int ptr = stack_push(state);
                            state->stack[ptr].code = '?';
                            state->stack[ptr].u.v_bool = op;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot compare a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }   
                break;
            }
        }
        break;

        default:
            em_panic(code, index, len, state, "Unknown boolean instruction %c", current_code);
            return 0;

    }

    return 0;

}