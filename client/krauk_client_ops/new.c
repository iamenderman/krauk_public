#include "krauk_client_ops.h"

int32_t krauk_new_repo(KRAUK_FD kc, CLIENT_CTX *ctx, char *project_name) {
    file_info info;
    uint8_t buffer[LARGE_NET_BUFFER] = {0};
    uint8_t entry[ENTRY_SIZE] = {0};
    uint32_t repo_id;

    // project name check
    if (strlen(project_name) > ENTRY_SIZE - sizeof(uint32_t) - 1) {
        fprintf(stderr, "[-] Project names are limted to %d chars\n", ENTRY_SIZE - sizeof(uint32_t) - 1);
        return -1;
    }

    // reads/creates info file & validates content
    info = open_file(C_INFO_FILE, O_RDWR, MEM_DEFAULT);
    if (info.file_s.st_size > 0) {
#ifndef DEBUG
        // check if the id is placeholder id
        if (info.file_s.st_size >= sizeof(uint32_t) && decode_uint32_t(info.file) != 0) {
            fprintf(stderr, "[-] Existing krauk repo found!");
            return -1;
        }
#endif
        close_file(info);
        info = open_file(C_INFO_FILE, O_RDWR | O_TRUNC, MEM_DEFAULT);
    }

    // creates repo reqeust message
    buffer[0] = CREATE_PROJECT;
    encode_str(buffer + 1, project_name);
    if (krauk_send(kc, ctx, buffer, WS_MSG) == -1) {
        return -1;
    }
    if (krauk_recv(kc, ctx, buffer, WS_MSG, BUF_CLEAR) == -1) {
        return -1;
    }
    if (buffer[1] == REQUEST_ERROR) {
        fprintf(stderr, "[-] Server msg: %s\n", buffer + 2);
        return -1;
    }

    // writes received id & project to repo file
    repo_id = decode_uint32_t(buffer + 2);
    encode_uint32_t(entry, repo_id);
    encode_str(entry + sizeof(uint32_t), project_name);
    write(info.file_d, entry, sizeof(uint32_t) + strlen(project_name));

    printf("[+] Successfully created repo [id: %d][%s]\n", repo_id, project_name);

    // cleanup
    close_file(info);

    return repo_id;
}