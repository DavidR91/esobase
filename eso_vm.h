#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

typedef enum {
    EM_BOOLEAN,
    EM_MEMORY, 
    EM_LITERAL,
    EM_STACK,
    EM_CC,
    EM_DEBUG,
    EM_UDT

} ESOMODE;

typedef struct {
    char code;
    bool signage;
    uint32_t size;
    union {
        bool v_bool;
        uint8_t v_byte;
        uint16_t v_int16;
        uint16_t v_int32;
        uint64_t v_int64;
        float v_float;
        double v_double;
        void* v_ptr;
    } u;
    uint16_t type;
} em_stack_item;

typedef struct {
    const char* name;
    char* types;
    char** field_names;
    int size; // total size in bytes of the fields inside
} em_type_definition;

typedef struct {
    em_type_definition* types;
    int type_ptr;
    int max_types;

    em_stack_item* stack;
    int stack_ptr;
    int stack_size;
    int last_mode_change;
    bool signed_flag;
    int file_line;
    const char* filename;

} em_state;

void em_panic(const char* code, int index, int len, em_state* state, const char* format, ...);
size_t code_sizeof(char code);
char safe_get(const char* code, int index, int len);