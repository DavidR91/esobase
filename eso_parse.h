#pragma once

char* alloc_until(em_state* state, const char* code, int index, int len, char terminator, bool eat_whitespace, int* size_to_skip);