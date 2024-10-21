#pragma once

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../shared/krauk_connection.h"
#include "../shared/cipher.h"



typedef struct {
    uint8_t id[USER_ID_LEN + 1]; // stringifed for ease of use
    // per client secrets
    EVP_PKEY *MAC_public_key; // ** <- test
    EVP_PKEY *MAC_private_key; // ** <- test
} USER_INFO;

typedef struct {
    USER_INFO **users;
    uint32_t user_count;
    FILE *user_fp;
} USER_BASE;

USER_INFO *USER_BASE_new_user(USER_BASE *ub);
USER_BASE *USER_BASE_deserialize(char *user_base_file_path);
void USER_BASE_free(USER_BASE *ub);
int USER_BASE_get_keys(USER_BASE *ub, CLIENT_CTX *ctx);
