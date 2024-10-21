#include "internal_res.h"
#include "krauk_server_res.h"

int send_repo_latest_version(KRAUK_FD ksc, CLIENT_CTX *ctx, PATH_BUILDER *pb, uint32_t repo_id);
int send_repo(KRAUK_FD ksc, CLIENT_CTX *ctx, PATH_BUILDER *pb, uint32_t repo_id, uint32_t version_id);

int handle_pull_repo(KRAUK_FD ksc, CLIENT_CTX *ctx, uint8_t *buffer) {
    int res;
    uint32_t repo_id = decode_uint32_t(buffer + 1);
    PATH_BUILDER *pb = PATH_BUILDER_new(ctx->id, repo_id, 0);

    res = send_repo_latest_version(ksc, ctx, pb, repo_id);
    PATH_BUILDER_free(pb);

    return res;
}

int handle_pull_repo_version(KRAUK_FD ksc, CLIENT_CTX *ctx, uint8_t *buffer) {
    int res;
    uint32_t repo_id = decode_uint32_t(buffer + 1);
    uint32_t version_id = decode_uint32_t(buffer + sizeof(uint32_t) + 1);
    PATH_BUILDER *pb = PATH_BUILDER_new(ctx->id, repo_id, version_id);

    res = send_repo(ksc, ctx, pb, repo_id, version_id);
    PATH_BUILDER_free(pb);

    return res;
}

int send_repo_latest_version(KRAUK_FD ksc, CLIENT_CTX *ctx, PATH_BUILDER *pb, uint32_t repo_id) {
    file_info project_file;
    uint32_t last_version;

    if (!validate_id(pb, repo_id)) {
        return 1;
    }

    if (access(PROJECT_PATH(pb), F_OK) != 0) {
        return 1;
    }

    // gets the latest version_id
    project_file = open_file(PROJECT_PATH(pb), O_RDONLY, MEM_DEFAULT);
    last_version = decode_uint32_t(project_file.file);
    close_file(project_file);

    // updates PATH_BUILDER with the new version
    PATH_BUILDER_update(pb, ctx->id, repo_id, last_version);

    return send_repo(ksc, ctx, pb, repo_id, last_version);
}

int send_repo(KRAUK_FD ksc, CLIENT_CTX *ctx, PATH_BUILDER *pb, uint32_t repo_id, uint32_t version_id) {
    file_info temp_file;
    row_file tracked_rowifed;
    uint8_t *hashed;

    if (!validate_version(pb, repo_id, version_id)) {
        return 1;
    }

    //  sends tracked file
    temp_file = open_file(TRACKED_PATH(pb), O_RDWR, MEM_DEFAULT);
    tracked_rowifed = row_file_create(temp_file, ROW_COPY);
    if (krauk_send_file(ksc, ctx, temp_file) == -1) {
        close_file(temp_file);
        row_file_destroy(tracked_rowifed);
        return -1;
    }
    close_file(temp_file);

    // sends freqtable file
    temp_file = open_file(TABLE_PATH(pb), O_RDWR, MEM_DEFAULT);
    if (krauk_send_file(ksc, ctx, temp_file) == -1) {
        close_file(temp_file);
        row_file_destroy(tracked_rowifed);
        return -1;
    }
    close_file(temp_file);

    for (size_t c = 0; c < tracked_rowifed.row_count; c += 2) {
        if (strcmp(tracked_rowifed.rows[c + 1], KRAUK_EMPTY_FILE) == 0) {
            continue;
        }

        // creates path to file tracked_files_rowifed.rows[c + 1]
        hashed = PATH_BUILDER_dynamic_dir(pb, HASHED, tracked_rowifed.rows[c + 1]);
        temp_file = open_file(hashed, O_RDONLY, MEM_DEFAULT);

        if (krauk_send_file(ksc, ctx, temp_file) == -1) {
            close_file(temp_file);
            row_file_destroy(tracked_rowifed);
            return -1;
        }
        close_file(temp_file);
        printf("[+] Sent: %s\n", hashed);
    }

    // cleanup
    row_file_destroy(tracked_rowifed);

    return 0;
}