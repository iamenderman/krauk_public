#include "huffman.h"

#include <assert.h>
void print_binary(uint128_t byte, uint16_t len);
void create_table(huffman_table* table, huffman_node* trie, uint128_t path, buffer len);
int comp(const void* a, const void* b);

/*
    Trie
*/
huffman_node* huffman_trie_create(huffman_queue* queue) {
    if (queue->pos == queue->size - 1) {
        return queue->nodes[queue->pos];
    }

    huffman_node* left = queue->nodes[queue->pos];
    huffman_node* right = queue->nodes[queue->pos + 1];

    if (left == NULL || right == NULL) {
        printf("length of queue: %u\n", queue->size);
        printf("left read: : %u\n", queue->pos);
        printf("right read: : %u\n", queue->pos + 1);
    }

    huffman_node* parent = huffman_trie_create_parent(left, right);

    queue->nodes[queue->pos] = NULL;
    queue->nodes[queue->pos++] = parent;
    huffman_queue_sort(queue);

    return huffman_trie_create(queue);
}

huffman_node* huffman_trie_create_parent(huffman_node* left, huffman_node* right) {
    huffman_node* parent = calloc(1, sizeof(huffman_node));

    assert(left != NULL);
    assert(right != NULL);
    parent->weight = left->weight + right->weight;
    parent->left_trie = (struct huffman_node*)left;
    parent->right_trie = (struct huffman_node*)right;
    parent->type = NODE;

    return parent;
}

void huffman_trie_destroy(huffman_node* trie) {
    if (trie->type == LEAF) {
        free(trie);
        return;
    }

    huffman_trie_destroy((huffman_node*)trie->left_trie);
    huffman_trie_destroy((huffman_node*)trie->right_trie);
    free(trie);
}

void huffman_trie_log(huffman_node* trie) {
    printf("c: %c, weight: %d\n", trie->c, trie->weight);
}

/*
    Queue
*/
huffman_queue* huffman_queue_create() {
    huffman_queue* queue = calloc(1, sizeof(huffman_queue));
    queue->nodes = calloc(257, sizeof(huffman_node*));
    queue->size = 0;
    queue->pos = 0;

    return queue;
}

void huffman_queue_destroy(huffman_queue* queue) {
    free(queue->nodes);
    free(queue);
}

void huffman_queue_add(huffman_queue* queue, huffman_node* node) {
    queue->nodes[queue->size++] = node;
}

void huffman_queue_sort(huffman_queue* queue) {
    qsort(queue->nodes, queue->size, sizeof(huffman_node*), comp);
}

void huffman_queue_log(huffman_queue* queue) {
    for (unsigned int i = queue->pos; i < queue->size; i++) {
        if (queue->nodes[i] == NULL) {
            printf("NULL\n");
        } else {
            if (i == queue->pos) {
                printf("*");
            } else {
                printf(" ");
            }
            printf("c: %c, weight: %d\n", (char)(queue->nodes[i]->c), queue->nodes[i]->weight);
        }
    }
}

/*
    Table
*/
huffman_table* huffman_table_create(huffman_node* trie) {
    huffman_table* table = calloc(1, sizeof(huffman_table));

    // hot fix
    if (trie->type == LEAF) {
        table->table[trie->c] = 0;
        table->length[trie->c] = 1;
    } else {
        create_table(table, trie, 0, 0);
    }

    return table;
}

void huffman_table_destroy(huffman_table* table) {
    free(table);
}

void huffman_table_log(huffman_table* table) {
    for (int i = 0; i < 256; i++) {
        if (table->length[i] > 0) {
            printf("[%c][%d]: \n", i, table->length[i]);
            print_binary(table->table[i], table->length[i]);
        }
    }
}

/*
    Assist functions
*/
void print_binary(uint128_t byte, uint16_t len) {
    uint128_t mask = ((uint128_t)1) << 127;
    for (uint128_t i = 0; i < 128; i++) {
        if (i > 127 - len) {
            printf("%c", byte & mask ? '1' : '0');
        }

        mask = mask >> 1;
    }
    printf("\n");
}

void create_table(huffman_table* table, huffman_node* trie, uint128_t path, buffer len) {
    if (trie->left_trie != NULL) {
        create_table(table, (huffman_node*)trie->left_trie, path << 1, len + 1);
    }
    if (trie->right_trie != NULL) {
        create_table(table, (huffman_node*)trie->right_trie, path << 1 | 1, len + 1);
    }

    if (trie->type == LEAF) {
        table->table[trie->c] = path;
        table->length[trie->c] = len;
        return;
    }
}

int comp(const void* a, const void* b) {
    huffman_node* left = (*(huffman_node**)a);
    huffman_node* right = (*(huffman_node**)b);

    if (right == NULL || left == NULL) {
        return 0;
    }

    return left->weight - right->weight;
}
