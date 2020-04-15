#pragma once
#include "eso_stack.h"

uint32_t align_type_size(char code);

uint32_t calculate_padding(char code, int starting_byte);

uint32_t calculate_aligned_struct_size(em_state* state, em_stack_item* field_qty);