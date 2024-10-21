#include "../shared/string_stack.h"

string_stack *string_stack_create() {
    string_stack *stack = calloc(1, sizeof(string_stack));
    stack->capacity = 10;
    stack->size = 0;
    stack->stack = calloc(10, sizeof(char *));
    return stack;
}

void string_stack_destroy(string_stack *stack) {
    for (size_t i = 0; i < stack->size; i++) {
        free(stack->stack[i]);
    }
    free(stack->stack);
    free(stack);
}; 

string_stack *string_stack_append(string_stack *stack, char *str) {
    int16_t str_len = strlen(str) + 1;
    
    if (stack->size == stack->capacity) {
        stack->capacity *= 3;
        stack->stack = realloc(stack->stack, stack->capacity * sizeof(char *));
    }

    char *temp = malloc(str_len *  sizeof(char));
    memcpy(temp, str, str_len);
    stack->stack[stack->size] = temp;
    stack->size++;

    return stack;
}
