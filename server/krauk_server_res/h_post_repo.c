#include "internal_res.h"
#include "krauk_server_res.h"

int handle_post_repo(KRAUK_FD ksc, CLIENT_CTX *ctx, uint8_t *buffer) {  // gets id
    PATH_BUILDER *pb;
    uint8_t *hashed;
    uint8_t project_entry[MSG_SIZE + sizeof(uint64_t)];
    uint8_t project_entry_size;
    uint32_t repo_id;
    uint32_t version_id;
    file_info tracked_files;
    file_info version_file;
    row_file track_file_rowifed;
    struct stat st = {0};

    // sets up PATH_BUILDER
    repo_id = decode_uint32_t(buffer + 1);
    pb = PATH_BUILDER_new(ctx->id, repo_id, 0);

    if (!validate_id(pb, repo_id)) {
        // sends ack to client
        encode_sequence(buffer, 2, POST_FILES, REQUEST_ERROR);
        encode_str(buffer + 2, "Invalid repository repo_id.");

        // no error check needed - redundant
        krauk_send(ksc, ctx, buffer, WS_MSG);
        puts("[-] Received invalid repo_id.");
        PATH_BUILDER_free(pb);

        return -1;
    }

    // gets the current version_id repo_id & writes the new version_id back to the file
    version_file = open_file(PROJECT_PATH(pb), O_RDWR | O_APPEND, MEM_DIRECT);
    version_id = decode_uint32_t(version_file.file) + 1;
    encode_uint32_t(version_file.file, version_id);

    // write commit-update entry to info file
    project_entry_size = encode_timed_numbered_msg(project_entry, repo_id, buffer + sizeof(uint32_t) + 1);
    write(version_file.file_d, project_entry, project_entry_size);
    close_file(version_file);

    // updates path builder
    PATH_BUILDER_update(pb, ctx->id, repo_id, version_id);

    // creates home rdir for the new version_id
    if (stat(VERSION_PATH(pb), &st) == -1) {
        mkdir(VERSION_PATH(pb), 0700);
    }

    // gets track file
    if (krauk_receive_file(ksc, ctx, TRACKED_PATH(pb)) == -1) {
        PATH_BUILDER_free(pb);
        return -1;
    }

    tracked_files = open_file(TRACKED_PATH(pb), O_RDONLY, MEM_COPY);
    track_file_rowifed = row_file_create(tracked_files, ROW_DEFAULT);
    printf("[+] Recived file: %s\n", TRACKED_PATH(pb));

    // fetches the freq table
    if (krauk_receive_file(ksc, ctx, TABLE_PATH(pb)) == -1) {
        PATH_BUILDER_free(pb);
        return -1;
    }
    printf("[+] Received file: %s\n", TABLE_PATH(pb));

    for (size_t c = 0; c < track_file_rowifed.row_count; c += 2) {
        if (strcmp(track_file_rowifed.rows[c + 1], KRAUK_EMPTY_FILE) == 0) {
            continue;
        }

        hashed = PATH_BUILDER_dynamic_dir(pb, HASHED, track_file_rowifed.rows[c + 1]);
        if (krauk_receive_file(ksc, ctx, hashed) == -1) {
            PATH_BUILDER_free(pb);
            return -1;
        }

        printf("[+] Recived file: %s\n", hashed);
    }

    puts("[+] Successfully received update");
    PATH_BUILDER_free(pb);

    return 0;
}