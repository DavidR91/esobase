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

        // Create a label at the current location
        case 'l':
        {
            em_stack_item* name = stack_top(state);

            if (name == NULL || name->code != 's') {
                em_panic(state, "Creating label requires a string name at stack top");
            }

            if (name->u.v_mptr == state->null) {
                em_panic(state, "String name for creating label is NULL and cannot be used");
            }

            state->label_ptr++;

            if ((state->label_ptr + 1) >= state->max_labels) {
                em_panic(state, "Label overflow (%d maximum of %d)", state->label_ptr, state->max_labels);
            }

            char* label_name = (char*)name->u.v_mptr->raw;
            em_label* label = &state->labels[state->label_ptr];

            label->name = em_perma_alloc(state, strlen(label_name) + 1);
            memset(label->name, 0, strlen(label_name) + 1);
            memcpy(label->name, label_name, strlen(label_name));

            label->location = state->index+1;

            log_verbose("Labelled location %d '%s'\n",  label->location, label->name);

            stack_pop(state);
        }
        break;

         // Forward-declare a label at an explicit index
        case 'f':
        {
            em_stack_item* name = stack_top(state);

            if (name == NULL || name->code != 's') {
                em_panic(state, "Creating label requires a string name at stack top");
            }

            if (name->u.v_mptr == state->null) {
                em_panic(state, "String name for creating label is NULL and cannot be used");
            }

            em_stack_item* index = stack_top_minus(state, 1);

            if (index == NULL || index->code != '4') {
                em_panic(state, "Absolute location for a forward declared label must be a valid integer at stack top - 1");
            }

            state->label_ptr++;

            if ((state->label_ptr + 1) >= state->max_labels) {
                em_panic(state, "Label overflow (%d maximum of %d)", state->label_ptr, state->max_labels);
            }

            char* label_name = (char*)name->u.v_mptr->raw;
            em_label* label = &state->labels[state->label_ptr];

            label->name = em_perma_alloc(state, strlen(label_name) + 1);
            memset(label->name, 0, strlen(label_name) + 1);
            memcpy(label->name, label_name, strlen(label_name));

            label->location = index->u.v_int32;

            log_verbose("Labelled location %d '%s'\n",  label->location, label->name);

            stack_pop(state);

            stack_pop(state);
        }
        break;

        // Jump to location
        case 'j':
        {
            em_stack_item* location = stack_top(state);

            if (location == NULL || location->code != '^') {                
                em_panic(state, "Jump requires location (^) at stack top");
            }

            if (location->u.v_int32 < 0 || location->u.v_int32 >= state->len) {
                em_panic(state, "Location %d is invalid or out of bounds", location->u.v_int32);
            }

            state->index = location->u.v_int32 - 1;

        }
        break;

        // get a location by name
        case 'g':
        {
            em_stack_item* name = stack_top(state);

            if (name == NULL || name->code != 's') {
                em_panic(state, "Getting label location requires a string name at stack top");
            }

            if (name->u.v_mptr == state->null) {
                em_panic(state, "String name for getting label locationis NULL and cannot be used");
            }

            em_label* label = NULL;

            // Find label by name
            for(int i = 0; i <= state->label_ptr; i++) {
                if (strcmp(name->u.v_mptr->raw, state->labels[i].name) == 0) {
                    label = &state->labels[i];
                    break;
                }
            }

            // No such binding
            if (label == NULL) {
                em_panic(state, "No label name '%s'", name->u.v_mptr->raw);
            }

            stack_pop(state);

            int top = stack_push(state);
            
            state->stack[top].u.v_int32 = label->location;
            state->stack[top].code = '^';

        }   
        break;

        case 'i':
        {
            // Toggle 'if' mode on/off
            state->control_flow_if_flag = !state->control_flow_if_flag;
        }
        break;

        // Find the index relating to the line number at the top of the stack
        case '$':
        {
            em_stack_item* location = stack_top(state);

            if (location == NULL || location->code != '4') {
                em_panic(state, "Determining location from line number requires a valid integer at stack top");
            }

            bool found_line_number = false;
            uint32_t found_index = 0;
            uint32_t line = 1;

            for(int i = 0; i < state->len; i++) {
                if (state->code[i] == '\n') {
                    line++;

                    if (line == location->u.v_int32) {
                        found_index = i;
                        found_line_number = true;
                        break;
                    }
                }
            }

            if (!found_line_number) {
                 em_panic(state, "Did not find line %d while searching for index to create location: Max line reached was %d", location->u.v_int32, line);
            }

            stack_pop(state);

            int top = stack_push(state);
            
            state->stack[top].u.v_int32 = found_index;
            state->stack[top].code = '4';
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