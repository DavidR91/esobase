#include "eso_vm.h"
#include "em_c_bindings.h"
#include "eso_stack.h"
#include "eso_debug.h"

#include <stdio.h>
#include <string.h>

void stdio_prints(em_state* state) {
    em_stack_item* str = stack_top(state);

    if (str == NULL || str->code != 's') {
        em_panic(state, "Expected an s at stack top to perform print");
    }  

    if (str->u.v_mptr == state->null) {
        printf("NULL");
    } else {
        printf("%s", str->u.v_mptr->raw);
    }
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

    if (bytes->u.v_mptr == state->null) {
        printf("NULL\n");
        return;
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

void string_cat(em_state* state) {

     em_stack_item* a = stack_top_minus(state, 1);
     em_stack_item* b = stack_top(state);

     if (a == NULL || a->code != 's' || b == NULL || b->code != 's') {
        em_panic(state, "Expected two strings (s) to concatenate");
     }

    int a_nonul_size = (a->u.v_mptr->size - 1);
    int b_nonul_size = (b->u.v_mptr->size - 1);

    em_managed_ptr* mptr = create_managed_ptr(state);

    mptr->size =  a_nonul_size + b_nonul_size + 1; 
    mptr->raw = em_usercode_alloc(state, mptr->size, false);
    mptr->concrete_type = NULL;
    em_add_reference(state, mptr); // Stack holds a reference
    memset(mptr->raw, 0, mptr->size);

    memcpy(mptr->raw, a->u.v_mptr->raw, a_nonul_size);
    memcpy(mptr->raw + a_nonul_size, b->u.v_mptr->raw, b_nonul_size);

    stack_pop(state);
    stack_pop(state);

    int top = stack_push(state);
    state->stack[top].code = 's';
    state->stack[top].u.v_mptr = mptr; 
}

void em_bind_c_default(em_state* state) {
    em_bind_c_call(state, "stdio.prints", stdio_prints);
    em_bind_c_call(state, "stdio.print_bytes", stdio_print_bytes);
    em_bind_c_call(state, "debug.assert_no_leak", debug_leakcheck);
    em_bind_c_call(state, "string.cat", string_cat);
}
