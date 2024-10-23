#include "enviroment.h"

ENV *ENV_read(char *path) {
    ENV *env;
    // file managing
    uint8_t default_path[FILENAME_MAX + 1];
    file_info env_file;
    row_file env_strings;
    // ENV_EMPTY temps;
    uint32_t name_len = 0;
    uint32_t value_len = 0;
    char *str;

    // sets path to default
    if (path == NULL) {
        strcpy(default_path, "./.env");
        path = default_path;
    }
    // checks if file exists
    if (access(path, F_OK) == -1) {
        return NULL;
    }

    // prepares environment
    env = calloc(1, sizeof(ENV));
    env->capacity = 10;
    env->count = 0;
    env->entries = calloc(env->capacity, sizeof(ENV_ENTRY));

    // opens & parses file
    env_file = open_file(path, O_RDWR, MEM_COPY);
    env_strings = row_file_create(env_file, ROW_DEFAULT);
    if (env_file.file_s.st_size == 0) {
        free(env->entries);
        free(env);
        close_file(env_file);
        row_file_destroy(env_strings);
        return NULL;
    }

    // steps though each row
    for (size_t i = 0; i < env_strings.row_count; i++) {
        str = env_strings.rows[i];
        // steps though each string
        for (size_t j = 0; j < strlen(str); j++) {
            if (str[j] == '=') {
                name_len = j;
                value_len = strlen(str) - j - 1;
                break;
            }
        }

        if (name_len == 0 || value_len == 0) {
            free(env->entries);
            free(env);
            close_file(env_file);
            row_file_destroy(env_strings);
            return NULL;
        }

        // increases the size if needed
        if (env->count >= env->capacity) {
            env->capacity *= 2;
            env->entries = realloc(env->entries, env->capacity * sizeof(ENV_ENTRY));
        }

        // callocs & sets the values;
        env->entries[env->count].name = calloc(name_len + 1, 1);
        env->entries[env->count].value = calloc(value_len + 1, 1);

        memcpy(env->entries[env->count].name, str, name_len);
        memcpy(env->entries[env->count].value, str + name_len + 1, value_len);

        env->count += 1;

        // resets temps;
        name_len = 0;
        value_len = 0;
        str = NULL;
    }

    close_file(env_file);
    row_file_destroy(env_strings);

    return env;
}
void ENV_free(ENV *env) {
    for (size_t i = 0; i < env->count; i++) {
        free(env->entries[i].name);
        free(env->entries[i].value);
    }
    free(env->entries);
    free(env);
}

char *ENV_get(ENV *env, char *name) {
    for (size_t i = 0; i < env->count; i++) {
        if (strcmp(env->entries[i].name, name) == 0) {
            return env->entries[i].value;
        }
    }

    return NULL;
}

// debugging
void ENV_log(ENV *env) {
    printf("~~~~~environment entries: %d~~~~~\n", env->count);
    for (size_t i = 0; i < env->count; i++) {
        printf("[%s]: %s\n", env->entries[i].name, env->entries[i].value);
    }
    puts("");
}