#ifndef DD2_ASSETS_BYTES_H
#define DD2_ASSETS_BYTES_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const uint8_t *data;
    size_t size;
} dd2_byte_view;

typedef struct {
    uint8_t *data;
    size_t size;
} dd2_byte_buffer;

enum { DD2_BYTE_BITS = 8 };

/* Internal decoding primitives: the caller must first validate that all bytes
 * of the word lie in its view. These never cast an unaligned on-disk pointer. */
static inline uint16_t dd2_read_le16(const uint8_t *bytes) {
    return (uint16_t)((unsigned)bytes[0] | ((unsigned)bytes[1] << DD2_BYTE_BITS));
}

static inline uint32_t dd2_read_le32(const uint8_t *bytes) {
    uint32_t value = 0;
    for (unsigned index = 0; index < sizeof(value); ++index) {
        value |= (uint32_t)bytes[index] << (index * DD2_BYTE_BITS);
    }
    return value;
}

static inline int32_t dd2_read_le_i32(const uint8_t *bytes) {
    const uint32_t value = dd2_read_le32(bytes);
    return value <= INT32_MAX ? (int32_t)value : -1 - (int32_t)(UINT32_MAX - value);
}

#endif
