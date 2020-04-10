// TODO
// String tables
// if and loops
// callable functions (?)
// user defined types

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h> 
#include <stdbool.h>

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
void run_file(const char* file);
void run(const char* filename, const char* i, int len);
void dump_instructions(const char* code, int index, int len, em_state* state);
void dump_stack(em_state* state);

int main(int argc, char** argv) {

    if (argc < 2) {
        fprintf(stderr, "At least one argument required\n");
        exit(0);
    }

    run_file(argv[1]);

    return 0;
}

void run_file(const char* file) {
    FILE* input = fopen(file, "r");

    if (input == NULL) {
        fprintf(stderr, "Cannot find or open %s\n", file);
        exit(1);
        return;
    }

    fseek(input, 0, SEEK_END);

    int length = ftell(input);

    rewind(input);

    char* file_content = malloc(length+1);
    memset(file_content, 0, length+1);

    if (fread(file_content, 1, length, input) != length) {
        fprintf(stderr, "Could not successfully read the entire length of %s\n", file);
        exit(1);
    }

    file_content[length] = 0;

    run(file, file_content, strlen(file_content));

    fclose(input);
}

void em_panic(const char* code, int index, int len, em_state* state, const char* format, ...) {
    va_list argptr;
    va_start(argptr, format);
    fprintf(stderr, "\n%s\n", "\033[0;31m* * * * PANIC * * * *\n\n");
    vfprintf(stderr, format, argptr);

    fprintf(stderr, "\n\n");
    dump_instructions(code, index, len, state);
    fprintf(stderr, "%s", "\033[0m\n\n");
    dump_stack(state);
    fprintf(stderr, "\n");
    va_end(argptr);
    abort();
}

int stack_push(em_state* state) {
    state->stack_ptr++;
    return state->stack_ptr;
}

// int stack_top_ptr(em_state* state) {
//     if (state->stack_ptr <= 0) {
//         return 0;
//     } else {
//         return state->stack_ptr - 1;
//     }
// }

size_t code_sizeof(char code) {
    switch(code) {
        case '?': return sizeof(bool); break;
        case '1': return sizeof(uint8_t); break;
        case '2': return sizeof(uint16_t); break;
        case '4': return sizeof(uint32_t); break;
        case '8': return sizeof(uint64_t); break;
        case 'f': return sizeof(float); break;
        case 'd': return sizeof(double); break;
        case 's': return sizeof(char*); break;
        case '*': return sizeof(void*); break;
        case '^': return sizeof(void*); break;
    }
    return 0;
}

em_stack_item* stack_pop(em_state* state) {
    if (state->stack_ptr < 0) {
        return NULL;
    } else {
        em_stack_item* top = &state->stack[state->stack_ptr];

        state->stack_ptr--;

        return top;
    }
}

em_stack_item* stack_pop_by(em_state* state, int amount) {
    if (state->stack_ptr < 0) {
        return NULL;
    } else {
        em_stack_item* top = &state->stack[state->stack_ptr];

        state->stack_ptr -= amount;

        return top;
    }
}


em_stack_item* stack_top_minus(em_state* state, int minus) {
    if (state->stack_ptr - minus < 0) {
        return NULL;
    } else {
        return &state->stack[state->stack_ptr - minus];
    }
}

em_stack_item* stack_top(em_state* state) {
    if (state->stack_ptr < 0) {
        return NULL;
    } else {
        return &state->stack[state->stack_ptr];
    }
}

bool is_stack_item_numeric(em_stack_item* item) {

    switch(item->code) {
        case '1': 
        case '2': 
        case '4': 
        case '8': 
        case 'f':
        case 'd':
            return true;
        default:
            return false;
    }
}

// void after_stack_push(em_state* state) {
//     state->stack_ptr++;
// }

char safe_get(const char* code, int index, int len) {
    if (index < 0 || index >= len) {
        abort();
    }

    return code[index];
}

void log_verbose(const char* format, ...) {
    va_list argptr;
    va_start(argptr, format);
   // vfprintf(stderr, format, argptr);
    va_end(argptr);
}

