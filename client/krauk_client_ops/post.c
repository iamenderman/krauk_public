#include <unistd.h>

#include "krauk_client_ops.h"
// gets the freqtable of file
void fill_file_freq_table(file_info file, uint32_t *frequency_table);
// writes hash of to_be_hased to fd
void write_hash(FILE *fp, file_info to_be_hased, uint16_t *non_hashables);
// hashes each of the tracked files - and saves hash to track file
FIlE_TYPE_TABLE rewrite_file_hashes();
// archives each tracked file.
void archive_tracked_files(row_file track_file_rowifed, FIlE_TYPE_TABLE ft_table);

int krauk_post(KRAUK_FD kc, CLIENT_CTX *ctx, uint32_t repo_id, char *msg) {
    // net
    uint8_t buffer[LARGE_NET_BUFFER] = {0};
    // file managing
    uint8_t hashed_file_path[FILENAME_MAX + 1];
    file_info temp_file;
    file_info track_file;
    row_file track_file_rowifed;
    // file type table managing
    FIlE_TYPE_TABLE ft_table;
    file_info ft_table_file;
    int32_t ft_table_fd;
    uint8_t *ft_table_serialized;
    uint32_t ft_table_serialized_len = 0;

    // manages hasing of files and ft_table
    ft_table = rewrite_file_hashes();
    ft_table = FIlE_TYPE_TABLE_build_huffman(ft_table);

    // reads the diffrent tracked files
    track_file = open_file(C_TRACKED_FILE, O_RDWR, MEM_COPY);
    track_file_rowifed = row_file_create(track_file, ROW_COPY);

    // archive each tracked file to a file named after the filehash itself
    archive_tracked_files(track_file_rowifed, ft_table);

    // encodes table and writes it to freq file
    ft_table_serialized = FIlE_TYPE_TABLE_seralize_table(ft_table, &ft_table_serialized_len);
    ft_table_fd = open(C_TABLE_FILE, O_RDWR | O_CREAT | O_TRUNC, 0777);
    write(ft_table_fd, ft_table_serialized, ft_table_serialized_len);

    // cleanup after write
    FIlE_TYPE_TABLE_free(ft_table);
    free(ft_table_serialized);

    // create msg
    buffer[0] = POST_FILES;
    encode_uint32_t(buffer + 1, repo_id);
    if (msg != NULL) {
        encode_str(buffer + 1 + sizeof(uint32_t), msg);
    }

    // sends the project post msg
    if (krauk_send(kc, ctx, buffer, WS_MSG) == -1) {
        return -1;
    }
    puts("[+] Sent post request");

    // sends freq file
    if (krauk_send_file(kc, ctx, track_file) == -1) {
        return -1;
    }
    printf("[+] Sent: %s\n", C_TRACKED_FILE);

    // sends the freq_table
    ft_table_file = fopen_file(ft_table_fd, MEM_DEFAULT);
    if (krauk_send_file(kc, ctx, ft_table_file) == -1) {
        return -1;
    }
    printf("[+] Sent: %s\n", C_TABLE_FILE);
    close_file(ft_table_file);

    // sends the tracked-compreseed files
    for (size_t c = 0; c < track_file_rowifed.row_count; c += 2) {
        if (strcmp(track_file_rowifed.rows[c + 1], KRAUK_EMPTY_FILE) == 0) {
            continue;
        }

        // creates path to hahed file
        sprintf(hashed_file_path, "%s%s", C_HASHED_FILES_DIR, track_file_rowifed.rows[c + 1]);
        // opens ands sends it
        temp_file = open_file(hashed_file_path, O_RDONLY, MEM_DEFAULT);
        if (krauk_send_file(kc, ctx, temp_file) == -1) {
            puts("[-] File transfer failed");
            return -1;
        }


        printf("[+] Sent: %s\n", hashed_file_path);
        // cleanup
        close_file(temp_file);
        BZERO(hashed_file_path, FILENAME_MAX + 1);
    }

    // cleanup
    row_file_destroy(track_file_rowifed);
    close_file(track_file);

    return 0;
}

void fill_file_freq_table(file_info file, uint32_t *frequency_table) {
    // resets the buffer
    BZERO(frequency_table, 256 * sizeof(uint32_t));

    // filling frequency table
    for (int i = 0; i < file.file_s.st_size; i++) {
        if (file.file[i] == EOF) {
            break;
        }

        if (frequency_table[(uint8_t)file.file[i]] < UINT32_MAX - 1) {  // overflow managing.
            frequency_table[(uint8_t)file.file[i]]++;
        }
    };
}

