#include "eso_vm.h"
#include "eso_c.h"
#include "eso_log.h"
#include "eso_stack.h"
#include "eso_parse.h"

#include <string.h>
#include <stdio.h>

int run_c(em_state* state) {

    char current_code = tolower(state->code[state->index]);
    log_verbose("\033[0;31m%c\033[0;0m (C interop)\n", current_code);
    log_ingestion(current_code);

    int size_to_skip  = 0;
    
    switch(current_code) {

        // Call a global function name
        case 'c':
        {
            em_stack_item* str = stack_top(state);

            if (str == NULL || str->code != 's') {
                em_panic(state, "Expected an s at stack top to call C function (name)");
            }  

            em_c_binding* binding = NULL;

            // Find call by name
            for(int i = 0; i <= state->c_binding_ptr; i++) {
                if (strcmp(str->u.v_mptr->raw, state->c_bindings[i].name) == 0) {
                    binding = &state->c_bindings[i];
                    break;
                }
            }

            // No such binding
            if (binding == NULL) {
                em_panic(state, "No such C function bound '%s'", str->u.v_mptr->raw);
            }

            stack_pop(state);

            log_verbose("CALL INTO C FUNCTION %s @ %p\n", binding->name, binding->bound);

            binding->bound(state);

            log_verbose("FINISHED CALL INTO C FUNCTION %s @ %p\n", binding->name, binding->bound);

        }
        return size_to_skip;

        default:
            em_panic(state, "Unknown C interop instruction %c", current_code);
            return size_to_skip;

    }

    return size_to_skip;
}