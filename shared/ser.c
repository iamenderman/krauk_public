#include "ser.h"

void encode_uint64_t(uint8_t *buffer, uint64_t i) {
    for (int j = 0; j < sizeof(uint64_t); j++) {
        buffer[j] = (i >> 8 * j);
    }
}

void encode_uint32_t(uint8_t *buffer, uint32_t i) {
    for (int j = 0; j < sizeof(uint32_t); j++) {
        buffer[j] = (i >> 8 * j);
    }
}

void encode_uint16_t(uint8_t *buffer, uint16_t i) {
    for (int j = 0; j < sizeof(uint16_t); j++) {
        buffer[j] = (i >> 8 * j);
    }
}
    
void encode_str(uint8_t *buffer, char *str) {
    strcpy(buffer, str);
}

void encode_str_slice(uint8_t *buffer, char *str, uint32_t len) {
    memcpy(buffer, str, len);
}

void encode_sequence(uint8_t *buffer, int arg_count, ...) {
    va_list args;
    va_start(args, arg_count);

    for (int i = 0; i < arg_count; i++) {
        buffer[i] = va_arg(args, int);
    }

    va_end(args);
}

uint64_t decode_uint64_t(uint8_t *buffer) {
    uint64_t i = 0;
    for (int j = sizeof(uint64_t) - 1; j >= 0; j--) {
        i = i << 8;
        i += buffer[j];
    }
    return i;
}

uint32_t decode_uint32_t(uint8_t *buffer) {
    uint32_t i = 0;
    for (int j = sizeof(uint32_t) - 1; j >= 0; j--) {
        i = i << 8;
        i += buffer[j];
    }
    return i;
}

uint16_t decode_uint16_t(uint8_t *buffer) {
    uint16_t i = 0;
    for (int j = sizeof(uint16_t) - 1; j >= 0; j--) {
        i = i << 8;
        i += buffer[j];
    }
    return i;
}