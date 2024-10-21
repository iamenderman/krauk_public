#pragma once

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
    int32_t size;
    int32_t capacity;
    char **stack;
} string_stack;

void string_stack_destroy(string_stack *stack);
string_stack *string_stack_create();
string_stack *string_stack_append(string_stack *stack, char *str);