#pragma once

#include <dirent.h>
#include <openssl/sha.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../shared/archive.h"
#include "../../shared/file_info.h"
#include "../../shared/file_type_table.h"
#include "../../shared/krauk_connection.h"
#include "../../shared/ser.h"
#include "../../shared/string_stack.h"

#define C_NONE_HASHABLE "NONE_"
#define C_HOME_DIR ".krauk/"
#define C_HASHED_FILES_DIR ".krauk/files/"
#define C_TRACKED_FILE ".krauk/tracked"
#define C_TABLE_FILE ".krauk/table"
#define C_INFO_FILE ".krauk/krauk"
#define C_LISTED_REPOS_FILE "./krauk/repo_list"
#define C_TEMP_PUBLIC ".krauk/public_key.pem"

int krauk_post(KRAUK_FD kc, CLIENT_CTX *ctx, uint32_t repo_id, char *msg);
int krauk_pull(KRAUK_FD kc, CLIENT_CTX *ctx, uint32_t repo_id);
int32_t krauk_new_repo(KRAUK_FD kc, CLIENT_CTX *ctx, char *project_name);
int krauk_request_file(KRAUK_FD kc, CLIENT_CTX *ctx);
int krauk_list_repos(KRAUK_FD kc, CLIENT_CTX *ctx);
int krauk_track(char *path);
