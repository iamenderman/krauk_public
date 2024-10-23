#include "krauk_connection.h"

#define S_KEYS "keys"
#define S_PRIVATE_KEY "keys/private_key.der"
#define S_PUBLIC_KEY "keys/public_key.der"

/*
    CONTEXT
*/
SERVER_CTX *SERVER_CTX_new() {
    SERVER_CTX *ctx = NULL;
    EVP_PKEY *pair = NULL;
    struct stat st = {0};

    ctx = calloc(1, sizeof(SERVER_CTX));

    // checks if public and priva keys dont exist
    if (access(S_PUBLIC_KEY, F_OK) == -1 || access(S_PRIVATE_KEY, F_OK) == -1) {
        if (stat(S_KEYS, &st) == -1) {
            mkdir(S_KEYS, 0700);
        }

        pair = rsa_2480_rnd_keypair();

        if (rsa_write_key(S_PUBLIC_KEY, pair, RSA_PUBLIC) == 0) {
            perror("[-] Failed to write server public key");
            exit(EXIT_FAILURE);
        }

        if (rsa_write_key(S_PRIVATE_KEY, pair, RSA_PRIVATE) == 0) {
            perror("[-] Failed to write server private key");
            exit(EXIT_FAILURE);
        }

        EVP_PKEY_free(pair);
    }

    ctx->public_key = rsa_read_key(S_PUBLIC_KEY, RSA_PUBLIC);
    if (ctx->public_key == NULL) {
        perror("[-] Failed to read server public key");
        exit(EXIT_FAILURE);
    }

    ctx->private_key = rsa_read_key(S_PRIVATE_KEY, RSA_PRIVATE);
    if (ctx->private_key == NULL) {
        perror("[-] Failed to read server private key");
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void SERVER_CTX_free(SERVER_CTX *ctx) {
    EVP_PKEY_free(ctx->private_key);
    EVP_PKEY_free(ctx->public_key);

    free(ctx);
}

void init_SERVER_CLIENT_CTX(char *identification_path, SERVER_CTX **s_ctx, CLIENT_CTX **c_ctx) {
    FILE *fp;

    if (access(identification_path, F_OK) == -1) {
        fprintf(stderr, "[-] No registerd user found\n");
        exit(EXIT_FAILURE);
    }

    *c_ctx = calloc(1, sizeof(CLIENT_CTX));
    *s_ctx = calloc(1, sizeof(SERVER_CTX));

    fp = fopen(identification_path, "r+");

    /*
        server ctx construction
    */
    // reads server public key path
    (*s_ctx)->public_key = rsa_fread_key(fp, RSA_PUBLIC);
    if ((*s_ctx)->public_key == NULL) {
        fprintf(stderr, "[-] Failed to read server public key - try reg again\n");
        exit(EXIT_FAILURE);
    }
    /*
        client ctx construction
    */
    if (fread((*c_ctx)->id, 1, USER_ID_LEN, fp) != USER_ID_LEN) {
        fprintf(stderr, "[-] Failed to read userid - Invalid identification file, try reg again :(\n");
        exit(EXIT_FAILURE);
    }

    // reads shared public key
    (*c_ctx)->MAC_public_key = rsa_fread_key(fp, RSA_PUBLIC);
    if ((*c_ctx)->MAC_public_key == NULL) {
        fprintf(stderr, "[-] Failed to read shared public key - try reg again\n");
        exit(EXIT_FAILURE);
    }

    // reads shared private key
    (*c_ctx)->MAC_private_key = rsa_fread_key(fp, RSA_PRIVATE);
    if ((*c_ctx)->MAC_private_key == NULL) {
        fprintf(stderr, "[-] Failed to read shared private key - try reg again\n");
        exit(EXIT_FAILURE);
    }

    // constructs key
    SECURE_RND_BYTES((*c_ctx)->key, AES_KEY_LEN);
    SECURE_RND_BYTES((*c_ctx)->iv, AES_IV_LEN);

    // cleanup
    fclose(fp);
}

CLIENT_CTX *CLIENT_CTX_new_blank() {
    return calloc(1, sizeof(CLIENT_CTX));
}

void CLIENT_CTX_free(CLIENT_CTX *ctx) {
    if (ctx->MAC_private_key != NULL) {
        EVP_PKEY_free(ctx->MAC_private_key);
    }
    if (ctx->MAC_public_key != NULL) {
        EVP_PKEY_free(ctx->MAC_public_key);
    }

    free(ctx);
}

/*
    Server
*/
KRAUK_FD krauk_server_accept_client(KRAUK_FD kfd) {
    struct sockaddr_in client_addr;

    // listens for cliens
    if (listen(kfd, 1) == -1) {
        fprintf(stderr, "[-] Error while trying to listen for client: ");
        perror("");
        exit(EXIT_FAILURE);
    }

    uint32_t addr_size = sizeof(client_addr);

    KRAUK_FD client_fd = accept(kfd, (struct sockaddr *)&client_addr, &addr_size);
    if (client_fd == -1) {
        fprintf(stderr, "[-] Connection establishment failed");
        perror("");
        exit(EXIT_FAILURE);
    }

    return client_fd;
}

KRAUK_FD krauk_server_create(char *host, int port) {
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host);

    // creates socket
    int8_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("[-] server socket creation failed: ");
        exit(EXIT_FAILURE);
    }
    puts("[+] Socket created...");

    // binds addr to socket
    int8_t status = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (status == -1) {
        perror("[-] failed binding with client: ");
        exit(EXIT_FAILURE);
    }
    puts("[+] Bound adress to server socket...");

    return server_fd;
}

