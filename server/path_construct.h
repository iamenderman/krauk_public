#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// kanske inte vara här senare
#define P_ROOT "krauk"
#define P_USERS "krauk/users"
#define P_USERBASE "krauk/users/.userbase"
#define P_REPO "repos"
#define P_INFO ".info"
#define P_TABLE ".table"
#define P_TRACKED ".tracked"
#define P_PROJECT ".project"
#define P_TEMP_FILE ".temp"


// static
#define INFO 1
#define HOME 2
#define PROJECT 3
#define VERSION 4
#define TABLE 5
#define TRACKED 6
#define USER 7

// dynamic dir
#define HASHED 7

#define INFO_PATH(x) PATH_BUILDER_static_dir(x, INFO)
#define HOME_PATH(x) PATH_BUILDER_static_dir(x, HOME)
#define PROJECT_PATH(x) PATH_BUILDER_static_dir(x, PROJECT)
#define VERSION_PATH(x) PATH_BUILDER_static_dir(x, VERSION)
#define TABLE_PATH(x) PATH_BUILDER_static_dir(x, TABLE)
#define TRACKED_PATH(x) PATH_BUILDER_static_dir(x, TRACKED)
#define USER_PATH(x) PATH_BUILDER_static_dir(x, USER)


typedef struct {
    uint8_t *user_id;
    uint32_t repo_id;
    uint32_t version_id;
    uint8_t *info;
    uint8_t *home;
    uint8_t *project;
    uint8_t *ver;
    uint8_t *table;
    uint8_t *tracked;
    uint8_t *user;
    uint8_t *dynamic;
} PATH_BUILDER;

PATH_BUILDER *PATH_BUILDER_new(uint8_t *user_id, uint32_t repo_id, uint32_t version_id);
void PATH_BUILDER_free(PATH_BUILDER *pb);
void PATH_BUILDER_update(PATH_BUILDER *pb, uint8_t *user_id, uint32_t repo_id, uint32_t ver);

uint8_t *PATH_BUILDER_static_dir(PATH_BUILDER *pb, int type_id);
uint8_t *PATH_BUILDER_dynamic_dir(PATH_BUILDER *pb, int type_id, void *arg);
