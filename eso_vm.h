#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stddef.h>

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
    int* start_offset_bytes;
    int size; // total size in bytes of the fields inside
} em_type_definition;

typedef struct {
    void* raw;
    uint32_t size;
    uint16_t references; // strong references
    em_type_definition* concrete_type;

    bool is_array;
    uint32_t array_element_size;
    char array_element_code;
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

    // Currently allocated
    uint32_t allocated;
    uint32_t peak_allocated; 

    uint32_t overhead;
    uint32_t peak_overhead;

} em_memory_use;

struct t_em_c_binding;
typedef struct t_em_c_binding em_c_binding;

typedef struct em_state_forward {
    em_type_definition* types;
    int type_ptr;
    int max_types;

    em_stack_item* stack;
    int stack_ptr;
    int stack_size;
    int last_mode_change;
    const char* filename;
    bool control_flow_if_flag;
    uint8_t control_flow_token;

    em_c_binding* c_bindings;
    int max_c_bindings;
    int c_binding_ptr;

    em_memory_use memory_permanent;
    em_memory_use memory_usercode;
    em_memory_use memory_parser;

    ESOMODE mode;

    const char* code; 
    int len;
    int index;

    em_managed_ptr* null;

} em_state;

typedef void (*em_c_call) (em_state* state);

struct t_em_c_binding {

    char* name;
    em_c_call bound;

};

em_state* create_state();
void destroy_state(em_state* state);

void em_panic(em_state* state, const char* format, ...);
size_t code_sizeof(char code);
bool is_code_numeric(char code);
bool is_code_using_managed_memory(char code);
char safe_get(const char* code, int index, int len);

em_type_definition* create_new_type(em_state* state);
em_managed_ptr* create_managed_ptr(em_state* state);
void free_managed_ptr(em_state* state, em_managed_ptr* mptr);

void* em_perma_alloc(em_state* state, size_t size);

// Allocations that should not live forever that are temporary values
// for parsing
void* em_parser_alloc(em_state* state, size_t size);
void em_parser_free(em_state* state, void* ptr);

// Allocations that should NOT live forever
void* em_usercode_alloc(em_state* state, size_t size, bool bookkeep_as_overhead);
void em_usercode_free(em_state* state, void* ptr, size_t size, bool bookkeep_as_overhead);

// This is just for bookkeeping to say usercode now owns something originally owned
// by the parser
void em_transfer_alloc_parser_usercode(em_state* state, size_t size);

uint32_t calculate_file_line(em_state* state);
uint32_t calculate_file_column(em_state* state);

void em_bind_c_call(em_state* state, char* name, em_c_call call);

void em_add_reference(em_state* state, em_managed_ptr* mptr);