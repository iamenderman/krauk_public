#include "krauk_client_ops.h"

// TODO: complete
int krauk_request_file(KRAUK_FD kc, CLIENT_CTX *ctx) {
    uint8_t buffer[LARGE_NET_BUFFER];

    buffer[0] = FILE_HASH_STREAM;
    encode_uint32_t(buffer + 1, 1);
    encode_uint32_t(buffer + sizeof(uint32_t) + 1, 1);
    encode_uint32_t(buffer + (2 * sizeof(uint32_t)) + 1, 0);

    if (krauk_send(kc, ctx, buffer, WS_MSG) == -1) {
        return -1;
    }
    if (krauk_receive_file(kc, ctx, "test!") == -1) {
        return -1;
    }

    file_info temp_file = open_file("test!", O_RDWR, MEM_DEFAULT);
    printf("%s\n", temp_file.file);

    return 0;
}