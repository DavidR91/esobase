#include "eso_controlflow.h"
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

int run_control(em_state* state, const char* code, int index, int len) {

    // The * marker has no meaning on its own so skip it
    if (code[index] == '@') {
        return index;
    }

    switch(code[index]) {
        case 'i':
        {
            // Toggle 'if' mode on/off
            state->control_flow_if_flag = !state->control_flow_if_flag;
        }
        break;

        case '<': 
        {
            if (state->control_flow_if_flag) {
                // We only go backwards if a bool on top of the stack is true
                em_stack_item* condition = stack_top(state);

                if (condition == NULL) {
                    em_panic(code, index, len, state, "Operation rewind requires a ? on top the stack to check when if flag is set");
                } 

                if (condition->code != '?') {
                    em_panic(code, index, len, state, "Operation rewind requires a ? on top the stack (found %c) to check when if flag is set", condition->code);
                }

                if (!condition->u.v_bool) {
                    stack_pop(state);
                    break;
                }

                stack_pop(state);
            }

            // Go backwards
            for (int i = index; i >= 0; i--) {
                if (code[i] == '@') {
                    return i;
                }
            }
        }
        break;

        case '>':
        {
            if (state->control_flow_if_flag) {
                em_stack_item* condition = stack_top(state);

                if (condition == NULL) {
                    em_panic(code, index, len, state, "Operation fast-forward requires a ? on top the stack to check when if flag is set");
                } 

                if (condition->code != '?') {
                    em_panic(code, index, len, state, "Operation fast-forward requires a ? on top the stack (found %c) to check when if flag is set", condition->code);
                }

                if (!condition->u.v_bool) {
                    stack_pop(state);
                    break;
                }

                stack_pop(state);
            }
            
            // Go forwards
            for (int i = index; i < len; i++) {
                if (code[i] == '@') {
                    return i;
                }
            }
        }
        break;
    }

    return index;
}