// Return everything in code until the specified terminator is hit.
// The caller should resume from *resume in order to skip the eaten 
// text and the terminator. NULL returned if terminator not found before
// len
char* alloc_until(const char* code, int index, int len, char terminator, bool eat_whitespace, int* size_to_skip) {

    int total_skip = 0;
    int starting_index = index;

    while(eat_whitespace && isspace(code[starting_index])) {
        total_skip++;
        starting_index++;
    }

    int bytes_consumed = 0;

    for (int i = starting_index ; i < len; i++) {
        bytes_consumed++;
        total_skip++;
        if (code[i] == terminator) {
            
            int build_length = bytes_consumed + 1;
            char* built = malloc(build_length);

            memset(built, 0, build_length);
            memcpy(built, code+starting_index, i - starting_index);

            built[build_length-1] = 0;

            if (size_to_skip != NULL) {
                if (i + 1 < len) {
                    *size_to_skip = total_skip;
                } else {
                    *size_to_skip = -1;
                }
            }

            return built;
        }
    }

    if (size_to_skip != NULL) {
        *size_to_skip = 0;
    }
    return NULL;
}

const char* code_colour_code(char code) {
     switch(code) {
        case '?': return "\033[0;36m"; 
        case '1': return "\033[0;33m"; 
        case '2': return "\033[0;32m"; 
        case '4': return "\033[0;32m"; 
        case '8': return "\033[0;32m"; 
        case 'f': return "\033[0;35m"; 
        case 'd': return "\033[0;35m"; 
        case 's': return "\033[0;31m"; 
        case '*': return "\033[0;31m"; 
        case '^': return "\033[0;31m"; 
    }

    return "";
}

int run_udt(em_state* state, const char* code, int index, int len) {
   
    int size_to_skip = 0;

    char current_code = tolower(code[index]);
    log_verbose("DEBUG VERBOSE\t\tLiteral start '%c'\n", current_code);

    switch(current_code) {
        case 'd': 
        {
            em_stack_item* name = stack_top(state);
            em_stack_item* field_qty = stack_top_minus(state, 1);

            if (name == NULL || name->code != 's') {
                em_panic(code, index, len, state, "Expected an s at stack top to define the name of the type");
            }  

            if (field_qty == NULL || field_qty->code != '4') {
                em_panic(code, index, len, state, "Expected a 4 at stack top - 1 to define the quantity of fields for the type");
            }

            if (field_qty->u.v_int32 <= 0) {
                em_panic(code, index, len, state, "Not allowed to declare type %s with no fields", name->u.v_ptr);
            }

            state->type_ptr++;

            em_type_definition* new_type = &state->types[state->type_ptr];
            new_type->name = name->u.v_ptr;
            new_type->types = malloc(field_qty->u.v_int32 + 1);
            memset(new_type->types, 0, field_qty->u.v_int32 + 1);

            new_type->field_names = malloc(sizeof(char*) * field_qty->u.v_int32);
            memset(new_type->field_names, 0, sizeof(char*) * field_qty->u.v_int32);

            // We need a TYPE and NAME for each field
            int minus = 1 + (field_qty->u.v_int32 * 2);

            // Where we write the type code per field
            int type_code_ptr = 0;

            for (int field = 1; field <= field_qty->u.v_int32; field++) {
                em_stack_item* field_name = stack_top_minus(state, minus);

                if (field_name == NULL || field_name->code != 's') {
                    em_panic(code, index, len, state, "Expected an s at stack top - %d to define the name of field %d of %s", minus, field, name->u.v_ptr);
                }

                new_type->field_names[field-1] = field_name->u.v_ptr;
                minus--;

                em_stack_item* field_type = stack_top_minus(state, minus);

                if (field_qty == NULL) {
                    em_panic(code, index, len, state, "Expected an item of any type at stack top - %d to define the type of field %d of %s", minus, field, name->u.v_ptr);
                }

                new_type->types[type_code_ptr] = field_type->code;
                type_code_ptr++;
                minus--;
            }

            // Calculate total size 
            for(int i = 0; i < field_qty->u.v_int32; i++) {
                new_type->size += code_sizeof(new_type->types[i]);
            }

            stack_pop_by(state, 2 + (field_qty->u.v_int32 * 2));
        }
        return size_to_skip;


        default:
            em_panic(code, index, len, state, "Unknown UDT instruction %c", current_code);
            return 0;
    }

    return size_to_skip;
}

