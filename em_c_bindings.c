#include "eso_vm.h"
#include "em_c_bindings.h"
#include "eso_stack.h"

#include <stdio.h>

void stdio_prints(em_state* state) {
    em_stack_item* str = stack_top(state);

    if (str == NULL || str->code != 's') {
        em_panic("Inside C function", 0, 17, state, "Expected an s at stack top to perform print");
    }  

    printf("%s", str->u.v_mptr->raw);

    stack_pop(state);
}


void em_bind_c_default(em_state* state) {
    em_bind_c_call(state, "stdio.prints", stdio_prints);
}
