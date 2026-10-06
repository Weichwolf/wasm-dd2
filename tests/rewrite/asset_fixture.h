#ifndef DD2_TEST_ASSET_FIXTURE_H
#define DD2_TEST_ASSET_FIXTURE_H

#include "assets/bytes.h"
#include "assets/textures.h"

#include <stddef.h>
#include <stdint.h>

enum {
    DD2_TEST_WORD_BYTES = 4,
    DD2_TEST_PALETTE_BYTES = 1024,
    DD2_TEST_CLUT_BYTES = 4096,
    DD2_TEST_TX0_BYTES = 44,
    DD2_TEST_TX1_BYTES = 6,
    DD2_TEST_TX_DESCRIPTOR_BYTES = 16,
    DD2_TEST_TX_X_OFFSET = 6,
    DD2_TEST_TX_Y_OFFSET = 8,
    DD2_TEST_TX_FORMAT_8 = 8,
    DD2_TEST_TX_DATA_OFFSET = 36,
    DD2_TEST_TEXEL_A = 0x11,
    DD2_TEST_TEXEL_B = 0x20,
    DD2_TEST_TEXEL_C = 0x33,
    DD2_TEST_TEXEL_D = 0x44,
    DD2_TEST_TEXEL_E = 0x55,
    DD2_TEST_TEXEL_F = 0x66,
    DD2_TEST_NEUTRAL_SHADE = 8,
    DD2_TEST_PALETTE_BLUE = 127,
    DD2_TEST_TEXTURE_MAGIC = 0x54584554,
    DD2_TEST_COLOR_CHANNELS = 4,
    DD2_TEST_UNUSED_BYTE = 0xcd
};

typedef struct {
    uint8_t first[DD2_TEST_TX0_BYTES];
    uint8_t second[DD2_TEST_TX1_BYTES];
    uint8_t palette[DD2_TEST_PALETTE_BYTES];
    uint8_t cluts[DD2_TEST_CLUT_BYTES];
    uint8_t extra_cluts[DD2_TEST_CLUT_BYTES];
} dd2_test_texture_fixture;

static inline void dd2_test_write_le32(uint8_t *bytes, uint32_t value) {
    for (unsigned index = 0; index < sizeof(value); ++index) {
        bytes[index] = (uint8_t)(value >> (index * DD2_BYTE_BITS));
    }
}

static inline void dd2_test_write_le16(uint8_t *bytes, uint16_t value) {
    for (unsigned index = 0; index < sizeof(value); ++index) {
        bytes[index] = (uint8_t)(value >> (index * DD2_BYTE_BITS));
    }
}

static inline dd2_texture_sources dd2_test_texture_sources(dd2_test_texture_fixture *fixture) {
    *fixture = (dd2_test_texture_fixture){0};
    dd2_test_write_le32(fixture->first, 2);
    uint8_t *descriptor = fixture->first + DD2_TEST_WORD_BYTES;
    dd2_test_write_le16(descriptor, 4);
    dd2_test_write_le16(descriptor + 2, 2);
    dd2_test_write_le16(descriptor + 4, 2);
    dd2_test_write_le16(descriptor + DD2_TEST_TX_X_OFFSET, DD2_TEXTURE_PAGE_SIDE - 2);
    dd2_test_write_le16(descriptor + DD2_TEST_TX_Y_OFFSET, DD2_TEXTURE_PAGE_SIDE - 1);
    descriptor += DD2_TEST_TX_DESCRIPTOR_BYTES;
    dd2_test_write_le16(descriptor, DD2_TEST_TX_FORMAT_8);
    dd2_test_write_le16(descriptor + 2, 2);
    dd2_test_write_le16(descriptor + 4, 1);
    dd2_test_write_le16(descriptor + DD2_TEST_TX_Y_OFFSET, DD2_TEXTURE_PAGE_SIDE);
    dd2_test_write_le32(fixture->first + DD2_TEST_TX_DATA_OFFSET, DD2_TEST_TEXTURE_MAGIC);
    fixture->first[DD2_TEST_TX_DATA_OFFSET + DD2_TEST_WORD_BYTES] = DD2_TEST_TEXEL_A;
    fixture->first[DD2_TEST_TX_DATA_OFFSET + DD2_TEST_WORD_BYTES + 1] = DD2_TEST_TEXEL_B;
    fixture->first[DD2_TEST_TX_DATA_OFFSET + DD2_TEST_WORD_BYTES + 2] = DD2_TEST_TEXEL_C;
    fixture->first[DD2_TEST_TX_DATA_OFFSET + DD2_TEST_WORD_BYTES + 3] = DD2_TEST_TEXEL_D;
    dd2_test_write_le32(fixture->second, DD2_TEST_TEXTURE_MAGIC);
    fixture->second[DD2_TEST_WORD_BYTES] = DD2_TEST_TEXEL_E;
    fixture->second[DD2_TEST_WORD_BYTES + 1] = DD2_TEST_TEXEL_F;
    for (size_t color = 0; color < DD2_TEXTURE_PAGE_SIDE; ++color) {
        /* PAL is BGR plus an unused byte. Distinct R/B values expose swaps in
         * both the decoder and the real SoftGL texture/mesh pixel probes. */
        fixture->palette[color * DD2_TEST_COLOR_CHANNELS] = DD2_TEST_PALETTE_BLUE;
        fixture->palette[(color * DD2_TEST_COLOR_CHANNELS) + 1] = (uint8_t)(UINT8_MAX - color);
        fixture->palette[(color * DD2_TEST_COLOR_CHANNELS) + 2] = (uint8_t)color;
        fixture->palette[(color * DD2_TEST_COLOR_CHANNELS) + 3] = DD2_TEST_UNUSED_BYTE;
        for (size_t shade = 0; shade < DD2_TEXTURE_SHADES; ++shade) {
            fixture->cluts[(shade * DD2_TEXTURE_PAGE_SIDE) + color] =
                (uint8_t)(shade == DD2_TEST_NEUTRAL_SHADE ? color : UINT8_MAX - color);
            fixture->extra_cluts[(shade * DD2_TEXTURE_PAGE_SIDE) + color] =
                (uint8_t)((color + 1) & UINT8_MAX);
        }
    }
    return (dd2_texture_sources){
        .parts = {{.data = fixture->first, .size = sizeof(fixture->first)},
                  {.data = fixture->second, .size = sizeof(fixture->second)}},
        .part_count = 2,
        .palette = {.data = fixture->palette, .size = sizeof(fixture->palette)},
        .cluts = {.data = fixture->cluts, .size = sizeof(fixture->cluts)},
        .extra_cluts = {.data = fixture->extra_cluts, .size = sizeof(fixture->extra_cluts)}};
}

#endif