void krauk_server_close(KRAUK_FD server) {
    printf("closed server");
    close(server);
}

/*
    Client
*/
KRAUK_FD krauk_client_connect(char *host, int port) {
    // sets and creates dst
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host);

    // creates socket
    int8_t client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        fprintf(stderr, "[-] Client socket creation failed: ");
        perror("");
        exit(EXIT_FAILURE);
    }
    puts("[+] Socket created...");

    int8_t status = connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (status == -1) {
        fprintf(stderr, "[-] Connection to server falied: ");
        perror("");
        exit(EXIT_FAILURE);
    }
    puts("[+] Connected to server...");

    return client_fd;
}

void krauk_client_close(KRAUK_FD client) {
    printf("[+] Closed client\n");
    close(client);
}

/*
    Key sharing
*/
int krauk_send_header(KRAUK_FD kfd, CLIENT_CTX *c_ctx, SERVER_CTX *s_ctx) {
    uint8_t header[BUFFER_SIZE];
    uint8_t *ciphertext = NULL;
    int64_t status;

    memcpy(header, c_ctx->key, AES_KEY_LEN);
    memcpy(header + AES_KEY_LEN, c_ctx->iv, AES_IV_LEN);
    memcpy(header + AES_KEY_LEN + AES_IV_LEN, c_ctx->id, USER_ID_LEN);

    if (rsa_2048_encrypt(header, AES_KEY_LEN + AES_IV_LEN + USER_ID_LEN, s_ctx->public_key, &ciphertext) == -1) {
        if (ciphertext != NULL) {
            free(ciphertext);
        }

        return -1;
    }

    status = send(kfd, ciphertext, RSA_MAXLEN, 0);
    if (status == -1) {
        perror("[-] Failed to send payload: ");
        free(ciphertext);
        return -1;
    }

    // cleanup
    free(ciphertext);

    return 0;
}

int krauk_recv_header(KRAUK_FD kfd, CLIENT_CTX *c_ctx, SERVER_CTX *s_ctx) {
    uint8_t header[BUFFER_SIZE];
    uint8_t *plaintext = NULL;
    int64_t status;

    status = recv(kfd, header, LARGE_NET_BUFFER, 0);
    if (status == -1) {
        perror("[-] Error while recving payload: ");
        return -1;
    }

    // decrypts received buffer
    if (rsa_2048_decrypt(header, RSA_MAXLEN, s_ctx->private_key, &plaintext) == -1) {
        if (plaintext != NULL) {
            free(plaintext);
        }

        return -1;
    }

    // decyrpts the per connection secrets
    memcpy(c_ctx->key, plaintext, AES_KEY_LEN);
    memcpy(c_ctx->iv, plaintext + AES_KEY_LEN, AES_IV_LEN);
    memcpy(c_ctx->id, plaintext + AES_KEY_LEN + AES_IV_LEN, USER_ID_LEN);

    free(plaintext);

    return 0;
}

