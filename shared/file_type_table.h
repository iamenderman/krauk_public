#pragma once

/*
    utan file_ending ?
        "namn"
    Makefile | makefile => makefile
    update_freq(char *file ending, )
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "huffman.h"
#include "ser.h"

#include "../shared/krauk_connection.h"

typedef struct {
    char *file_end;
    uint32_t *freq_table;
    huffman_table *huff_table;
} FIlE_TYPE_FREQ_NODE;

typedef struct {
    FIlE_TYPE_FREQ_NODE *list;
    uint32_t list_count;
    uint32_t list_capacity;
} FIlE_TYPE_TABLE;

char *get_file_ending(char *file_name);

void FIlE_TYPE_TABLE_free(FIlE_TYPE_TABLE t);

FIlE_TYPE_TABLE FIlE_TYPE_TABLE_new();

FIlE_TYPE_TABLE FIlE_TYPE_TABLE_update(FIlE_TYPE_TABLE t, char *file_end, uint32_t *frequency_table);

uint32_t *FIlE_TYPE_TABLE_get_table(FIlE_TYPE_TABLE t, char *file_end);

huffman_table *FIlE_TYPE_TABLE_get_huff(FIlE_TYPE_TABLE t, char *file_end);

FIlE_TYPE_TABLE FIlE_TYPE_TABLE_build_huffman(FIlE_TYPE_TABLE f);

uint8_t *FIlE_TYPE_TABLE_serialize_table(FIlE_TYPE_TABLE t, uint32_t *length);

FIlE_TYPE_TABLE FIlE_TYPE_TABLE_deserialize_table(uint8_t *buffer);

void FIlE_TYPE_TABLE_log(FIlE_TYPE_TABLE t);