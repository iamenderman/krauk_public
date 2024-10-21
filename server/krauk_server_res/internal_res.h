#pragma once

#include "krauk_server_res.h"

uint8_t encode_timed_numbered_msg(uint8_t *buffer, uint32_t num, char *str);
bool validate_version(PATH_BUILDER *pb, uint32_t repo_id, uint32_t version_id);
bool validate_id(PATH_BUILDER *pb, uint32_t repo_id);