int run_literal(em_state* state, const char* code, int index, int len) {
   
    int size_to_skip = 0;

    char current_code = tolower(code[index]);
    log_verbose("DEBUG VERBOSE\t\tLiteral start '%c'\n", current_code);

    switch(current_code) {
        case '+': 
            state->signed_flag = !state->signed_flag;
            log_verbose("DEBUG VERBOSE\t\tSigned flag now %d\n", state->signed_flag);
            return 0;

        case '?': 
        {
            int top = stack_push(state);
            state->stack[top].signage = false;
            
            char v = tolower(safe_get(code, index+1, len));

            state->stack[top].u.v_bool = (v == 'y' || v == 't');
            state->stack[top].code = current_code;

            log_verbose("DEBUG VERBOSE\t\tPush bool literal %c\n", v);
        }
        return 1;

        case '1': 
        {
            char* test = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(code, index, len, state, "Could not find a complete literal for byte: Did you forget to terminate it?");
            }

            uint8_t v = atoi(test);
            free(test);

            int top = stack_push(state);
            state->stack[top].signage = state->signed_flag;
            state->stack[top].code = current_code;
            state->stack[top].u.v_byte = v;

            log_verbose("DEBUG VERBOSE\t\tPush u8 literal %d\n", v);
        }
        return size_to_skip;

        case '2': 
        {
            char* test = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(code, index, len, state, "Could not find a complete literal for int16: Did you forget to terminate it?");
            }

            uint16_t v = atoi(test);
            free(test);

            int top = stack_push(state);
            state->stack[top].signage = state->signed_flag;
            state->stack[top].code = current_code;
            state->stack[top].u.v_int32 = v;

            log_verbose("DEBUG VERBOSE\t\tPush u16 literal %d\n", v);
        }
        return size_to_skip;

        case '4': 
        {
            char* test = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(code, index, len, state, "Could not find a complete literal for int32: Did you forget to terminate it?");
            }

            uint32_t v = atoi(test);
            free(test);

            int top = stack_push(state);
            state->stack[top].signage = state->signed_flag;
            state->stack[top].code = current_code;
            state->stack[top].u.v_int32 = v;

            log_verbose("DEBUG VERBOSE\t\tPush u32 literal %d\n", v);
        }
        return size_to_skip;

        case '8': 
        {
            char* test = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(code, index, len, state, "Could not find a complete literal for int64: Did you forget to terminate it?");
            }

            uint64_t v = strtol(test, NULL, 10);
            free(test);

            int top = stack_push(state);
            state->stack[top].signage = state->signed_flag;
            state->stack[top].code = current_code;
            state->stack[top].u.v_int64 = v;

            log_verbose("DEBUG VERBOSE\t\tPush u64 literal %llu\n", v);
        }
        return size_to_skip;

        case 'f':  
        {
            char* test = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(code, index, len, state, "Could not find a complete literal for float32: Did you forget to terminate it?");
            }

            float v = strtod(test, NULL);
            free(test);

            int top = stack_push(state);
            state->stack[top].signage = state->signed_flag;
            state->stack[top].code = current_code;
            state->stack[top].u.v_float = v;

            log_verbose("DEBUG VERBOSE\t\tPush float literal %f\n", v);
        }
        return size_to_skip;

        case 'd':  
        {
            char* test = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (test == NULL) {
                em_panic(code, index, len, state, "Could not find a complete literal for float64: Did you forget to terminate it?");
            }

            double v = strtod(test, NULL);
            free(test);

            int top = stack_push(state);
            state->stack[top].signage = state->signed_flag;
            state->stack[top].code = current_code;
            state->stack[top].u.v_double = v;

            log_verbose("DEBUG VERBOSE\t\tPush double literal %f\n", v);
        }
        return size_to_skip;

        case 's': 
        {
            // Just leaking this right now, need a string table
            //
            char* text = alloc_until(code, index+1, len, ';', true, &size_to_skip);

            if (text == NULL) {
                em_panic(code, index, len, state, "Could not find a complete literal for string: Did you forget to terminate it?");
            }

            int top = stack_push(state);
            state->stack[top].signage = state->signed_flag;
            state->stack[top].code = current_code;
            state->stack[top].u.v_ptr = text; 
            state->stack[top].size = strlen(text) + 1; //alloc until is always NUL terminated

            log_verbose("DEBUG VERBOSE\t\tPush string literal %s skip %d\n", text, size_to_skip);
        }
        return size_to_skip;

        case '*': break;
        case '^': break;

        default:
            em_panic(code, index, len, state, "Unknown literal instruction %c", current_code);
            return 0;
    }

    return 0;
}

