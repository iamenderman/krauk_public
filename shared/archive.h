#pragma once

#include <stdint.h>

#include "file_info.h"
#include "huffman.h"

/*
    packs the given file to the dst
    user responsable for freeing the returend freq_table
*/
extern int archive_pack_file(huffman_table *table, char *src_file_name, char *dst_file_name);
extern int archive_unpack_file(uint32_t *frequency_table, char *src_file_name, char *dst_file_name);