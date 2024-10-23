#include <arpa/inet.h>
#include <assert.h>
#include <dirent.h>
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "../shared/archive.h"
#include "../shared/cipher.h"
#include "../shared/enviroment.h"
#include "../shared/file_info.h"
#include "../shared/file_type_table.h"
#include "../shared/krauk_connection.h"
#include "krauk_client_ops/krauk_client_ops.h"
#include "launch_options.h"

uint32_t validate_krauk_system();
void validate_self(LAUNCH_SETTINGS ls);
void register_user_context(LAUNCH_SETTINGS ls, uint8_t *file_path);
void setup_file_structure();
void init_connection(LAUNCH_SETTINGS ls, KRAUK_FD *server, SERVER_CTX **s_ctx, CLIENT_CTX **c_ctx);
void destroy_connection(KRAUK_FD server, SERVER_CTX *s_ctx, CLIENT_CTX *c_ctx);

int main(int argc, char **argv) {
    LAUNCH_SETTINGS ls;
    CLIENT_CTX *c_ctx = NULL;
    SERVER_CTX *s_ctx = NULL;
    KRAUK_FD server = 0;
    uint32_t repo_id = 0;
    int res = 0;

    // parses user input
    ls = LAUNCH_SETTINGS_construct(argc, argv);

    // checks if user enterd valid command
    if (ls.cmd == CMD_INVALID || argc == 1) {
        puts("[-] Usage: krauk [new, track, list, pull, post, register] [args ...]");
        return 1;
    }

    // TODO: clean up ccode
    switch (ls.cmd) {
        case CMD_LIST:
            validate_self(ls);
            init_connection(ls, &server, &s_ctx, &c_ctx);
            res = krauk_list_repos(server, c_ctx) == -1;
            destroy_connection(server, s_ctx, c_ctx);
            break;
        case CMD_NEW:
            validate_self(ls);
            setup_file_structure();
            init_connection(ls, &server, &s_ctx, &c_ctx);
            res = krauk_new_repo(server, c_ctx, ls.arg[0]);
            destroy_connection(server, s_ctx, c_ctx);
            break;
        case CMD_TRACK:
            validate_krauk_system(ls);
            krauk_track(ls.arg[0]);
            break;
        case CMD_PULL:
            validate_self(ls);
            repo_id = validate_krauk_system(ls);
            init_connection(ls, &server, &s_ctx, &c_ctx);
            res = krauk_pull(server, c_ctx, repo_id);
            destroy_connection(server, s_ctx, c_ctx);
            break;
        case CMD_POST:
            validate_self(ls);
            repo_id = validate_krauk_system(ls);
            init_connection(ls, &server, &s_ctx, &c_ctx);
            res = krauk_post(server, c_ctx, repo_id, ls.msg);
            destroy_connection(server, s_ctx, c_ctx);
            break;
        case CMD_REGISTER:
            register_user_context(ls, ls.arg[0]);
            break;
        case CMD_ENV:
            printf("env: %s\n", ls.env_file);
            break;
    }

    return res == 0 ? 0 : 1;
}

uint32_t validate_krauk_system() {
    file_info info;
    int repo_id;

    // opens info file & validates contents
    info = open_file(C_INFO_FILE, O_RDWR, MEM_DEFAULT);
    if ((info.file_s.st_size < sizeof(uint32_t)) ||
        (info.file_s.st_size >= sizeof(uint32_t) && decode_uint32_t(info.file) == 0)) {
        fprintf(stderr, "[-] No valid krauk repo found! Create repo using \"krauk new [filename]\"\n");
        close_file(info);
        exit(EXIT_FAILURE);
    }

    repo_id = decode_uint32_t(info.file);
    close_file(info);

    return repo_id;
}

void validate_self(LAUNCH_SETTINGS ls) {
    struct stat st = {0};

    // validates user context
    if (access(ls.config_file, F_OK) == -1 || stat(ls.config_dir, &st) == -1) {
        fprintf(stderr, "[AAGGHHHH] no krauk user found, get one from server!\n");
        exit(EXIT_FAILURE);
    }
}

void init_connection(LAUNCH_SETTINGS ls, KRAUK_FD *server, SERVER_CTX **s_ctx, CLIENT_CTX **c_ctx) {
    ENV *env;
    // default host
    char host[17] = HOST;
    int port = PORT;
    // env values
    char *env_host = NULL;
    char *env_port = NULL;

    // reads env file
    env = ENV_read(ls.env_file);
    if (env != NULL) {
        // get envs
        env_host = ENV_get(env, "HOST");
        env_port = ENV_get(env, "PORT");

        if (env_host != NULL && strlen(env_host) < 16) {
            strcpy(host, env_host);
        }
        if (env_port != NULL && atoi(env_port) != 0) {
            port = atoi(env_port);
        }
        ENV_free(env);
    }

    *server = krauk_client_connect(host, port);
    init_SERVER_CLIENT_CTX(ls.config_file, s_ctx, c_ctx);
    krauk_send_header(*server, *c_ctx, *s_ctx);
}

void destroy_connection(KRAUK_FD server, SERVER_CTX *s_ctx, CLIENT_CTX *c_ctx) {
    SERVER_CTX_free(s_ctx);
    CLIENT_CTX_free(c_ctx);
    krauk_client_close(server);
}

void setup_file_structure() {
    struct stat st = {0};

    // creates krauk home dir in the given folder
    if (stat(C_HOME_DIR, &st) == -1) {
        mkdir(C_HOME_DIR, 0700);
    }
    // creates a dir for compressed files inside of the home dir
    if (stat(C_HASHED_FILES_DIR, &st) == -1) {
        mkdir(C_HASHED_FILES_DIR, 0700);
    }
}

void register_user_context(LAUNCH_SETTINGS ls, uint8_t *file_path) {
    struct stat st = {0};
    file_info to_be_registered;
    int config_fd;

    if (stat(ls.config_dir, &st) == -1) {
        mkdir(ls.config_dir, 0777);
    }

    to_be_registered = open_file(file_path, O_RDONLY, MEM_DEFAULT);
    config_fd = open(ls.config_file, O_RDWR | O_CREAT, 0777);

    write(config_fd, to_be_registered.file, to_be_registered.file_s.st_size);
    close_file(to_be_registered);
    close(config_fd);

    printf("[+] Successfully registered user file [%s]\n", ls.config_file);
}
