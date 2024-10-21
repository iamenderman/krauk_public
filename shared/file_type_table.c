#include "file_type_table.h"

void append_table(uint32_t *dst, uint32_t *src);
void print_byte(uint8_t byte);
void leb128_encode(uint32_t num, uint8_t *buffer, uint32_t *size);
uint32_t leb128_decode(uint8_t *buffer, uint32_t *size);


/*
    Interface definition
*/
char *get_file_ending(char *file_name) {
    int file_length = strlen(file_name);
    for (int i = file_length - 1; i >= 0; i--) {
        if (file_name[i] == '/' || file_name[i] == '.') {
            return file_name + i;
        }
    }

    return file_name;
}

FIlE_TYPE_TABLE FIlE_TYPE_TABLE_new() {
    FIlE_TYPE_TABLE t;
    t.list_capacity = 10;
    t.list_count = 0;
    t.list = calloc(t.list_capacity, sizeof(FIlE_TYPE_FREQ_NODE));
    return t;
}

void FIlE_TYPE_TABLE_free(FIlE_TYPE_TABLE t) {
    for (size_t i = 0; i < t.list_count; i++) {
        free(t.list[i].file_end);
        free(t.list[i].freq_table);
        huffman_table_destroy(t.list[i].huff_table);
    }
    free(t.list);
}

FIlE_TYPE_TABLE FIlE_TYPE_TABLE_update(FIlE_TYPE_TABLE t, char *file_end, uint32_t *frequency_table) {
    for (size_t i = 0; i < t.list_count; i++) {
        // if there exists an entery for the given file_end
        if (strcmp(file_end, t.list[i].file_end) == 0) {
            append_table(t.list[i].freq_table, frequency_table);
            return t;
        }
    }

    if (t.list_count == t.list_capacity) {
        t.list_capacity *= 3;
        t.list = realloc(t.list, t.list_capacity * sizeof(FIlE_TYPE_FREQ_NODE));
    }

    // creates the table
    t.list[t.list_count].freq_table = calloc(256, sizeof(uint32_t));
    append_table(t.list[t.list_count].freq_table, frequency_table);

    // creates the file ending
    t.list[t.list_count].file_end = calloc(strlen(file_end) + 1, sizeof(char));
    strcpy(t.list[t.list_count].file_end, file_end);

    // updates the size
    t.list_count += 1;

    return t;
}

uint32_t *FIlE_TYPE_TABLE_get_table(FIlE_TYPE_TABLE t, char *file_end) {
    for (size_t i = 0; i < t.list_count; i++) {
        // if there exists an entery for the given file_end
        if (strcmp(file_end, t.list[i].file_end) == 0) {
            return t.list[i].freq_table;
        }
    }

    // if no entry was found
    return NULL;
}

huffman_table *FIlE_TYPE_TABLE_get_huff(FIlE_TYPE_TABLE t, char *file_end) {
    for (size_t i = 0; i < t.list_count; i++) {
        // if there exists an entery for the given file_end
        if (strcmp(file_end, t.list[i].file_end) == 0) {
            return t.list[i].huff_table;
        }
    }

    // if no entry was found
    return NULL;
}

FIlE_TYPE_TABLE FIlE_TYPE_TABLE_build_huffman(FIlE_TYPE_TABLE f) {
    for (size_t i = 0; i < f.list_count; i++) {
        // creating and filling huffman queue
        huffman_queue *queue = huffman_queue_create(257);
        for (uint32_t c = 0; c < 256; c++) {
            if (f.list[i].freq_table[c] > 0) {
                huffman_node *leaf = calloc(1, sizeof(huffman_node));
                leaf->c = c;
                leaf->weight = f.list[i].freq_table[c];
                leaf->type = LEAF;
                huffman_queue_add(queue, leaf);
            }
        }

        // sorting the queue
        huffman_queue_sort(queue);
        // creating trie from the sorted queue
        huffman_node *trie = huffman_trie_create(queue);
        // creating table from trie
        f.list[i].huff_table = huffman_table_create(trie);

        huffman_queue_destroy(queue);
        huffman_trie_destroy(trie);
    }

    return f;
}

