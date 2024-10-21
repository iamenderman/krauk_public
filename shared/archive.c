#include "archive.h"

// #pragma region assist
// void log(uint8_t byte) {
//     uint128_t mask = ((uint128_t)1) << 7;
//     for (uint8_t i = 0; i < 7; i++) {
//         printf("%c", ((byte)&mask ? '1' : '0'));
//         mask = mask >> 1;
//     }
//     printf("\n");
// }
// #pragma endregion assist

int archive_pack_file(huffman_table* table, char* src_file_name, char* dst_file_name) {
    uint8_t buffer = 0;              // to be written to file
    uint8_t remaining_bits = 0;      // the bits remaning in the last written buffer.
    int16_t bits = 0;                // bits in the buffer
    int16_t total_bits_written = 0;  // bits written in the path
    int16_t delta = 0;               // diff
    int64_t height = 0;
    file_info src_file;
    file_info dst_file;

    puts("~~~~~~~~~~Encode~~~~~~~~~~");

    src_file = open_file(src_file_name, O_RDONLY, MEM_DEFAULT);
    dst_file = open_file(dst_file_name, O_RDWR | O_TRUNC, MEM_DEFAULT);

    if (src_file.file_s.st_size == 0) {
        printf("[!] Empty file found!\n");
        return 0;
    }

    /*
        1)
            ex1: negativ delta
                *
                11110000 11111111 // path
                        *
                        11000000 // current buffer
            ex2: positiv delta
                               *
                -------- --110000
                          *
                         11000000

            calculate the diffrence between * and *
        2)
            if the delta was negativ(ex1),
                the entire buffer will be filled, and the res will aways be zero
            if the delta positiv (ex2)
                the buffer will filled with bits available
                delta describes the number of bits remaning, 8-delta describes the bit un use
    */
    for (uint64_t i = 0; i < src_file.file_s.st_size; i++) {
        if (table->length[(uint8_t)src_file.file[i]] == 0) {
            continue;
        }

        do {
            // calcualtes the diff between the head of the path, and last element of the buffer
            delta = (8 - bits) - (table->length[(uint8_t)src_file.file[i]] - total_bits_written);
            if (0 < delta) {  // postiv delta
                buffer = buffer | table->table[(uint8_t)src_file.file[i]] << (delta);
            } else if (0 >= delta) {  /// negativ delta
                buffer = buffer | table->table[(uint8_t)src_file.file[i]] >> (delta * -1);
            }

            if (delta < 0) {
                total_bits_written += (8 - bits);
                bits = 8;
            } else {
                total_bits_written = 0;
                bits = 8 - delta;
            }

            if (bits == 8 || i == src_file.file_s.st_size - 1) {
                if (i == src_file.file_s.st_size - 1) {
                    remaining_bits = bits;
                }
                height++;
                write(dst_file.file_d, &buffer, 1);
                bits = 0;
                buffer = 0;
            }
        } while (delta < 0);
    }

    // writes the remaning bits to the last pos of the file.
    write(dst_file.file_d, &remaining_bits, 1);

    if (height >= src_file.file_s.st_size) {
        // use uncompressed file
    }

    // printf("[!] Written bits in last index: %d\n", remaining_bits);
    printf("[!] Compressed [%ldb] -> [%ldb]\n", src_file.file_s.st_size, height);
    
    // cleanup
    close_file(src_file);
    close_file(dst_file);

    return 0;
}

int archive_unpack_file(uint32_t* frequency_table, char* src_file_name, char* dst_file_name) {
    int64_t height;
    uint8_t remaining_bits;
    uint8_t bound;
    huffman_node* trie;
    huffman_table* table;
    huffman_node* current;
    file_info src_file;
    file_info dst_file;

    src_file = open_file(src_file_name, S_IRUSR, MEM_DEFAULT);
    dst_file = open_file(dst_file_name, O_RDWR | O_TRUNC, MEM_DEFAULT);

    printf("file to decode[%s] of size %ld\n", src_file_name, src_file.file_s.st_size);

    // counts the number of unique of the origin file
    uint8_t queue_size = 0;
    for (uint16_t i = 0; i < 256; i++) {
        if (frequency_table[i] > 0) {
            queue_size++;
        }
    }

    // creates the queue with the counted size
    huffman_queue* queue = huffman_queue_create(queue_size + 1);
    for (uint32_t i = 0; i < 256; i++) {
        if (frequency_table[i] > 0) {
            huffman_node* leaf = calloc(1, sizeof(huffman_node));
            leaf->c = i;
            leaf->weight = frequency_table[i];
            leaf->type = LEAF;
            huffman_queue_add(queue, leaf);  // todo: add realloc
        }
    }

    huffman_queue_sort(queue);
    trie = huffman_trie_create(queue);
    table = huffman_table_create(trie);
    current = trie;

    height = src_file.file_s.st_size - 1;  // number of uncompressed characters
    remaining_bits = src_file.file[height];
    bound = 0;  // width bound

    for (int64_t y = 0; y < height; y++) {
        if (y == height - 1) {
            bound = 8 - remaining_bits;
        }
        for (int16_t i = 7; i >= bound; i--) {
            if (current->type == NODE) {
                if (src_file.file[y] & (1 << i)) {
                    current = (huffman_node*)current->right_trie;
                } else {
                    current = (huffman_node*)current->left_trie;
                }
            }

            if (current->type == LEAF) {
                // printf("%c", *((char*)(&current->c)));
                write(dst_file.file_d, (char*)(&current->c), 1);
                current = trie;
            }
        }
    }

    printf("[!] Compressed file size: %ld\n", height);

    // cleanup
    huffman_queue_destroy(queue);
    huffman_trie_destroy(trie);
    huffman_table_destroy(table);
    close_file(src_file);
    close_file(dst_file);

    return 0;
}
