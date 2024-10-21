#include "internal_res.h"
#include "krauk_server_res.h"

// [version_id][time][msg len][msg]
int handle_create_repo(KRAUK_FD ksc, CLIENT_CTX *ctx, uint8_t *buffer) {
    // file path building and managing
    struct stat st = {0};
    PATH_BUILDER *pb;
    // info file managing
    uint8_t entry_buffer[ENTRY_SIZE] = {0};
    uint8_t entry_size;
    uint8_t *project_name;
    uint32_t repo_id;
    file_info info;
    int32_t info_fd;

    // gets and copies project name
    project_name = calloc(strlen(buffer + 1) + 1, sizeof(char));
    strcpy(project_name, buffer + 1);
    BZERO(buffer, BUFFER_SIZE);

    // sets up pathbuilder
    pb = PATH_BUILDER_new(ctx->id, 0, 0);

    // if the user haven't creates a repo before - create folder
    printf("trying to create %s\n", USER_PATH(pb));
    if (stat(USER_PATH(pb), &st) == -1) {
        if (mkdir(USER_PATH(pb), 0700)) {
            perror("failed to create user repo");

            // cleanup
            PATH_BUILDER_free(pb);
            free(project_name);
            return -1;
        }

        // inits the info file
        info_fd = open(INFO_PATH(pb), O_CREAT | O_RDWR, 0777);
        WRITE_UINT32_T_ZERO(info_fd);
        close(info_fd);
    }

    // re-opens user info file
    info = open_file(INFO_PATH(pb), O_RDWR | O_APPEND, MEM_DIRECT);

    // checks if name is already takem
    for (size_t i = sizeof(uint32_t); i < info.file_s.st_size;) {
        i += sizeof(uint32_t) + sizeof(uint64_t);

        // length check
        if (info.file[i] != strlen(project_name)) {
            continue;
        }

        // contents check
        if (memcmp(info.file + i + 1, project_name, info.file[i]) == 0) {
            fprintf(stderr, "[-] Request denied, name [%s] taken \n", project_name);

            // sets up pacakge
            encode_sequence(buffer, 2, CREATE_PROJECT, REQUEST_ERROR);
            encode_str(buffer + 2, "Project name taken");

            if (krauk_send(ksc, ctx, buffer, WS_MSG) == -1) {

                // cleanup
                close_file(info);
                free(project_name);
                PATH_BUILDER_free(pb);

                return -1;
            }

            BZERO(buffer, BUFFER_SIZE);

            goto free;
        }
    }

    // creates new repo id and overwrites old id
    repo_id = decode_uint32_t(info.file) + 1;
    encode_uint32_t(info.file, repo_id);

    // updates path buffer with the newly created id
    PATH_BUILDER_update(pb, ctx->id, repo_id, 0);

    // write entry to info file
    entry_size = encode_timed_numbered_msg(entry_buffer, repo_id, project_name);
    write(info.file_d, entry_buffer, entry_size);

    // creates dir for new repo
    if (stat(HOME_PATH(pb), &st) == -1) {
        mkdir(HOME_PATH(pb), 0700);
    }

    // check if project fil already exists
    if (access(PROJECT_PATH(pb), F_OK) == -1) {
        int fd = open(PROJECT_PATH(pb), O_RDWR | O_CREAT, 0777);
        WRITE_UINT32_T_ZERO(fd);
        close(fd);
    }

    // creates and sends repo ack.
    encode_sequence(buffer, 2, CREATE_PROJECT, REQUEST_VALID);
    encode_uint32_t(buffer + 2, repo_id);
    if (krauk_send(ksc, ctx, buffer, WS_MSG) == -1) {
        close_file(info);
        free(project_name);
        PATH_BUILDER_free(pb);

        return -1;
    }

    puts("[+] Successfully created repo");

free:
    close_file(info);
    free(project_name);
    PATH_BUILDER_free(pb);

    return 0;
}