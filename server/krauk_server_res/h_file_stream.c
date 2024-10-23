#include "internal_res.h"
#include "krauk_server_res.h"

int handle_hashed_file_stream(KRAUK_FD ksc, CLIENT_CTX *ctx, uint8_t *buffer) {
    PATH_BUILDER *pb;
    uint8_t *hashed;
    uint32_t repo_id;
    uint32_t version_id;
    uint32_t client_file_row;
    uint32_t *temp_table;
    file_info tracked_file;
    file_info freq_table_file;
    file_info temp_file;
    row_file rowifed_tracked;
    FIlE_TYPE_TABLE ft_table;

    repo_id = decode_uint32_t(buffer + 1);
    version_id = decode_uint32_t(buffer + sizeof(uint32_t) + 1);
    pb = PATH_BUILDER_new(ctx->id, repo_id, version_id);

    if (!validate_version(pb, repo_id, version_id)) {
        PATH_BUILDER_free(pb);
        return -1;
    }

    // sets up necessary files
    tracked_file = open_file(TRACKED_PATH(pb), O_RDWR, MEM_DEFAULT);
    freq_table_file = open_file(TABLE_PATH(pb), O_RDWR, MEM_DEFAULT);
    rowifed_tracked = row_file_create(tracked_file, ROW_COPY);

    // preps hashed file
    client_file_row = decode_uint32_t(buffer + (2 * sizeof(uint32_t)) + 1);
    hashed = PATH_BUILDER_dynamic_dir(pb, HASHED, rowifed_tracked.rows[client_file_row + 1]);

    // freq table setup
    ft_table = FIlE_TYPE_TABLE_deserialize_table(freq_table_file.file);
    temp_table = FIlE_TYPE_TABLE_get_table(ft_table, get_file_ending(rowifed_tracked.rows[client_file_row]));

    // builds path to hashed file
    archive_unpack_file(temp_table, hashed, GLOBAL_TEMP_FILE);
    temp_file = open_file(GLOBAL_TEMP_FILE, O_RDWR, MEM_DEFAULT);

    // sends file to client
    krauk_send_file(ksc, ctx, temp_file);

    // cleanup
    if (remove(GLOBAL_TEMP_FILE) == -1) {
        puts("[!] Failed to remove temp file");
    }

    // cleanup
    close_file(tracked_file);
    close_file(freq_table_file);
    close_file(temp_file);
    FIlE_TYPE_TABLE_free(ft_table);
    row_file_destroy(rowifed_tracked);
    PATH_BUILDER_free(pb);

    return 0;
}

int handle_file_stream(KRAUK_FD ksc, CLIENT_CTX *ctx, uint8_t *buffer) {
    PATH_BUILDER *pb;
    file_info temp_file;
    uint32_t repo_id = 0;
    uint32_t version_id = 0;
    uint8_t response[BUFFER_SIZE];
    uint8_t *to_be_sent;
    uint8_t f;

    switch (buffer[1]) {
        case FREQ_FILE:
            repo_id = decode_uint32_t(buffer + 2);
            version_id = decode_uint32_t(buffer + 2 + sizeof(uint32_t));
            f = TABLE;
            break;
        case TRACKED_FILE:
            repo_id = decode_uint32_t(buffer + 2);
            version_id = decode_uint32_t(buffer + 2 + sizeof(uint32_t));
            f = TRACKED;
            break;
        case PROJECT_FILE:
            repo_id = decode_uint32_t(buffer + 2);
            f = PROJECT;
            break;
    }

    // creates ack
    pb = PATH_BUILDER_new(ctx->id, repo_id, version_id);
    to_be_sent = PATH_BUILDER_static_dir(pb, f);
    response[0] = POST_FILES;
    response[1] = access(to_be_sent, F_OK) == 0 ? REQUEST_VALID : REQUEST_ERROR;

    // if sending failed
    if (krauk_send(ksc, ctx, response, WS_MSG) == -1) {
        PATH_BUILDER_free(pb);
        return -1;
    }

    // smelly - check if ack was bad
    if (response[1] == REQUEST_ERROR) {
        puts("[!] Client requested non-existing file");
        return -1;
    }

    // opens and sends file
    temp_file = open_file(to_be_sent, O_RDWR, MEM_DEFAULT);
    if (krauk_send_file(ksc, ctx, temp_file) == -1) {
        close_file(temp_file);
        PATH_BUILDER_free(pb);
        return -1;
    }

    close_file(temp_file);
    PATH_BUILDER_free(pb);

    return 0;
}