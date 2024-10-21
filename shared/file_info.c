#include "file_info.h"


file_info fopen_file(int fd, int mem_flags) {
    file_info file_i;
    file_i.mem_flags = mem_flags;
    file_i.file_d = fd;

    if (fstat(file_i.file_d, &file_i.file_s) == -1) {
        fprintf(stderr, "[-] Failed to stat given file");
        perror("");
        exit(0);
    }
    
    if (mem_flags == MEM_DIRECT) {
        file_i.file = (char *)mmap(NULL, file_i.file_s.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_i.file_d, 0);
    } else {
        file_i.file = (char *)mmap(NULL, file_i.file_s.st_size, PROT_READ, MAP_PRIVATE, file_i.file_d, 0);
    }

    // creates a copy of the file, making it unafffected by trunc etc.
    if (mem_flags == MEM_COPY) {
        char *file_copy = (char *)calloc(file_i.file_s.st_size + 1, sizeof(char));

        // crude empty file managing
        if (file_i.file_s.st_size > 0) {
            memcpy(file_copy, file_i.file, file_i.file_s.st_size + 1);
        } else {
            file_copy[0] = '\0';
        }
        
        // removes original
        munmap(file_i.file, file_i.file_s.st_size);
        // replaces original
        file_i.file = file_copy;
    }

    return file_i;
}

file_info open_file(char *src_file, int d_flags, int mem_flags) {
    int file_d = open(src_file, d_flags | O_CREAT, 0777);

    if (file_d == -1) {
        fprintf(stderr, "[-] failed to open file[%s]: ", src_file);
        perror("");
        exit(0);
    }

    return fopen_file(file_d, mem_flags);
}

void close_file(file_info file_i) {
    if (file_i.mem_flags == MEM_COPY) {
        free(file_i.file);
    } else {
        munmap(file_i.file, file_i.file_s.st_size);
    }
    close(file_i.file_d);
}

// converts file_info to row_file
row_file row_file_create(file_info file_i, int r_flags) {
    uint32_t row_count = 1;
    uint32_t row_capacity = 20;
    char **tracked_files = calloc(row_capacity, sizeof(char *));
    char *file = file_i.file;

    row_file rf;
    rf.r_flags = r_flags;
    if (r_flags == ROW_COPY) {
        file = calloc(file_i.file_s.st_size + 1, sizeof(char));
        memcpy(file, file_i.file, file_i.file_s.st_size + 1);
    }

    // converts the copied file to an array of strings;
    tracked_files[0] = file;
    for (uint32_t i = 1; i < file_i.file_s.st_size; i++) {
        if (row_count == row_capacity - 1) {
            row_capacity *= 4;
            tracked_files = realloc(tracked_files, row_capacity * sizeof(char *));
        }

        // replaces new tracked_files with string ends, turning each row to a string
        if (file[i] == '\n') {
            file[i] = '\0';
            tracked_files[row_count++] = &file[i + 1];
        }
    }

    rf.rows = tracked_files;
    rf.row_count = row_count;

    return rf;
}

void row_file_destroy(row_file row_f) {
    if (row_f.r_flags == ROW_COPY) {
        free(row_f.rows[0]);
    }
    free(row_f.rows);
}
#pragma endregion interface