/*
    Quick and dangerous & dirty & ugly .ENV implmentation
*/

#pragma once

#include "file_info.h"
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    char *name;
    char *value;
} ENV_ENTRY;

typedef struct {
    ENV_ENTRY *entries;
    uint32_t count;
    uint32_t capacity;
} ENV;

// ENV managing
ENV *ENV_read(char *path);
void ENV_free(ENV *env);

// returns a reference, null on failure
char *ENV_get(ENV *env, char *name);

// debugging
void ENV_log(ENV *env);