void write_hash(FILE *fp, file_info to_be_hased, uint16_t *non_hashables) {
    char res[21];  // hash storage
    char hexed_file_name[56] = C_HASHED_FILES_DIR;

    /*
    TODO: check if new sha1 library is better
        sha1 lib got bugs when hasing large files -> max size for hashing is introduces ~1.25mb
        this needs to be fixed!!
    */
    if (to_be_hased.file_s.st_size + 1 > 10000000) {
        sprintf(hexed_file_name + 13, "%s%d", C_NONE_HASHABLE, (*non_hashables)++);
        // ignores the relative path to the file(+13), a bit hacky
        fwrite(hexed_file_name + 13, strlen(hexed_file_name) - 13, 1, fp);
        return;
    }

    // creates file-name via hashing
    SHA1(to_be_hased.file, to_be_hased.file_s.st_size + 1, res);

    // turns the hash into a user/filesystem readable name
    for (size_t offset = 0; offset < 20; offset++) {
        sprintf(((hexed_file_name + 13) + (2 * offset)), "%02x", res[offset] & 0xff);
    }

    // write the new hash, of the current file. ignores the relative path to the file(+13)
    fwrite(hexed_file_name + 13, 40, 1, fp);
}

FIlE_TYPE_TABLE rewrite_file_hashes() {
    // file managing
    file_info to_be_hased;
    file_info track_file;
    row_file track_file_rowifed;
    FILE *fp;
    // file type table managing
    FIlE_TYPE_TABLE ft_table;
    uint32_t table[256] = {0};
    // exernal hash managing
    uint16_t non_hashables = 0;

    // open and parses tracked file
    track_file = open_file(C_TRACKED_FILE, O_RDWR, MEM_COPY);
    // checks if file is empty
    if (track_file.file_s.st_size == 0) {
        fprintf(stderr, "[-] Track file empty\n");
        exit(EXIT_FAILURE);
    }
    track_file_rowifed = row_file_create(track_file, ROW_COPY);
    close_file(track_file);

    ft_table = FIlE_TYPE_TABLE_new();
    fp = fopen(C_TRACKED_FILE, "w+");  // ease of printing to hash file.
    for (size_t c = 0; c < track_file_rowifed.row_count; c += 2) {
        // removes non-existing files.
        if (access(track_file_rowifed.rows[c], F_OK) == -1) {
            printf("[+] Untracked removed file: %s\n", track_file_rowifed.rows[c]);
            continue;
        }

        // re-writes the current file path,
        fprintf(fp, "%s\n", track_file_rowifed.rows[c]);
        fprintf(stdout, "%s\n", track_file_rowifed.rows[c]);

        // opens the current file
        to_be_hased = open_file(track_file_rowifed.rows[c], O_RDONLY, MEM_DEFAULT);

        // manages empty files
        if (to_be_hased.file_s.st_size == 0) {
            fprintf(fp, "%s", KRAUK_EMPTY_FILE);
        } else {
            // writes the hash of 'to_be_hashed' to file_d;
            write_hash(fp, to_be_hased, &non_hashables);
            // creates freq table and adds it to the file type table
            fill_file_freq_table(to_be_hased, table);
            ft_table = FIlE_TYPE_TABLE_update(ft_table, get_file_ending(track_file_rowifed.rows[c]), table);
        }

        // adds new row to the file, unless we're at the last entry of the file.
        if (c != track_file_rowifed.row_count - 2) {
            fprintf(fp, "\n");
        }

        close_file(to_be_hased);
    }

    row_file_destroy(track_file_rowifed);
    fclose(fp);

    return ft_table;
}

void archive_tracked_files(row_file track_file_rowifed, FIlE_TYPE_TABLE ft_table) {
    huffman_table *huffman_table;

    // packs each file
    for (size_t c = 0; c < track_file_rowifed.row_count; c += 2) {
        huffman_table = FIlE_TYPE_TABLE_get_huff(ft_table, get_file_ending(track_file_rowifed.rows[c]));
        // compresses the given file to the a new file with the hash as the name
        char hexed_file_name[56];
        sprintf(hexed_file_name, "%s%s", C_HASHED_FILES_DIR, track_file_rowifed.rows[c + 1]);

        archive_pack_file(huffman_table, track_file_rowifed.rows[c], hexed_file_name);
        printf("[+] %s archived to %s\n\n", track_file_rowifed.rows[c], track_file_rowifed.rows[c + 1]);
    }
}