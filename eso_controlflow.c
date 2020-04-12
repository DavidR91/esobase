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

    char current_code = code[index];
    log_verbose("\033[0;31m%c\033[0;0m (Control flow)\n", current_code);

    // The * marker has no meaning on its own so skip it
    if (current_code == state->control_flow_token) {
        return index;
    }

    switch(current_code) {
        case 'i':
        {
            // Toggle 'if' mode on/off
            state->control_flow_if_flag = !state->control_flow_if_flag;
        }
        break;

        case 's': 
        {
            // Set the control flow symbol
            state->control_flow_token = safe_get(code, index+1, len);
        }
        return index+1;

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
                if (code[i] == state->control_flow_token) {
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
                if (code[i] == state->control_flow_token) {
                    return i;
                }
            }
        }
        break;

        default:
            em_panic(code, index, len, state, "Unknown control flow instruction %c", current_code);
            return 0;

    }

    return index;
}