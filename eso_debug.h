#pragma once

void assert_no_leak(em_state* state);

int run_debug(em_state* state);

void dump_stack_item(em_state* state, em_stack_item* item, int relative_index, int top_index);

void dump_stack(em_state* state);

void dump_types(em_state* state);

void dump_instructions(em_state* state);

const char* code_colour_code(char code);

void dump_pointers(em_state* state);