uint8_t *FIlE_TYPE_TABLE_seralize_table(FIlE_TYPE_TABLE t, uint32_t *length) {
    // calcs the maximum size for the seralize_table
    uint32_t capacity = 256 * (sizeof(uint32_t) + 2) * t.list_capacity + sizeof(uint32_t);
    for (size_t i = 0; i < t.list_count; i++) {
        capacity += strlen(t.list[i].file_end) + 1;
    }

    uint8_t *buffer = calloc(capacity, sizeof(uint8_t));
    uint32_t size = sizeof(uint32_t);  // starting index after the table size;

    // shifts table size into the buffer
    encode_uint32_t(buffer, t.list_count);

    for (size_t i = 0; i < t.list_count; i++) {
        // temp_node for cleaner code
        FIlE_TYPE_FREQ_NODE *fn = t.list + i;

        // copies the file_ending to the buffer
        uint32_t len = strlen(fn->file_end) + 1;  // account for \0...
        memcpy(buffer + size, fn->file_end, len);
        size += len;

        // makes space for freq-table elem count
        uint32_t freq_count = size++;
        buffer[freq_count] = 0;

        // writes the 'sparse' table
        for (size_t j = 0; j < 256; j++) {
            if (fn->freq_table[j] == 0) {
                continue;
            }

            // increases the elem-count
            buffer[freq_count]++;

            // index - for sparse representation
            buffer[size++] = j;
            leb128_encode(fn->freq_table[j], buffer, &size);
        }
    }

    *length = size;

    return buffer;
}

FIlE_TYPE_TABLE FIlE_TYPE_TABLE_deseralize_table(uint8_t *buffer) {
    FIlE_TYPE_TABLE fl = FIlE_TYPE_TABLE_new();
    uint32_t temp_table[256] = {0};
    uint32_t offset = sizeof(uint32_t);

    // and decodes the size....
    uint32_t size = decode_uint32_t(buffer);

    for (size_t i = 0; i < size; i++) {
        // gets the file_ending
        char *temp_file_end = buffer + offset;
        uint32_t str_len = strlen(temp_file_end);

        // \0 skip
        offset += str_len + 1;

        // number of occupied elemets in the freq_tablec
        uint8_t freq_table_size = buffer[offset++];
        for (size_t j = 0; j < freq_table_size; j++) {
            uint32_t ascii = buffer[offset++];
            temp_table[ascii] = leb128_decode(buffer + offset, &offset);
        }

        // updates the table with newly parsed values.
        fl = FIlE_TYPE_TABLE_update(fl, temp_file_end, temp_table);

        // clears temp table
        BZERO(temp_table, 256 * sizeof(uint32_t));
    }

    return fl;
}

void FIlE_TYPE_TABLE_log(FIlE_TYPE_TABLE t) {
    for (uint32_t i = 0; i < t.list_count; i++) {
        printf("~~~~~~~~%s~~~~~~~~\n", t.list[i].file_end);
        for (size_t j = 0; j < 256; j++) {
            if (t.list[i].freq_table[j] > 0) {
                printf("\t[%ld]: %d\n", j, t.list[i].freq_table[j]);
            }
        }
        puts("");
    }
}

/*
    Assist functions
*/
void append_table(uint32_t *dst, uint32_t *src) {
    for (size_t i = 0; i < 256; i++) {
        if (dst[i] + src[i] >= dst[i]) {  // overflow protecion 
            dst[i] += src[i];
        }
    }
}

void print_byte(uint8_t byte) {
    uint8_t mask = 0b10000000;
    for (size_t i = 0; i < 8; i++) {
        printf("%c", byte & mask ? '1' : '0');
        mask = mask >> 1;
    }
}

void leb128_encode(uint32_t num, uint8_t *buffer, uint32_t *size) {
    uint8_t byte = 0;

    do {
        // gets the first seven bits
        byte = num & 0b01111111;
        // adds 7 bits into the first 8 bits.
        num = num >> 7;
        // if we have bits left to parse
        if (num > 0) {
            // highets bit to one
            byte |= 0b10000000;
        }
        buffer[(*size)++] = byte;
    } while (num > 0);
}

uint32_t leb128_decode(uint8_t *buffer, uint32_t *size) {
    uint32_t i = 0;
    uint32_t res = 0;
    do {
        res |= ((buffer[i] & 0b01111111) << 7 * i);
    } while (buffer[i++] & 0b10000000);

    *size += i;
    return res;
}
