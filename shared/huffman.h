#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned __int128 uint128_t;
typedef __uint8_t buffer;

typedef enum {
    LEAF,
    NODE
} huffman_node_type;

typedef struct {
    struct huffman_node* left_trie;  
    struct huffman_node* right_trie; 
    uint32_t weight;                 
    uint8_t c;                 
    huffman_node_type type;
} huffman_node;

typedef struct {
    huffman_node** nodes; 
    uint32_t size;
    uint32_t pos;
} huffman_queue;

typedef struct {
    buffer length[256];
    uint128_t table[256];
} huffman_table;

// creates a huffman priority queue
huffman_queue* huffman_queue_create();
// destroyes huffman queue and all of its contents
void huffman_queue_destroy(huffman_queue* queue);
// adds a 'node' to the queue
void huffman_queue_add(huffman_queue* queue, huffman_node* node);
// sort the huffman queue - must be called before trie creation
void huffman_queue_sort(huffman_queue* queue);

// creates a huffman trie;
huffman_node* huffman_trie_create(huffman_queue* queue);
// creates an huffman trie parent from children
huffman_node* huffman_trie_create_parent(huffman_node* left, huffman_node* right);
// frees the trie and all of its content
void huffman_trie_destroy(huffman_node* trie);

// creates huffman table
huffman_table* huffman_table_create(huffman_node* trie);
// yeets table
void huffman_table_destroy(huffman_table* table);

// add debug flag
void huffman_queue_log(huffman_queue* queue);

void huffman_trie_log(huffman_node* trie);

void huffman_table_log(huffman_table* table);