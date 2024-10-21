#pragma once

#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "../../shared/archive.h"
#include "../../shared/comms.h"
#include "../../shared/file_info.h"
#include "../../shared/file_type_table.h"
#include "../../shared/krauk_connection.h"
#include "../../shared/ser.h"

#include "../path_construct.h"
#include "../user.h"

#define GOBAL_TEMP_FILE "/krauk/temp"

int handle_new_user(SERVER_CTX *s_ctx, USER_BASE *ub);
int handle_create_repo(KRAUK_FD ksc, CLIENT_CTX *ctx, uint8_t *buffer);
int handle_post_repo(KRAUK_FD ksc, CLIENT_CTX *ctx, uint8_t *buffer);
int handle_list_repos(KRAUK_FD ksc, CLIENT_CTX *ctx);
int handle_pull_repo(KRAUK_FD ksc, CLIENT_CTX *ctx, uint8_t *buffer);
int handle_pull_repo_version(KRAUK_FD ksc, CLIENT_CTX *ctx, uint8_t *buffer);
int handle_hased_file_stream(KRAUK_FD ksc, CLIENT_CTX *ctx, uint8_t *buffer);
int handle_file_stream(KRAUK_FD ksc, CLIENT_CTX *ctx, uint8_t *buffer);