void dump_stack_item(em_stack_item* item, int top_index) {

    const char* type_colour = code_colour_code(item->code);
    const char* reset_colour = "\033[0m";

    fprintf(stdout, "[%d]\t%s %c %s", top_index, type_colour, item->code, reset_colour);

    fprintf(stdout, "\t=\t%s", type_colour);

    switch(item->code) {
        case '?': fprintf(stdout, "%d", item->u.v_bool); break;
        case '1': fprintf(stdout, "%d", item->u.v_byte);break;
        case '2': fprintf(stdout, "%d", item->u.v_int16);break;
        case '4': fprintf(stdout, "%d", item->u.v_int32);break;
        case '8': fprintf(stdout, "%llu", item->u.v_int64);break;
        case 'f': fprintf(stdout, "%f", item->u.v_float);break;
        case 'd': fprintf(stdout, "%f", item->u.v_double);break;
        case 's': fprintf(stdout, "\"%s\\0\" %p length %d", item->u.v_ptr, item->u.v_ptr, item->size);break;
        case '*': fprintf(stdout, "%p length %d", item->u.v_ptr, item->size); break;
        case '^': break;
    }

    fprintf(stdout, "%s", reset_colour);

    if (item->signage) {
         fprintf(stdout, " (signed) ");
    }

    fprintf(stdout, "\n");
}

void dump_stack(em_state* state) {
    fprintf(stdout, "=== Bottom of stack ===\n");
    for (int i = 0; i <= state->stack_ptr; i++) {
        dump_stack_item(&state->stack[i], i - state->stack_ptr);
    }
    fprintf(stdout, "=== Top of stack ===\n");
}

void dump_types(em_state* state) {
    for(int i = 0; i <= state->type_ptr; i++) {

        em_type_definition* type = &state->types[i];
        printf("\033[0;36m%s\033[0m (%db in size)\n", type->name, type->size);

        for(int t = 0; t < strlen(type->types); t++) {
            printf("%s%c", code_colour_code(type->types[t]), type->types[t]);
        }
        printf("\033[0m\n\n");

         for(int t = 0; t < strlen(type->types); t++) {
            printf("%s%c\033[0m\t\033[0;36m%s\n\033[0m", code_colour_code(type->types[t]), type->types[t], type->field_names[t]);
        }
        printf("\n");
    }
}

void dump_instructions(const char* code, int index, int len, em_state* state) {

    int lengthcontext = index + 32; 

    if (lengthcontext >= len) {
        lengthcontext = len;
    }

    int precontext = index - 32;

    if (precontext <= 0) {
        precontext = 0;
    }

    // Clamp the precontext so we start AFTER any newline
    for(int i = index; i >= precontext; i--) {
        if (code[i] == '\n') {
            precontext = i + 1;
            break;
        }
    }

    for(int i = index; i < lengthcontext; i++) {
        if (code[i] == '\n') {
            lengthcontext = i;
            break;
        }
    }

    const char* type_colour = NULL;

    fprintf(stdout, "\t");

    for (int i = precontext; i < lengthcontext; i++) {

        // Guess at top level codes
         switch (code[i]) {
            case 'm': 
            case 'l':
            case 's':
            case 'c':
            case 'd':
            case 'b':
                type_colour = "\033[0;36m"; 
            break;
            default: type_colour =  "\033[0m"; break;                
        }

        if (i == index) {
            type_colour = "\033[0;31m";
        }

        fprintf(stdout, "%s", type_colour);
        fprintf(stdout, "%c ", code[i]);
    }

    fprintf(stdout, "\n");
    fprintf(stdout, "\033[0m");
    fprintf(stdout, "\t");

    for (int i = precontext; i < lengthcontext; i++) {

        if (i == index) {
            fprintf(stdout, "%s", "\033[0;31m^^\033[0m");
            continue;
        }
        
        if (i == state->last_mode_change) {
            fprintf(stdout, "%s", "\033[0;31m| \033[0m");
            continue;
        }

        if (i > state->last_mode_change && i < index &&
            index >= precontext && index < lengthcontext &&
            state->last_mode_change >= precontext && state->last_mode_change < lengthcontext) {
            fprintf(stdout, "%s", "--");
            continue;
        }

        fprintf(stdout, "%s", "  ");       

    }

    fprintf(stdout, "\n\n(%s line %d col %d)\n", state->filename, (state->file_line+1),index + 1);
}


