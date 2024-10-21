#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../shared/file_type_table.h"
#include "../shared/krauk_connection.h"
#include "./krauk_server_res/krauk_server_res.h"
#include "path_construct.h"
#include "user.h"

/*
    Fixa hantering av idn som inte finn
*/

void test_aes() {
    // check_file_structre();
    uint8_t iv[AES_IV_LEN];
    uint8_t key[AES_KEY_LEN];

    uint8_t tag[AES_TAG_LEN];
    uint8_t badtag[AES_TAG_LEN];

    // RAND_bytes(tag, AES_TAG_LEN);b
    RAND_bytes(iv, AES_IV_LEN);
    RAND_bytes(key, AES_KEY_LEN);

    for (size_t len = 100; len < 101; len++) {
        uint8_t *plaintext = calloc(len + 1, sizeof(uint8_t));
        uint8_t *decrypted = calloc(len + 1, sizeof(uint8_t));
        uint8_t *ciphertext = calloc(len + 256, sizeof(uint8_t));

        uint32_t ciphertext_len;
        uint32_t decrypted_len;

        RAND_bytes(plaintext, len);
        ciphertext_len = aes_256_gcm_encrypt(plaintext, len, key, iv, tag, ciphertext);

        decrypted_len = aes_256_gcm_decrypt(ciphertext, ciphertext_len, key, iv, tag, decrypted);
        printf("cipher text length: %d\n", ciphertext_len);
        printf("decrypted length: %d\n", decrypted_len);

        OPENSSL_assert(decrypted_len == ciphertext_len);

        if (decrypted_len == -1) {
            puts("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
            exit(EXIT_FAILURE);
        }

        free(decrypted);
        free(ciphertext);
        free(plaintext);
    }
};

void test_rsa() {
    SERVER_CTX *s_ctx = SERVER_CTX_new();
    uint8_t input[AES_TAG_LEN + 1] = "1234567812345678";

    int output_len;
    uint8_t *output = calloc(RSA_MAXLEN, 1);

    output_len = rsa_2048_encrypt(input, AES_TAG_LEN, s_ctx->public_key, &output);

    int len;
    uint8_t *decoded = NULL;
    len = rsa_2048_decrypt(output, output_len, s_ctx->private_key, &decoded);

    printf("inputlen[%d]\n", strlen(input));
    printf("ciphetext[%d]\n", output_len);
    printf("decodedlen[%d]\n", len);

    printf("\n\ninput [%s]", input);

    printf("\n\ndecoded [%s]", decoded);
    printf("\n\noutput [%s]", output);

    assert(memcmp(input, decoded, AES_TAG_LEN) == 0);
    SERVER_CTX_free(s_ctx);
};

void test_user_base() {
    USER_BASE *user_base = USER_BASE_deserialize("tempbase");
    printf("found %d old users\n", user_base->user_count);
    printf("\n~~~~~~~~~~Old users~~~~~~~~~~\n");

    for (size_t i = 0; i < user_base->user_count; i++) {
        printf("%s\n", user_base->users[i]->id);
    }

    printf("\n~~~~~~~~~~Generating new user~~~~~~~~~~\n");
    USER_INFO *new_user = USER_BASE_new_user(user_base);
    printf("generated user id: %s\n", new_user->id);
    USER_BASE_free(user_base);
}

void test_path_builder() {
    uint8_t user_id[] = "9adfd9e9690aab63a0baeab87f460beb";
    PATH_BUILDER *x = PATH_BUILDER_new(user_id, 1, 133);

    printf("INFO: %s\n", INFO_PATH(x));
    printf("HOME: %s\n", HOME_PATH(x));
    printf("PROJECT: %s\n", PROJECT_PATH(x));
    printf("VERSION: %s\n", VERSION_PATH(x));
    printf("TABLE: %s\n", TABLE_PATH(x));
    printf("TRACKED: %s\n", TRACKED_PATH(x));

    printf("HASHEd: %s\n", PATH_BUILDER_dynamic_dir(x, HASHED, "pungkulapungkulapungkulapungkulapungkula"));

    PATH_BUILDER_free(x);
}

typedef struct {
    EVP_PKEY *public_key;
    EVP_PKEY *private_key;
} test;

test test_key() {
    test a;
    EVP_PKEY *keypair = rsa_2480_rnd_keypair();
    EVP_PKEY *public_key;
    EVP_PKEY *private_key;

    rsa_write_key("public", keypair, RSA_PUBLIC);
    rsa_write_key("private", keypair, RSA_PRIVATE);

    public_key = rsa_read_key("public", RSA_PUBLIC);
    private_key = rsa_read_key("public", RSA_PRIVATE);

    EVP_PKEY_free(keypair);

    a.private_key = private_key;
    a.public_key = public_key;
    return a;
}

int main(int argc, char **argv) {
    // user managing
    CLIENT_CTX *c_ctx;
    SERVER_CTX *s_ctx;
    USER_BASE *ub;
    // net
    uint8_t buffer[LARGE_NET_BUFFER] = {0};
    KRAUK_FD server;
    KRAUK_FD client;
    // file managing
    struct stat st = {0};
    // monke
    int res;

    // creates basic file structure
    if (stat(P_ROOT, &st) == -1) {
        mkdir(P_ROOT, 0700);
    }
    if (stat(P_USERS, &st) == -1) {
        mkdir(P_USERS, 0700);
    }

    // creates server context
    s_ctx = SERVER_CTX_new();
    ub = USER_BASE_deserialize(P_USERBASE);

    // creates new user - temporary solotion
    if (argc == 2 && strcmp(argv[1], "new") == 0) {
        handle_new_user(s_ctx, ub);
        goto free_s;  // agh
    }

    // inits server
    server = krauk_server_create();

    while (1) {
        printf("[+] Waiting for client\n");
        client = krauk_server_accept_client(server);
        res = 0;

        // sets up client context
        c_ctx = CLIENT_CTX_new_blank();
        krauk_recv_header(client, c_ctx, s_ctx);
        if (USER_BASE_get_keys(ub, c_ctx) == -1) {
            puts("[-] User not found, aborting connection");
            goto free_c;  // quick and dumb
        }

        // recives message
        krauk_recv(client, c_ctx, buffer, WS_MSG, BUF_DEFAULT);

        // parses request
        switch (buffer[0]) {
            case CREATE_PROJECT:
                res = handle_create_repo(client, c_ctx, buffer);
                break;
            case POST_FILES:
                res = handle_post_repo(client, c_ctx, buffer);
                break;
            case LIST_REPOSITORIES:
                res = handle_list_repos(client, c_ctx);
                break;
            case PULL_REPOSITORY:
                res = handle_pull_repo(client, c_ctx, buffer);
                break;
            case PULL_REPOSITORY_VERSION:
                res = handle_pull_repo_version(client, c_ctx, buffer);
                break;
            case FILE_HASH_STREAM:
                res = handle_hased_file_stream(client, c_ctx, buffer);
            case FILE_STREAM:
                res = handle_file_stream(client, c_ctx, buffer);
            default:
                break;
        }

        if (res == -1) {
            puts("[-] non fatal error occurd, aborting client");
        }

    free_c:
        close(client);
        CLIENT_CTX_free(c_ctx);
        printf("[!] Client removed\n");
    }

    krauk_server_close(server);
free_s:
    USER_BASE_free(ub);
    SERVER_CTX_free(s_ctx);

    return 0;
}