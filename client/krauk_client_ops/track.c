#include "krauk_client_ops.h"

// own defines due to problems with dirent.h defines
#define DIRENT_FILE 8
#define DIRENT_DIR 4

// add all of the elemets of a file tree starting from the root relative_path
string_stack *file_tree(string_stack *stack, char *relative_path);

int krauk_track(char *path) {
    // path storage
    string_stack *stack;
    // file managing
    file_info track_file;
    FILE *fd; // for ease of printing

    stack = string_stack_create();
    track_file = open_file(C_TRACKED_FILE, O_RDWR | O_TRUNC, MEM_DEFAULT);

    file_tree(stack, path);

    fd = fdopen(track_file.file_d, "w");
    for (size_t i = 0; i < stack->size; i++) {
        fprintf(fd, "%s\n", stack->stack[i]);
        // creates a blank row every other row - used for the hash of the file.
        if (i != stack->size - 1) {
            fprintf(fd, "\n");
        }
    }

    // clean up
    fclose(fd);
    close_file(track_file);
 
    string_stack_destroy(stack);

    return 0;
}

string_stack *file_tree(string_stack *stack, char *relative_path) {
    struct dirent *d_entry;
    DIR *d_stream = opendir(relative_path);

    if (!d_stream) {
        fprintf(stderr, "[-] could not open directory: %s\n", relative_path);
        return stack;
    }

    while ((d_entry = readdir(d_stream)) != NULL) {
        if (strcmp(d_entry->d_name, ".") == 0 ||
            strcmp(d_entry->d_name, "..") == 0 ||
            strstr(d_entry->d_name, ".krauk")) {
            continue;
        }

        char new_dir[4096];
        strcpy(new_dir, relative_path);
        strcat(new_dir, "/");
        strcat(new_dir, d_entry->d_name);

        switch (d_entry->d_type) {
            case DIRENT_DIR:
                stack = file_tree(stack, new_dir);
                break;
            case DIRENT_FILE:
                printf("[+] Tracking %s\n", new_dir);
                stack = string_stack_append(stack, new_dir);
                break;
        }
    }

    closedir(d_stream);
    return stack;
}
