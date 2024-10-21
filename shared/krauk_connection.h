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
#include "comms.h"
#include "file_info.h"
#include "ser.h"

#define LARGE_NET_BUFFER 4096
#define SMALL_NET_BUFFER 512
#define BUFFER_SIZE (LARGE_NET_BUFFER - RSA_MAXLEN)
#define MSG_SIZE (SMALL_NET_BUFFER - RSA_MAXLEN)
#define HEADER_SIZE (RSA_KEYLEN - RSA_PADDING_SIZE)
#define ENTRY_SIZE 200

#define WS_MSG 1
#define WS_PAYLOAD 2
#define BUF_CLEAR 1
#define BUF_DEFAULT 2

// connection decides size of user_id :(
#define USER_ID_LEN 32

typedef int8_t KRAUK_FD;

typedef struct {
    // per-request secret
    uint8_t key[AES_KEY_LEN];
    uint8_t iv[AES_IV_LEN];
    // user identification
    uint8_t id[USER_ID_LEN + 1];  // stringified user_id
    // server-client shared keypair - message authenticaiton
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
KRAUK_FD krauk_server_create();
void krauk_server_close(KRAUK_FD kfd);

/*
    Client
*/
KRAUK_FD krauk_client_connect();
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
int krauk_recive_file(KRAUK_FD kfd, CLIENT_CTX *ctx, char *file_name);
int krauk_send_file(KRAUK_FD kfd, CLIENT_CTX *ctx, file_info file);

/*
    DEBUGGING
*/
void DEBUG_LOG_CLIENT_CTX(CLIENT_CTX *ctx);