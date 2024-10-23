#pragma once
#include <arpa/inet.h>
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cipher.h"
#include "file_info.h"
#include "ser.h"

// TODO: move defines
#define BZERO(b,len) (memset((b), '\0', (len)), (void) 0)
#define KRAUK_EMPTY_FILE "EMPTY_FILE"
#define USER_ID_LEN 32

// buffer sizes
#define LARGE_NET_BUFFER 4096
#define SMALL_NET_BUFFER 512
#define BUFFER_SIZE (LARGE_NET_BUFFER - RSA_MAXLEN)
#define MSG_SIZE (SMALL_NET_BUFFER - RSA_MAXLEN)
#define HEADER_SIZE (RSA_KEYLEN - RSA_PADDING_SIZE)
#define ENTRY_SIZE 200

// ws communication managing
#define PORT 8080
#define HOST "127.0.0.1"

// requests
#define LIST_VERSIONS 1
#define LIST_REPOSITORIES 2
#define PULL_REPOSITORY 3
#define PULL_REPOSITORY_VERSION 4
#define POST_FILES 5
#define CREATE_PROJECT 6
#define FILE_HASH_STREAM 7
#define FILE_STREAM 8

// responses
#define REQUEST_ERROR 9
#define REQUEST_VALID 10
#define FILE_RECEIVED 11

// file stream managing
#define FREQ_FILE 12
#define TRACKED_FILE 13
#define PROJECT_FILE 14

// ws managing
#define WS_MSG 1
#define WS_PAYLOAD 2
#define BUF_CLEAR 3
#define BUF_DEFAULT 4


typedef int8_t KRAUK_FD;

typedef struct {
    // per-request secret
    uint8_t key[AES_KEY_LEN];
    uint8_t iv[AES_IV_LEN];
    // user identification
    uint8_t id[USER_ID_LEN + 1];  // stringified user_id
    // server-client shared keypair - message authentication
    EVP_PKEY *MAC_public_key;
    EVP_PKEY *MAC_private_key;
} CLIENT_CTX;

typedef struct {
    EVP_PKEY *public_key;
    EVP_PKEY *private_key;
} SERVER_CTX;

/*
    Context :)
*/
SERVER_CTX *SERVER_CTX_new();
void init_SERVER_CLIENT_CTX(char *identification_path, SERVER_CTX **s_ctx, CLIENT_CTX **c_ctx);
CLIENT_CTX *CLIENT_CTX_new_blank();
void SERVER_CTX_free(SERVER_CTX *ctx);
void CLIENT_CTX_free(CLIENT_CTX *ctx);

/*
    Server
*/
KRAUK_FD krauk_server_accept_client(KRAUK_FD kfd);
KRAUK_FD krauk_server_create(char *host, int port);
void krauk_server_close(KRAUK_FD kfd);

/*
    Client
*/
KRAUK_FD krauk_client_connect(char *host, int port);
void krauk_client_close(KRAUK_FD kfd);

/*
    Key sharing
*/
int krauk_send_header(KRAUK_FD kfd, CLIENT_CTX *c_ctx, SERVER_CTX *s_ctx);
int krauk_recv_header(KRAUK_FD kfd, CLIENT_CTX *c_ctx, SERVER_CTX *s_ctx);

/*
    MSG sending
*/
int krauk_send(KRAUK_FD kfd, CLIENT_CTX *ctx, uint8_t *buffer, int ws_flags);
int krauk_recv(KRAUK_FD kfd, CLIENT_CTX *ctx, uint8_t *buffer, int ws_flags, int buf_flags);

/*
    File & multi-send buffer transfer
*/
int krauk_receive_file(KRAUK_FD kfd, CLIENT_CTX *ctx, char *file_name);
int krauk_send_file(KRAUK_FD kfd, CLIENT_CTX *ctx, file_info file);

/*
    DEBUGGING
*/
void DEBUG_LOG_CLIENT_CTX(CLIENT_CTX *ctx);