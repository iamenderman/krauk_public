#include "path_construct.h"

void prep_info(PATH_BUILDER *pb);
void prep_home(PATH_BUILDER *pb);
void prep_project(PATH_BUILDER *pb);
void prep_ver(PATH_BUILDER *pb);
void prep_table(PATH_BUILDER *pb);
void prep_tracked(PATH_BUILDER *pb);
void prep_user(PATH_BUILDER *pb);

PATH_BUILDER *PATH_BUILDER_new(uint8_t *user_id, uint32_t repo_id, uint32_t version_id) {
    PATH_BUILDER *pb = calloc(1, sizeof(PATH_BUILDER));
    pb->user_id = user_id,
    pb->repo_id = repo_id;
    pb->version_id = version_id;

    return pb;
}

void PATH_BUILDER_update(PATH_BUILDER *pb, uint8_t *user_id, uint32_t repo_id, uint32_t version_id) {
    pb->repo_id = repo_id;
    pb->user_id = user_id;
    pb->version_id = version_id;
}

void PATH_BUILDER_free(PATH_BUILDER *pb) {
    if (pb->info != NULL) {
        free(pb->info);
    }
    if (pb->home != NULL) {
        free(pb->home);
    }
    if (pb->project != NULL) {
        free(pb->project);
    }
    if (pb->ver != NULL) {
        free(pb->ver);
    }
    if (pb->table != NULL) {
        free(pb->table);
    }
    if (pb->tracked != NULL) {
        free(pb->tracked);
    }
    if (pb->user != NULL) {
        free(pb->user);
    }
    if (pb->dynamic != NULL) {
        free(pb->dynamic);
    }

    free(pb);
}

uint8_t *PATH_BUILDER_static_dir(PATH_BUILDER *pb, int type_id) {
    uint8_t *output = NULL;

    switch (type_id) {
        case INFO:
            prep_info(pb);
            output = pb->info;
            break;
        case HOME:
            prep_home(pb);
            output = pb->home;
            break;
        case PROJECT:
            prep_project(pb);
            output = pb->project;
            break;
        case VERSION:
            prep_ver(pb);
            output = pb->ver;
            break;
        case TABLE:
            prep_table(pb);
            output = pb->table;
            break;
        case TRACKED:
            prep_tracked(pb);
            output = pb->tracked;
            break;
        case USER:
            prep_user(pb);
            output = pb->user;
            break;
    }

    return output;
}

/*
    /user/
         info
         repo_id/
                .proj
                version/
                       .tracked
                       .table
                       hashed
*/

uint8_t *PATH_BUILDER_dynamic_dir(PATH_BUILDER *pb, int type_id, void *arg) {
    uint8_t *output = NULL;

    switch (type_id) {
        case HASHED:
            if (pb->dynamic == NULL) {
                pb->dynamic = calloc(FILENAME_MAX + 1, sizeof(uint8_t));
            } else {
                memset(pb->dynamic, 0, FILENAME_MAX + 1);
            }
            sprintf(pb->dynamic, "%s/%s/%u/%u/%s", P_ROOT, pb->user_id, pb->repo_id, pb->version_id, (char *)arg);
            output = pb->dynamic;
            break;
    }

    return output;
}

void prep_info(PATH_BUILDER *pb) {
    if (pb->info == NULL) {
        pb->info = calloc(FILENAME_MAX + 1, sizeof(uint8_t));
        // root/user/.info
        sprintf(pb->info, "%s/%s/%s", P_ROOT, pb->user_id, P_INFO);
    }
}

void prep_home(PATH_BUILDER *pb) {
    if (pb->home == NULL) {
        pb->home = calloc(FILENAME_MAX + 1, sizeof(uint8_t));
        // root/user/repo/
        sprintf(pb->home, "%s/%s/%u", P_ROOT, pb->user_id, pb->repo_id);
    }
}

void prep_project(PATH_BUILDER *pb) {
    if (pb->project == NULL) {
        pb->project = calloc(FILENAME_MAX + 1, sizeof(uint8_t));
        // root/user/repo/.project
        sprintf(pb->project, "%s/%s/%u/%s", P_ROOT, pb->user_id, pb->repo_id, P_PROJECT);
    }
}

void prep_ver(PATH_BUILDER *pb) {
    if (pb->ver == NULL) {
        pb->ver = calloc(FILENAME_MAX + 1, sizeof(uint8_t));
        // root/user/repo/version
        sprintf(pb->ver, "%s/%s/%u/%u", P_ROOT, pb->user_id, pb->repo_id, pb->version_id);
    }
}

void prep_table(PATH_BUILDER *pb) {
    if (pb->table == NULL) {
        pb->table = calloc(FILENAME_MAX + 1, sizeof(uint8_t));
        // root/user/repo/version/.table
        sprintf(pb->table, "%s/%s/%u/%u/%s", P_ROOT, pb->user_id, pb->repo_id, pb->version_id, P_TABLE);
    }
}

void prep_tracked(PATH_BUILDER *pb) {
    if (pb->tracked == NULL) {
        pb->tracked = calloc(FILENAME_MAX + 1, sizeof(uint8_t));
        // root/user/repo/version/.tracked
        sprintf(pb->tracked, "%s/%s/%u/%u/%s", P_ROOT, pb->user_id, pb->repo_id, pb->version_id, P_TRACKED);
    }
}

void prep_user(PATH_BUILDER *pb) {
    if (pb->user == NULL) {
        pb->user = calloc(FILENAME_MAX + 1, sizeof(uint8_t));
        // root/user/repo/
        sprintf(pb->user, "%s/%s", P_ROOT, pb->user_id);
    }
}