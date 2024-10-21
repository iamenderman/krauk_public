#include "internal_res.h"

uint8_t encode_timed_numbered_msg(uint8_t *buffer, uint32_t num, char *str) {
    time_t sec;
    uint8_t write_size;
    uint8_t writable_size = ENTRY_SIZE - sizeof(uint32_t) - 1;

    // 194 = ENTRY_SIZE - sizeof(uint32_t)(num) - 1(strlength)
    write_size = strlen(str) > writable_size ? writable_size : strlen(str);
    encode_uint32_t(buffer, num);

    sec = time(NULL);
    encode_uint64_t(buffer + sizeof(uint32_t), sec);

    buffer[sizeof(uint64_t) + sizeof(uint32_t)] = write_size;
    memcpy(buffer + sizeof(uint64_t) + sizeof(uint32_t) + 1, str, write_size);

    // 5 = sizeof(uint32_t) + 1
    return sizeof(uint64_t) + sizeof(uint32_t) + 1 + write_size;
}

bool validate_id(PATH_BUILDER *pb, uint32_t repo_id) {
    file_info info_file = open_file(INFO_PATH(pb), O_RDONLY, MEM_DEFAULT);
    uint32_t max_id = decode_uint32_t(info_file.file);
    close_file(info_file);

    return repo_id <= max_id;
}

bool validate_version(PATH_BUILDER *pb, uint32_t repo_id, uint32_t version_id) {
    file_info project_file;
    uint32_t latset_version;

    if (!validate_id(pb, repo_id)) {
        return false;
    }

    if (access(PROJECT_PATH(pb), F_OK) != 0) {
        return false;
    }

    project_file = open_file(PROJECT_PATH(pb), O_RDONLY, MEM_DEFAULT);
    latset_version = decode_uint32_t(project_file.file);

    return version_id <= latset_version;
}