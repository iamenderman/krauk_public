#include "krauk_client_ops.h"

int krauk_list_repos(KRAUK_FD kc, CLIENT_CTX *ctx) {
    // logging-temps
    uint8_t temp_name_size;
    uint8_t buffer[LARGE_NET_BUFFER];
    uint8_t temp_name[MSG_SIZE] = {0};
    uint32_t temp_id;
    uint64_t temp_version;
    // file manging
    file_info rlist;
    struct stat st = {0};

    // hacky - creates folder if needed
    if (stat(C_HOME_DIR, &st) == -1) {
        mkdir(C_HOME_DIR, 0700);
    }

    // creates request - sends it
    buffer[0] = LIST_REPOSITORIES;
    if(krauk_send(kc, ctx, buffer, WS_MSG) == -1) {
        return -1;
    }
    
    // recives and opens file
    if(krauk_recive_file(kc, ctx, C_LISTED_REPOS_FILE) == -1) {
        return -1;
    }
    rlist = open_file(C_LISTED_REPOS_FILE, O_RDONLY, MEM_DEFAULT);

    printf("~~~~~~~~~~~~repos~~~~~~~~~~~~~~\n");
    for (size_t i = sizeof(uint32_t); i < rlist.file_s.st_size; i += 1 + sizeof(uint64_t) + sizeof(uint32_t) + temp_name_size) {
        temp_id = decode_uint32_t(rlist.file + i);
        temp_version = decode_uint64_t(rlist.file + i + sizeof(uint32_t));
        temp_name_size = rlist.file[sizeof(uint32_t) + sizeof(uint64_t) + i];
        memcpy(temp_name, rlist.file + 1 + sizeof(uint32_t) + sizeof(uint64_t) + i, temp_name_size);

        printf("[%d][%ld]: %s\n", temp_id, temp_version, temp_name);
        BZERO(temp_name, temp_name_size);
    }
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");

    // cleanup
    if (remove(C_LISTED_REPOS_FILE) == -1) {
        perror("[-] Failed to remove temp repo file");
    }

    close_file(rlist);

    return 0;
}
