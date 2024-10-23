#include "krauk_client_ops.h"

int krauk_pull(KRAUK_FD kc, CLIENT_CTX *ctx, uint32_t repo_id) {
    // net
    uint8_t buffer[LARGE_NET_BUFFER];
    // file managing
    int32_t empty_file_fd;
    uint8_t hashed_file_path[FILENAME_MAX + 1];
    file_info temp_file;
    row_file tracked_rowifed;
    // file unpacking
    FIlE_TYPE_TABLE ft_table;
    uint32_t *table;

    // creates request & sends it
    buffer[0] = PULL_REPOSITORY;
    encode_uint32_t(buffer + 1, repo_id);
    if (krauk_send(kc, ctx, buffer, WS_MSG) == -1) {
        return -1;
    }

    // gets the tracked file
    if (krauk_receive_file(kc, ctx, C_TRACKED_FILE) == -1) {
        return -1;
    }
    temp_file = open_file(C_TRACKED_FILE, O_RDONLY, MEM_DEFAULT);
    tracked_rowifed = row_file_create(temp_file, ROW_COPY);
    close_file(temp_file);

    // gets the freq table file
    if (krauk_receive_file(kc, ctx, C_TABLE_FILE) == -1) {
        return -1;
    }
    temp_file = open_file(C_TABLE_FILE, O_RDWR, MEM_DEFAULT);
    ft_table = FIlE_TYPE_TABLE_deserialize_table(temp_file.file);
    close_file(temp_file);

    for (uint32_t c = 0; c < tracked_rowifed.row_count; c += 2) {
        // if the file is empty - create or truncate
        if (strcmp(tracked_rowifed.rows[c + 1], KRAUK_EMPTY_FILE) == 0) {
            if (empty_file_fd == -1) {
                fprintf(stderr, "[-] Failed to create file: %s", tracked_rowifed.rows[c]);
            } else {
                close(empty_file_fd);
            }
            continue;
        }

        // creates path to hashed file
        sprintf(hashed_file_path, "%s%s", C_HASHED_FILES_DIR, tracked_rowifed.rows[c + 1]);

        // wait for the file
        if (krauk_receive_file(kc, ctx, hashed_file_path) == -1) {
            return -1;
        }
        table = FIlE_TYPE_TABLE_get_table(ft_table, get_file_ending(tracked_rowifed.rows[c]));
        // unpacks the newly received file
        archive_unpack_file(table, hashed_file_path, tracked_rowifed.rows[c]);

        BZERO(hashed_file_path, FILENAME_MAX + 1);
    }

    // cleanup
    FIlE_TYPE_TABLE_free(ft_table);
    row_file_destroy(tracked_rowifed);

    return 0;
}