int run_stack(em_state* state, const char* code, int index, int len) {

    char current_code = tolower(code[index]);

     log_verbose("DEBUG VERBOSE\t\tStack start '%c'\n", current_code);

    switch(current_code) {

        // pop
        case 'p':         
            log_verbose("DEBUG VERBOSE\t\tStack pop\n");
            
            if (stack_pop(state) == NULL) {
                em_panic(code, index, len, state, "Cannot pop from stack: stack is empty");
            }

            return 0;
    }

    return 0;
}

int run_boolean(em_state* state, const char* code, int index, int len) {

    char current_code = tolower(code[index]);

    switch(current_code) {
        // add
        case '+': 
        {
            em_stack_item* one = stack_top_minus(state, 1);
            em_stack_item* two = stack_top(state);

            if (one == NULL || two == NULL) {
                em_panic(code, index, len, state, "Operation add requires two items on the stack to add");
            }

            if (!is_stack_item_numeric(one) || !is_stack_item_numeric(two)) {
                em_panic(code, index, len, state, 
                    "Operation add requires two arguments of numeric type. Got %c and %c", one->code, two->code);
            }

            stack_pop(state);
            stack_pop(state);

            switch(one->code) {
                case '1': 
                    switch(two->code) { 
                        case '1':  {                            
                            int ptr = stack_push(state);
                            state->stack[ptr].signage = one->signage;
                            state->stack[ptr].code = '1';
                            state->stack[ptr].u.v_byte = one->u.v_byte + two->u.v_byte;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot add a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break; 
                
                case '2': 
                    switch(two->code) { 
                        case '2':  {                            
                            int ptr = stack_push(state);
                            state->stack[ptr].signage = one->signage;
                            state->stack[ptr].code = '2';
                            state->stack[ptr].u.v_int16 = one->u.v_int16 + two->u.v_int16;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot add a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break;
                
                case '4': 
                    switch(two->code) { 
                        case '4':  {                            
                            int ptr = stack_push(state);
                            state->stack[ptr].signage = one->signage;
                            state->stack[ptr].code = '4';
                            state->stack[ptr].u.v_int32 = one->u.v_int32 + two->u.v_int32;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot add a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break;
                
                case '8': 
                    switch(two->code) { 
                        case '8':  {                            
                            int ptr = stack_push(state);
                            state->stack[ptr].signage = one->signage;
                            state->stack[ptr].code = '8';
                            state->stack[ptr].u.v_int64 = one->u.v_int64 + two->u.v_int64;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot add a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }
                break;
                
                case 'f': 
                    switch(two->code) { 
                        case 'f':  {                            
                            int ptr = stack_push(state);
                            state->stack[ptr].signage = one->signage;
                            state->stack[ptr].code = 'f';
                            state->stack[ptr].u.v_float = one->u.v_float + two->u.v_float;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot add a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }   
                break;

                case 'd': 
                    switch(two->code) { 
                        case 'd':  {                            
                            int ptr = stack_push(state);
                            state->stack[ptr].signage = one->signage;
                            state->stack[ptr].code = 'd';
                            state->stack[ptr].u.v_double = one->u.v_double + two->u.v_double;
                            state->stack[ptr].size = 0;
                        }
                        break;
                        default:
                        em_panic(code, index, len, state, "Cannot add a number of type %c to a number of type %c without a cast", one->code, two->code);
                        break;
                    }   
                break;
            }

        }
        break;

        // and
        case 'a': 
        {
            em_stack_item* one = stack_top_minus(state, 1);
            em_stack_item* two = stack_top(state);

            if (one == NULL || two == NULL) {
                em_panic(code, index, len, state, "Operation and requires two items on the stack");
            }

            if (one->code != '?' || two->code != '?') {
                em_panic(code, index, len, state, 
                    "Operation and requires two arguments of ? (boolean) type. Got %c and %c", one->code, two->code);
            }

            stack_pop(state);
            stack_pop(state);

            int ptr = stack_push(state);
            state->stack[ptr].signage = one->signage;
            state->stack[ptr].code = '?';
            state->stack[ptr].u.v_bool = one->u.v_bool && two->u.v_bool;
            state->stack[ptr].size = 0;
        }
        break;

        // or
        case 'o': 
        {
            em_stack_item* one = stack_top_minus(state, 1);
            em_stack_item* two = stack_top(state);

            if (one == NULL || two == NULL) {
                em_panic(code, index, len, state, "Operation or requires two items on the stack");
            }

            if (one->code != '?' || two->code != '?') {
                em_panic(code, index, len, state, 
                    "Operation or requires two arguments of ? (boolean) type. Got %c and %c", one->code, two->code);
            }

            stack_pop(state);
            stack_pop(state);

            int ptr = stack_push(state);
            state->stack[ptr].signage = one->signage;
            state->stack[ptr].code = '?';
            state->stack[ptr].u.v_bool = one->u.v_bool || two->u.v_bool;
            state->stack[ptr].size = 0;
        }
        break;

        // not
        case '!': 
        {
            em_stack_item* one = stack_top(state);
          
            if (one == NULL) {
                em_panic(code, index, len, state, "Operation not requires an item on top of the stack");
            }

            if (one->code != '?') {
                em_panic(code, index, len, state, 
                    "Operation not requires one argument of ? (boolean) type. Got %c", one->code);
            }

            stack_pop(state);

            int ptr = stack_push(state);
            state->stack[ptr].signage = one->signage;
            state->stack[ptr].code = '?';
            state->stack[ptr].u.v_bool = !one->u.v_bool;
            state->stack[ptr].size = 0;
        }
        break;

        // subtract
        case '-': break;

        // multiply
        case '*': break;

        // divide
        case '/': break;

        // mod
        case '%': break;

        // logical not
        case '~': break;

        // and
        case '&': break;

        // left shift
        case '<': break;

        // right shift
        case '>': break;

        default:
            em_panic(code, index, len, state, "Unknown boolean instruction %c", current_code);
            return 0;

    }

    return 0;

}

int run_debug(em_state* state, const char* code, int index, int len) {

    char current_code = tolower(code[index]);
    switch(current_code) {

        // Stack
        case 's':
        dump_stack(state);
        return 0;

        // Trace
        case 't':
        dump_instructions(code, index, len, state);
        break;

        // Type map
        case 'u':
        dump_types(state);
        break;

        // Assert that the two stack items present contain equivalent values
        case 'a':
        {
            em_stack_item* one = stack_top_minus(state, 1);
            em_stack_item* two = stack_top(state);

            if (one == NULL || two == NULL) {
                em_panic(code, index, len, state, "Operation assert requires two items on the stack to compare");
            }

            if (one->code == 's' && two->code == 's') {
                // Do a string compare
                if (strcmp((const char*)one->u.v_ptr, (const char*)two->u.v_ptr) != 0) {
                    em_panic(code, index, len, state, "Assertion failed (string compare)");
                }
            } else {
                if (memcmp(&one->u, &two->u, sizeof(one->u)) != 0) {
                    em_panic(code, index, len, state, "Assertion failed");
                }
            }

            stack_pop(state);
            stack_pop(state);
            
            log_verbose("DEBUG VERBOSE\t\tAssertion successful\n");
        }
        break;

        default:
            em_panic(code, index, len, state, "Unknown debug instruction %c", current_code);
            return 0;

    }

    return 0;
}

int run_memory(em_state* state, const char* code, int index, int len) {

    char current_code = tolower(code[index]);
    log_verbose("DEBUG VERBOSE\t\tMemory start '%c'\n", current_code);

    switch(current_code) {

        // alloc
        case 'a':
        {
            // We need an int on the stack
            em_stack_item* top = stack_top(state);

            if (top == NULL) {
                em_panic(code, index, len, state, "Insufficient arguments to memory allocation: requires integer size on stack top");
            }

            uint32_t real_size = 0;

            switch(top->code) {
                case '1': real_size = top->u.v_byte; break;
                case '2': real_size = top->u.v_int16; break;
                case '4': real_size = top->u.v_int32; break;
                case '8': real_size = top->u.v_int64; break;
                default: em_panic(code, index, len, state, "Memory allocation requires integer number on top of stack - found %c\n", top->code);
            }

            void* arb = malloc(real_size);
            memset(arb, 0, real_size);

            stack_pop(state);

            int ptr = stack_push(state);

            state->stack[ptr].signage = false;
            state->stack[ptr].code = '*';
            state->stack[ptr].u.v_ptr = arb;
            state->stack[ptr].size = real_size;

            log_verbose("DEBUG VERBOSE\t\tAllocated %db of memory @ %p\n", real_size, arb);

            return 0;
        }

        // free
        case 'f': 
        {
            // We need a MANAGED pointer on top of the stack
            em_stack_item* top = stack_top(state);

            if (top == NULL) {
                em_panic(code, index, len, state, "Insufficient arguments to memory free: requires pointer on stack top");
            }

            if (top->code != '*') {
                em_panic(code, index, len, state, "Memory free requires pointer top of stack - found %c\n", top->code);            
            }

            if (top->size == 0) {
                em_panic(code, index, len, state, "Attempting to free allocation of zero size: Unlikely to be legitimate allocation");
            }

            log_verbose("DEBUG VERBOSE\t\tFreeing %db of memory @ %p\n", top->size, top->u.v_ptr);
            free(top->u.v_ptr);

            top->signage = false;
            top->code = 0;
            top->u.v_ptr = 0;
            top->size = 0;

            stack_pop(state);   
            return 0;
        }

        // set
        case 's': 
        {

            return 0;
        }


        default:
            em_panic(code, index, len, state, "Unknown memory instruction %c", current_code);
            return 0;

    }
}

void run(const char* filename, const char* code, int len) {

    bool premature_exit = false;

    em_state state;
    memset(&state, 0, sizeof(em_state));
    state.stack_size = 4096;
    state.stack_ptr = -1;
    state.stack = malloc(sizeof(em_stack_item) * state.stack_size);
    state.filename = filename;
    memset(state.stack, 0, sizeof(em_stack_item) * state.stack_size);

    state.type_ptr = -1;
    state.max_types = 255;
    state.types = malloc(sizeof(em_type_definition) * state.max_types);
    memset(state.types, 0, sizeof(em_type_definition) * state.max_types);

    ESOMODE mode = EM_MEMORY;

    for (int i = 0; i < len; i++) {

        if (premature_exit) {
            break;
        }

        if (code[i] == '#') {
            log_verbose("DEBUG VERBOSE\t\tTerminating at comment at index %d\n", i);

            for(int current = i; current < len; current++) {
                if (code[current] == '\n') {
                    i = current - 1;
                    break;
                }
            }

            continue;
        }

        if (code[i] == '\n') {
            state.file_line++;
            continue;
        }

        if (isspace(code[i]) || code[i] == '\r') {
            continue;
        }

        log_verbose("DEBUG VERBOSE\t\t%d = %c\n", i, code[i]);

        // 'Free letters' i.e. top level mode changes
        switch(tolower(code[i])) {

            // Change mode
            case 'm': {
                char new_mode = safe_get(code, i+1, len);
                state.last_mode_change = i;
                switch(tolower(new_mode)) {
                    case 'b': mode = EM_BOOLEAN; break;
                    case 'm': mode = EM_MEMORY; break;
                    case 'l': mode = EM_LITERAL; break;
                    case 's': mode = EM_STACK; break;
                    case 'c': mode = EM_CC; break;
                    case 'd': mode = EM_DEBUG; break;
                    case 'u': mode = EM_UDT; break;
                default:
                    em_panic(code, i, len, &state, "Unknown mode change '%c'\n", new_mode);
                }

                log_verbose("DEBUG VERBOSE\t\tMODE CHANGED TO %d\n", mode);

                i++;
                continue;
            }
            break;

            default:
            {
                log_verbose("DEBUG VERBOSE MODE LOGIC\t\t%d\n", mode);

                int skip = 0;

                // Allow modes to eat text
                switch(mode) {
                    case EM_MEMORY: skip = run_memory(&state, code, i, len); break;
                    case EM_LITERAL: skip = run_literal(&state, code, i, len); break;
                    case EM_STACK: skip = run_stack(&state, code, i, len); break;
                    case EM_CC: break;
                    case EM_DEBUG: skip = run_debug(&state, code, i, len); break;
                    case EM_BOOLEAN: skip = run_boolean(&state, code, i, len); break;
                    case EM_UDT: skip = run_udt(&state, code, i, len); break;
                }

                log_verbose("DEBUG VERBOSE SKIP %d characters\t\t%d\n", skip);

                if (skip >= 0) {
                    i += skip;
                } else {
                    log_verbose("EOF reached detected by skip of %d\n", skip);
                    premature_exit = true;
                }

                break;
            }
        }       
       
    }
}