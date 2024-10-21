#pragma once

#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// connection = user file, init_connection

// connection 
#define CMD_LIST 2
// connection 
#define CMD_NEW 3
// krauk system
#define CMD_TRACK 4
// krauk system & connection
#define CMD_PULL 5
// krauk system & connection
#define CMD_POST 6
// inget 
#define CMD_REGISTER 7
#define CMD_INVALID 8

typedef struct {
    // flags settings
    bool append;
    bool remove;
    char *msg;
    // command managing
    char **arg;
    int argc;
    int cmd;
    // client identification
    char config_dir[FILENAME_MAX + 1];
    char config_file[FILENAME_MAX + 1];

} LAUNCH_SETTINGS;

LAUNCH_SETTINGS LAUNCH_SETTINGS_construct(int argc, char **argv);
