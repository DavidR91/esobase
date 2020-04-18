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

int run_control(em_state* state) {

    char current_code = state->code[state->index];
    log_verbose("\033[0;31m%c\033[0;0m (Control flow)\n", current_code);
    log_ingestion(current_code);

    // The * marker has no meaning on its own so skip it
    if (current_code == state->control_flow_token) {
        return state->index;
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
            // Set the control flow symbol: Must be specified as an ASCII code
            em_stack_item* top = stack_top(state);

            if (top == NULL || top->code != '2') {
                em_panic(state, "Setting control flow symbol requires a 1 on top of the stack (ASCII code for new symbol)");
            }

            state->control_flow_token = top->u.v_byte;

            stack_pop(state);
        }
        return state->index+1;

        case '<': 
        {
            if (state->control_flow_if_flag) {
                // We only go backwards if a bool on top of the stack is true
                em_stack_item* condition = stack_top(state);

                if (condition == NULL) {
                    em_panic(state, "Operation rewind requires a ? on top the stack to check when if flag is set");
                } 

                if (condition->code != '?') {
                    em_panic(state, "Operation rewind requires a ? on top the stack (found %c) to check when if flag is set", condition->code);
                }

                if (!condition->u.v_bool) {
                    stack_pop(state);
                    break;
                }

                stack_pop(state);
            }

            // Go backwards
            for (int i = state->index; i >= 0; i--) {

                if (state->code[i] == state->control_flow_token) {
                    log_ingestion(state->code[i]);
                    log_verbose("Control flow backward jump to index %d\n", i);
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
                    em_panic(state, "Operation fast-forward requires a ? on top the stack to check when if flag is set");
                } 

                if (condition->code != '?') {
                    em_panic(state, "Operation fast-forward requires a ? on top the stack (found %c) to check when if flag is set", condition->code);
                }

                if (!condition->u.v_bool) {
                    stack_pop(state);
                    break;
                }

                stack_pop(state);
            }

            log_verbose("Control flow forward jump search starting at %d\n", index);

            // Go forwards
            for (int i = state->index; i < state->len; i++) {

                if (state->code[i] == state->control_flow_token) {
                    log_ingestion(state->code[i]);
                    log_verbose("Control flow forward jump to index %d\n", i);
                    return i;
                }
            }
        }
        break;

        default:
            em_panic(state, "Unknown control flow instruction %c", current_code);
            return 0;

    }

    return state->index;
}