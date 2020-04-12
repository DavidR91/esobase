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
    EM_UDT,
    EM_CONTROL_FLOW

} ESOMODE;

typedef struct {
    char* name;
    char* types;
    char** field_names;
    int size; // total size in bytes of the fields inside
} em_type_definition;

typedef struct {
    void* raw;
    uint32_t size;
    bool free_on_stack_pop;
    em_type_definition* concrete_type;
    bool dead;
} em_managed_ptr;

typedef struct {
    char code;
    union {
        bool v_bool;
        uint8_t v_byte;
        uint16_t v_int16;
        uint16_t v_int32;
        uint64_t v_int64;
        float v_float;
        double v_double;
        em_managed_ptr* v_mptr;
    } u;
} em_stack_item;

typedef struct {
    em_type_definition* types;
    int type_ptr;
    int max_types;

    em_managed_ptr* pointers;
    int pointer_ptr;
    int max_pointers; // Obviously needs to be changed to grow

    em_stack_item* stack;
    int stack_ptr;
    int stack_size;
    int last_mode_change;
    int file_line;
    const char* filename;
    bool control_flow_if_flag;
    uint8_t control_flow_token;
} em_state;

void em_panic(const char* code, int index, int len, em_state* state, const char* format, ...);
size_t code_sizeof(char code);
bool is_code_numeric(char code);
bool is_coding_using_managed_memory(char code);
char safe_get(const char* code, int index, int len);

em_type_definition* create_new_type(em_state* state);
em_managed_ptr* create_managed_ptr(em_state* state);
void free_managed_ptr(const char* code, int index, int len, em_state* state, em_managed_ptr* mptr);

void* em_perma_alloc(size_t size);

// Allocations that should not live forever that are temporary values
// for parsing
void* em_parser_alloc(size_t size);
void em_parser_free(void* ptr);

// Allocations that should NOT live forever
void* em_usercode_alloc(size_t size);
void em_usercode_free(void* ptr, size_t size);

