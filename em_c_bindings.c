#include "eso_vm.h"
#include "em_c_bindings.h"
#include "eso_stack.h"
#include "eso_debug.h"

#include <stdio.h>

void stdio_prints(em_state* state) {
    em_stack_item* str = stack_top(state);

    if (str == NULL || str->code != 's') {
        em_panic(state, "Expected an s at stack top to perform print");
    }  

    printf("%s", str->u.v_mptr->raw);

    stack_pop(state);
}

void debug_leakcheck(em_state* state) {
    assert_no_leak(state);
}

void stdio_print_bytes(em_state* state) {
     em_stack_item* bytes = stack_top(state);

    if (bytes == NULL || !is_code_using_managed_memory(bytes->code)) {
        em_panic(state, "Expected an s u or * at stack top to print bytes");
    }  

    printf("%db = ", bytes->u.v_mptr->size);

    for (int i = 0; i < bytes->u.v_mptr->size; i++) {
        printf("%d", (int)*(char*)(bytes->u.v_mptr->raw + i));

        if (i != bytes->u.v_mptr->size - 1) {
            printf(",");
        }
    }

    printf("\n");
}

void em_bind_c_default(em_state* state) {
    em_bind_c_call(state, "stdio.prints", stdio_prints);
    em_bind_c_call(state, "stdio.print_bytes", stdio_print_bytes);
    em_bind_c_call(state, "debug.assert_no_leak", debug_leakcheck);
}