int krauk_send(KRAUK_FD kfd, CLIENT_CTX *ctx, uint8_t *buffer, int ws_flags) {
    // package size managing
    uint16_t size;
    uint16_t msg_size;
    // cryp buffers
    uint8_t tag[AES_TAG_LEN + 1] = {0};
    uint8_t *encrypted_tag = NULL;
    uint8_t *cipherbuffer = NULL;
    // net
    int64_t status;

    // calc the size
    size = ws_flags == WS_MSG ? SMALL_NET_BUFFER : LARGE_NET_BUFFER;
    msg_size = size - RSA_MAXLEN;

    cipherbuffer = calloc(size, sizeof(uint8_t));

    // encrypts msg, and encryps generated tag
    if (aes_256_gcm_encrypt(buffer, msg_size, ctx->key, ctx->iv, tag, cipherbuffer) == -1) {
        free(cipherbuffer);
        return -1;
    }

    // encrypts the buffer of the msg
    if (rsa_2048_encrypt(tag, AES_TAG_LEN, ctx->MAC_public_key, &encrypted_tag) == -1) {
        if (encrypted_tag != NULL) {
            free(encrypted_tag);
        }
        free(cipherbuffer);
        return -1;
    }

    // appends tag to
    memcpy(cipherbuffer + msg_size, encrypted_tag, RSA_MAXLEN);

    status = send(kfd, cipherbuffer, size, 0);
    if (status == -1) {
        perror("[-] Failed to send payload: ");
        return -1;
    }

    // cleanup
    free(encrypted_tag);
    free(cipherbuffer);

    return 0;
}

int krauk_recv(KRAUK_FD kfd, CLIENT_CTX *ctx, uint8_t *buffer, int ws_flags, int buf_flags) {
    // package size managing
    uint16_t size;
    uint16_t msg_size;
    // crypt buffers
    uint8_t *tag = NULL;
    uint8_t *cipherbuffer = NULL;
    // net
    uint32_t status;

    // calc the size
    size = ws_flags == WS_MSG ? SMALL_NET_BUFFER : LARGE_NET_BUFFER;
    msg_size = size - RSA_MAXLEN;
    cipherbuffer = calloc(size, sizeof(uint8_t));

    if (buf_flags == BUF_CLEAR) {
        BZERO(buffer, size);
    }

    // recives data
    status = recv(kfd, cipherbuffer, size, 0);
    if (status == -1) {
        free(cipherbuffer);
        perror("[-] Error while recving payload: ");
        return -1;
    }

    // decrypts the tag
    if (rsa_2048_decrypt(cipherbuffer + msg_size, RSA_MAXLEN, ctx->MAC_private_key, &tag) == -1) {
        if (tag != NULL) {
            free(tag);
        }
        free(cipherbuffer);
        return -1;
    }

    // decrypts the buffer contents
    if (aes_256_gcm_decrypt(cipherbuffer, msg_size, ctx->key, ctx->iv, tag, buffer) == -1) {
        free(cipherbuffer);
        free(tag);
        return -1;
    }

    // cleanup
    free(cipherbuffer);
    free(tag);

    return 0;
}

