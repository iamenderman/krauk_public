#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void encode_uint64_t(uint8_t *buffer, uint64_t i);
void encode_uint32_t(uint8_t *buffer, uint32_t i);
void encode_uint16_t(uint8_t *buffer, uint16_t i);
void encode_str(uint8_t *buffer, char *str);
void encode_str_slice(uint8_t *buffer, char *str, uint32_t len);
void encode_sequence(uint8_t *buffer, int arg_count, ...);
uint64_t decode_uint64_t(uint8_t *buffer);
uint32_t decode_uint32_t(uint8_t *buffer);
uint16_t decode_uint16_t(uint8_t *buffer);