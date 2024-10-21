
#pragma once
#include <stdint.h>
#include <string.h>

// TODO: rework shared constant file 
// TODO: rename 
#define KRAUK_EMPTY_FILE "EMPTY_FILE"

#define MAX_INPUT_SIZE 195

#define LIST_VERSIONS 1
#define LIST_REPOSITORIES 2
#define PULL_REPOSITORY 3
#define PULL_REPOSITORY_VERSION 4
#define POST_FILES 5
#define CREATE_PROJECT 6
#define FILE_HASH_STREAM 7
#define FILE_STREAM 8

// ack managing
#define REQUEST_ERROR 0
#define REQUEST_VALID 1

#define FREQ_FILE 1
#define TRACKED_FILE 2
#define PROJECT_FILE 3


#define PORT 3254
#define HOST "127.0.0.1"

#define BZERO(b,len) (memset((b), '\0', (len)), (void) 0)