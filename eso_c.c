#include "eso_vm.h"
#include "eso_c.h"
#include "eso_log.h"
#include "eso_stack.h"
#include "eso_parse.h"

#include <string.h>
#include <stdio.h>

int run_c(em_state* state, const char* code, int index, int len) {

    char current_code = tolower(code[index]);
    log_verbose("\033[0;31m%c\033[0;0m (C interop)\n", current_code);
    log_ingestion(current_code);

    int size_to_skip  = 0;
    
    switch(current_code) {

        // Call a global function name
        case 'c':
        {
            char* name = alloc_until(state, code, index+1, len, ';', true, &size_to_skip);

            if (name == NULL) {
                em_panic(code, index, len, state, "Could not find a complete name for C function to call: Did you forget to terminate the string?");
            }

            em_c_binding* binding = NULL;

            // Find call by name
            for(int i = 0; i <= state->c_binding_ptr; i++) {
                if (strcmp(name, state->c_bindings[i].name) == 0) {
                    binding = &state->c_bindings[i];
                    break;
                }
            }

            // No such binding
            if (binding == NULL) {
                em_panic(code, index, len, state, "No such C function bound '%s'", name);
            }

            log_verbose("CALL INTO C FUNCTION %s @ %p\n", binding->name, binding->bound);

            binding->bound(state);

            log_verbose("FINISHED CALL INTO C FUNCTION %s @ %p\n", binding->name, binding->bound);

            em_parser_free(state, name);
        }
        return size_to_skip;

        default:
            em_panic(code, index, len, state, "Unknown C interop instruction %c", current_code);
            return size_to_skip;

    }

    return size_to_skip;
}