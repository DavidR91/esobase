#pragma once
#include "eso_vm.h"

int stack_push(em_state* state);

em_stack_item* stack_pop(em_state* state);

em_stack_item* stack_pop_by(em_state* state, int amount);

em_stack_item* stack_top_minus(em_state* state, int minus);

em_stack_item* stack_top(em_state* state);

bool is_stack_item_numeric(em_stack_item* item);

int run_stack(em_state* state, const char* code, int index, int len);