/*
    File & multi-send buffer transfer
    TODO: optimize send and recive
            - larger buffer?
            - smaller acks?
            - fewer acks? 3rd?
*/
int krauk_receive_file(KRAUK_FD kfd, CLIENT_CTX *ctx, char *file_name) {
    uint8_t buffer[LARGE_NET_BUFFER] = {0};
    uint64_t file_size;
    int64_t received_bytes;
    int32_t fd;
    int16_t write_size;

    fprintf(stderr, "%s\n", file_name);
    fd = open(file_name, O_RDWR | O_CREAT, 0777);

    if (fd == -1) {
        fprintf(stderr, "failed to open file[%s]: ", file_name);
        perror("");
        close(fd);
        return -1;
    }

    if (ftruncate(fd, 0) == -1) {
        perror("[-] Failed to clear file: ");
        close(fd);
        return -1;
    }

    if (krauk_recv(kfd, ctx, buffer, WS_PAYLOAD, BUF_CLEAR) == -1) {
        close(fd);
        return -1;
    }

    file_size = decode_uint64_t(buffer);
    // nothing to send.
    if (file_size == 0) {
        close(fd);
        return -1;
    }

    /*
        First payload could be integrated into the recive loop -
        but would require redundent checks each new payload
    */
    received_bytes = 0;
    write_size = BUFFER_SIZE - sizeof(long);

    if (file_size <= BUFFER_SIZE - sizeof(long)) {  // if the file fit into the first paylaod
        write_size = file_size;
    }

    // fill the first payload
    write(fd, buffer + sizeof(long), write_size);
    received_bytes += write_size;
    write_size = BUFFER_SIZE;

    while (received_bytes != file_size) {
        // checks if the next write is the last write.
        if (file_size - received_bytes < BUFFER_SIZE) {  // checks if thge
            write_size = file_size - received_bytes;
        }

        if (krauk_recv(kfd, ctx, buffer, WS_PAYLOAD, BUF_CLEAR) == -1) {
            close(fd);
            return -1;
        }

        write(fd, buffer, write_size);
        received_bytes += write_size;

        // // sends ack
        // BZERO(buffer, MSG_SIZE);
        // encode_sequence(buffer, 2, POST_FILES, REQUEST_VALID);
        // if (krauk_send(kfd, ctx, buffer, WS_MSG) == -1) {
        //     puts("[-] Failed to send ack");
        //     return -1;
        // } else {
        //     puts("SENT ACK");
        // }

    }

    // sends file received ack
    BZERO(buffer, MSG_SIZE);
    encode_sequence(buffer, 2, POST_FILES, FILE_RECEIVED);
    krauk_send(kfd, ctx, buffer, WS_MSG);

    close(fd);

    return 0;
}

int krauk_send_file(KRAUK_FD kfd, CLIENT_CTX *ctx, file_info file) {
    uint8_t buffer[LARGE_NET_BUFFER] = {0};
    int64_t sent_bytes = 0;
    int16_t next_expected_size = BUFFER_SIZE - sizeof(long);

    // shifts the file size into the buffer
    encode_uint64_t(buffer, file.file_s.st_size);

    puts("sending file");
    while (sent_bytes != file.file_s.st_size) {
        /*
            if the remaning bytes is less than expexted size, the expexted size
            is shrunken to the remaning bits
        */
        if (file.file_s.st_size - sent_bytes < next_expected_size) {
            next_expected_size = file.file_s.st_size - sent_bytes;
        }

        // copies the memory - offset if its the first payload
        if (sent_bytes == 0) {
            memcpy(buffer + sizeof(long), file.file + sent_bytes, next_expected_size);
        } else {
            memcpy(buffer, file.file + sent_bytes, next_expected_size);
        }

        /*
            send payload 
        */
        if (krauk_send(kfd, ctx, buffer, WS_PAYLOAD) == -1) {
            return -1;
        }


        // /*
        //     Wait for ack
        // */
        // if (krauk_recv(kfd, ctx, buffer, WS_MSG, BUF_CLEAR) == -1) {
        //     return -1;
        // }
        // if (buffer[0] != POST_FILES || buffer[1] != REQUEST_VALID) {
        //     puts("[-] Failed to send file, no ack received");
        //     return -1;
        // }

        // offset update
        sent_bytes += next_expected_size;
        next_expected_size = BUFFER_SIZE;

        BZERO(buffer, BUFFER_SIZE);
    }

    // await file received ack
    if (krauk_recv(kfd, ctx, buffer, WS_MSG, BUF_CLEAR) == -1) {
        return -1;
    }

    if (buffer[0] != POST_FILES || buffer[1] != FILE_RECEIVED) {
        puts("[-] Failed to send file, reciver side error");
        return -1;
    }

    return 0;
}

void DEBUG_LOG_CLIENT_CTX(CLIENT_CTX *ctx) {
    printf("\nkey: \n%.*s\n", AES_KEY_LEN, ctx->key);
    printf("iv: \n%.*s\n", AES_IV_LEN, ctx->iv);
    printf("id: %.*s\n", USER_ID_LEN, ctx->id);
}