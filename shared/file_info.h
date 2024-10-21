#pragma once

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MEM_DEFAULT 1
#define MEM_COPY 2
#define MEM_DIRECT 3

#define ROW_DEFAULT 1
#define ROW_COPY 2

// writes 0(UIN32_T) to f  \0
#define WRITE_UINT32_T_ZERO(x) write(x, "\0\0\0\0", sizeof(uint32_t))
#define WRITE_UINT64_T_ZERO(x) write(x, "\0\0\0\0\0\0\0\0", sizeof(uint32_t))



typedef struct {
    int r_flags;
    uint32_t row_count;
    char** rows;
} row_file;

typedef struct {
    struct stat file_s;
    int64_t file_d;
    uint32_t mem_flags;
    char* file;
} file_info;

// mmaps, stats a given file
// d_flags = des flags
// m_flags = memory flags
file_info open_file(char* src_file, int d_flags, int mem_flags);

// mmaps, stats a given file
// d_flags = des flags
// m_flags = memory flags
file_info fopen_file(int fd, int mem_flags);

// converts file_info to row_file
row_file row_file_create(file_info file_i, int r_flags);

void row_file_destroy(row_file row_f);

// destroys the given file
void close_file(file_info file_i);