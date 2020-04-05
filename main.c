// TODO
// String tables
// if and loops

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
    EM_DEBUG

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

    em_stack_item* stack;
    int stack_ptr;
    int stack_size;
    int last_mode_change;
    bool signed_flag;

} em_state;

void run(const char* i, int len);
void dump_instructions(const char* code, int index, int len, em_state* state);
void dump_stack(em_state* state);

int main(int argc, char** argv) {

    if (argc < 2) {
        fprintf(stderr, "At least one argument required\n");
        exit(0);
    }

    fprintf(stdout, "Called with: %s\n", argv[1]);

    run(argv[1], strlen(argv[1]));
    return 0;
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

em_stack_item* stack_pop(em_state* state) {
    if (state->stack_ptr < 0) {
        return NULL;
    } else {
        em_stack_item* top = &state->stack[state->stack_ptr];

        state->stack_ptr--;

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

    int starting_index = index;

    while(eat_whitespace && isspace(code[starting_index])) {
        starting_index++;
    }

    for (int i = starting_index ; i < len; i++) {
        if (code[i] == terminator) {
            
            char* built = malloc(i - index + 1);
            memset(built, 0, i - index + 1);
            memcpy(built, code+index, i - index);

            if (size_to_skip != NULL) {
                if (i + 1 < len) {
                    *size_to_skip = (i - index) + 1;
                } else {
                    *size_to_skip = 0;
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
            char* test = alloc_until(code, index+1, len, '_', true, &size_to_skip);
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
            char* test = alloc_until(code, index+1, len, '_', true, &size_to_skip);
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
            char* test = alloc_until(code, index+1, len, '_', true, &size_to_skip);
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
            char* test = alloc_until(code, index+1, len, '_', true, &size_to_skip);
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
            char* test = alloc_until(code, index+1, len, '_', true, &size_to_skip);
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
            char* test = alloc_until(code, index+1, len, '_', true, &size_to_skip);
            double v = strtod(test, NULL);
            free(test);

            int top = stack_push(state);
            state->stack[top].signage = state->signed_flag;
            state->stack[top].code = current_code;
            state->stack[top].u.v_double = v;

            log_verbose("DEBUG VERBOSE\t\tPush double literal %f\n", v);
        }
        return size_to_skip;

        case 's': break;
        case '*': break;
        case '^': break;

        default:
            em_panic(code, index, len, state, "Unknown literal instruction %c", current_code);
            return 0;
    }

    return 0;
}

void dump_stack_item(em_stack_item* item) {

    const char* type_colour;
    const char* reset_colour = "\033[0m";

    switch(item->code) {
        case '?': type_colour = "\033[0;36m"; break;
        case '1': type_colour = "\033[0;33m"; break;
        case '2': type_colour = "\033[0;32m"; break;
        case '4': type_colour = "\033[0;32m"; break;
        case '8': type_colour = "\033[0;32m"; break;
        case 'f': type_colour = "\033[0;35m"; break;
        case 'd': type_colour = "\033[0;35m"; break;
        case 's': type_colour = "\033[0;31m"; break;
        case '*': type_colour = "\033[0;31m"; break;
        case '^': type_colour = "\033[0;31m"; break;
        
    }

    fprintf(stdout, "%s %c %s", type_colour, item->code, reset_colour);

    fprintf(stdout, "\t=\t%s", type_colour);

    switch(item->code) {
        case '?': fprintf(stdout, "%d", item->u.v_bool); break;
        case '1': fprintf(stdout, "%d", item->u.v_byte);break;
        case '2': fprintf(stdout, "%d", item->u.v_int16);break;
        case '4': fprintf(stdout, "%d", item->u.v_int32);break;
        case '8': fprintf(stdout, "%llu", item->u.v_int64);break;
        case 'f': fprintf(stdout, "%f", item->u.v_float);break;
        case 'd': fprintf(stdout, "%f", item->u.v_double);break;
        case 's': break;
        case '*': fprintf(stdout, "%p length %d", item->u.v_ptr, item->size); break;break;
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
        dump_stack_item(&state->stack[i]);
    }
    fprintf(stdout, "=== Top of stack ===\n");
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

    fprintf(stdout, "\n(col %d)\n", index + 1);
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
        // and
        case 'a': 
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

        // or
        case 'o': break;

        // not
        case '!': break;

        // add
        case '+': break;

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

        // Assert that the two stack items present contain equivalent values
        case 'a':
        {
            em_stack_item* one = stack_top_minus(state, 1);
            em_stack_item* two = stack_top(state);

            if (one == NULL || two == NULL) {
                em_panic(code, index, len, state, "Operation assert requires two items on the stack to compare");
            }

            if (memcmp(&one->u, &two->u, sizeof(one->u)) != 0) {
                em_panic(code, index, len, state, "Assertion failed");
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

void run(const char* code, int len) {

    em_state state;
    memset(&state, 0, sizeof(em_state));
    state.stack_size = 4096;
    state.stack_ptr = -1;
    state.stack = malloc(sizeof(em_stack_item) * state.stack_size);
    memset(state.stack, 0, sizeof(em_stack_item) * state.stack_size);

    ESOMODE mode = EM_MEMORY;

    for (int i = 0; i < len; i++) {

        if (isspace(code[i])) {
            continue;
        }

        if (code[i] == '#') {
            log_verbose("DEBUG VERBOSE\t\tTerminating at comment at index %d\n", i);
            return;
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
                default:
                    em_panic(code, i, len, &state, "Unknown mode change '%c'\n", code[i]);
                }

                log_verbose("DEBUG VERBOSE\t\tMODE CHANGED TO %d\n", mode);

                i++;
                continue;
            }
            break;

            default:
            log_verbose("DEBUG VERBOSE MODE LOGIC\t\t%d\n", mode);

            // Allow modes to eat text
            switch(mode) {
                case EM_MEMORY: i += run_memory(&state, code, i, len); break;
                case EM_LITERAL: i += run_literal(&state, code, i, len); break;
                case EM_STACK: i += run_stack(&state, code, i, len); break;
                case EM_CC: break;
                case EM_DEBUG: i += run_debug(&state, code, i, len); break;
                case EM_BOOLEAN: i += run_boolean(&state, code, i, len); break;
            }
            break;
        }       
       
    }
}