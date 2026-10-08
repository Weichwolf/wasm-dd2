#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/font.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_FONT_TEST_HEADER = 16,
    DD2_FONT_TEST_RECORD = 416,
    DD2_FONT_TEST_RECORD_HEADER = 16,
    DD2_FONT_TEST_COUNT = 2,
    DD2_FONT_TEST_BYTES = DD2_FONT_TEST_HEADER + (DD2_FONT_TEST_COUNT * DD2_FONT_TEST_RECORD),
    DD2_FONT_TEST_GLYPH_BYTES = 4,
    DD2_FONT_TEST_FIRST = 'A',
    DD2_FONT_TEST_LAST = 127,
    DD2_FONT_TEST_U = 240,
    DD2_FONT_TEST_V = 230,
    DD2_FONT_TEST_WIDTH = 16,
    DD2_FONT_TEST_HEIGHT = 26,
    DD2_FONT_TEST_SHORT_STRIDE = 399,
    DD2_FONT_TEST_MAX_BYTES = 1024 * 1024
};

static void dd2_font_test_fixture(uint8_t bytes[DD2_FONT_TEST_BYTES]) {
    for (size_t index = 0; index < DD2_FONT_TEST_BYTES; ++index) {
        bytes[index] = 0;
    }
    dd2_test_write_le32(bytes + DD2_TEST_WORD_BYTES, UINT32_C(0x464f4e54));
    dd2_test_write_le32(bytes + ((size_t)2 * DD2_TEST_WORD_BYTES), DD2_FONT_TEST_BYTES);
    dd2_test_write_le32(bytes + ((size_t)3 * DD2_TEST_WORD_BYTES), DD2_FONT_TEST_COUNT);
    for (unsigned font = 0; font < DD2_FONT_TEST_COUNT; ++font) {
        uint8_t *record = bytes + DD2_FONT_TEST_HEADER + ((size_t)font * DD2_FONT_TEST_RECORD);
        dd2_test_write_le32(record, DD2_FONT_TEST_RECORD);
        uint8_t *glyph =
            record + DD2_FONT_TEST_RECORD_HEADER +
            (((size_t)DD2_FONT_TEST_FIRST - DD2_FONT_FIRST_CHARACTER) * DD2_FONT_TEST_GLYPH_BYTES);
        glyph[0] = DD2_FONT_TEST_U;
        glyph[1] = DD2_FONT_TEST_V;
        glyph[2] = DD2_FONT_TEST_WIDTH;
        glyph[3] = DD2_FONT_TEST_HEIGHT - (uint8_t)font;
        glyph =
            record + DD2_FONT_TEST_RECORD_HEADER +
            (((size_t)DD2_FONT_TEST_LAST - DD2_FONT_FIRST_CHARACTER) * DD2_FONT_TEST_GLYPH_BYTES);
        glyph[2] = 1;
        glyph[3] = 1;
    }
}

static bool dd2_font_test_owned(uint8_t bytes[DD2_FONT_TEST_BYTES], size_t size) {
    dd2_test_write_le32(bytes + ((size_t)2 * DD2_TEST_WORD_BYTES), (uint32_t)size);
    dd2_font_bank *bank = dd2_font_bank_create((dd2_byte_view){.data = bytes, .size = size});
    if (bank == NULL) {
        return false;
    }
    /* Releasing or overwriting the source must not invalidate decoded glyphs. */
    for (size_t index = 0; index < size; ++index) {
        bytes[index] = UINT8_MAX;
    }
    dd2_font_glyph glyph = {0};
    bool valid = dd2_font_bank_count(bank) == DD2_FONT_TEST_COUNT;
    for (unsigned font = 0; font < DD2_FONT_TEST_COUNT; ++font) {
        valid = valid && dd2_font_bank_glyph(bank, font, DD2_FONT_TEST_FIRST, &glyph) &&
                glyph.u == DD2_FONT_TEST_U && glyph.v == DD2_FONT_TEST_V &&
                glyph.width == DD2_FONT_TEST_WIDTH && glyph.height == DD2_FONT_TEST_HEIGHT - font &&
                dd2_font_bank_glyph(bank, font, DD2_FONT_FIRST_CHARACTER, &glyph) &&
                glyph.width == 0 && glyph.height == 0 &&
                dd2_font_bank_glyph(bank, font, DD2_FONT_TEST_LAST, &glyph) && glyph.width == 1;
    }
    valid = valid && !dd2_font_bank_glyph(bank, 0, DD2_FONT_FIRST_CHARACTER - 1, &glyph) &&
            glyph.width == 0 && !dd2_font_bank_glyph(bank, 0, DD2_FONT_TEST_LAST + 1, &glyph) &&
            !dd2_font_bank_glyph(bank, SIZE_MAX, DD2_FONT_TEST_FIRST, &glyph) &&
            !dd2_font_bank_glyph(NULL, 0, DD2_FONT_TEST_FIRST, &glyph) &&
            !dd2_font_bank_glyph(bank, 0, DD2_FONT_TEST_FIRST, NULL);
    dd2_font_bank_destroy(bank);
    dd2_font_bank_destroy(NULL);
    return valid && dd2_font_bank_count(NULL) == 0;
}

static bool dd2_font_test_invalid(const uint8_t bytes[DD2_FONT_TEST_BYTES], size_t size) {
    dd2_font_bank *bank = dd2_font_bank_create((dd2_byte_view){.data = bytes, .size = size});
    const bool valid = bank == NULL;
    dd2_font_bank_destroy(bank);
    return valid;
}

static bool dd2_font_test_bounds(void) {
    uint8_t bytes[DD2_FONT_TEST_BYTES];
    dd2_font_test_fixture(bytes);
    if (!dd2_font_test_owned(bytes, sizeof(bytes))) {
        return false;
    }
    dd2_font_test_fixture(bytes);
    if (!dd2_font_test_owned(bytes, sizeof(bytes) - DD2_TEST_WORD_BYTES) ||
        dd2_font_bank_create((dd2_byte_view){0}) != NULL) {
        return false;
    }
    dd2_font_test_fixture(bytes);
    for (size_t size = 0; size < sizeof(bytes); ++size) {
        if (!dd2_font_test_invalid(bytes, size)) {
            return false;
        }
    }
    bytes[DD2_TEST_WORD_BYTES] = 0;
    if (!dd2_font_test_invalid(bytes, sizeof(bytes))) {
        return false;
    }
    dd2_font_test_fixture(bytes);
    dd2_test_write_le32(bytes + ((size_t)3 * DD2_TEST_WORD_BYTES), UINT32_MAX);
    if (!dd2_font_test_invalid(bytes, sizeof(bytes))) {
        return false;
    }
    const uint32_t bad_strides[] = {0, DD2_FONT_TEST_SHORT_STRIDE, UINT32_MAX};
    for (unsigned font = 0; font < DD2_FONT_TEST_COUNT; ++font) {
        for (size_t trial = 0; trial < sizeof(bad_strides) / sizeof(bad_strides[0]); ++trial) {
            dd2_font_test_fixture(bytes);
            dd2_test_write_le32(bytes + DD2_FONT_TEST_HEADER +
                                    ((size_t)font * DD2_FONT_TEST_RECORD),
                                bad_strides[trial]);
            if (!dd2_font_test_invalid(bytes, sizeof(bytes))) {
                return false;
            }
        }
    }
    dd2_font_test_fixture(bytes);
    uint8_t *glyph =
        bytes + DD2_FONT_TEST_HEADER + DD2_FONT_TEST_RECORD_HEADER +
        (((size_t)DD2_FONT_TEST_FIRST - DD2_FONT_FIRST_CHARACTER) * DD2_FONT_TEST_GLYPH_BYTES);
    ++glyph[2];
    if (!dd2_font_test_invalid(bytes, sizeof(bytes))) {
        return false;
    }
    --glyph[2];
    ++glyph[3];
    if (!dd2_font_test_invalid(bytes, sizeof(bytes))) {
        return false;
    }
    dd2_font_test_fixture(bytes);
    dd2_test_write_le32(bytes + ((size_t)2 * DD2_TEST_WORD_BYTES), DD2_FONT_TEST_HEADER);
    dd2_test_write_le32(bytes + ((size_t)3 * DD2_TEST_WORD_BYTES), 0);
    dd2_font_bank *empty =
        dd2_font_bank_create((dd2_byte_view){.data = bytes, .size = DD2_FONT_TEST_HEADER});
    const bool valid = empty != NULL && dd2_font_bank_count(empty) == 0;
    dd2_font_bank_destroy(empty);
    return valid;
}

static bool dd2_font_test_export(dd2_byte_view bytes) {
    dd2_font_bank *bank = dd2_font_bank_create(bytes);
    if (bank == NULL) {
        return false;
    }
    printf("{\"fonts\":[");
    bool valid = true;
    for (size_t font = 0; font < dd2_font_bank_count(bank) && valid; ++font) {
        printf("%s[", font == 0 ? "" : ",");
        for (unsigned index = 0; index < DD2_FONT_GLYPHS && valid; ++index) {
            dd2_font_glyph glyph = {0};
            valid = dd2_font_bank_glyph(bank, font, (uint8_t)(index + DD2_FONT_FIRST_CHARACTER),
                                        &glyph);
            printf("%s[%u,%u,%u,%u]", index == 0 ? "" : ",", (unsigned)glyph.u, (unsigned)glyph.v,
                   (unsigned)glyph.width, (unsigned)glyph.height);
        }
        printf("]");
    }
    puts("]}");
    dd2_font_bank_destroy(bank);
    return valid;
}

static bool dd2_font_test_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    bool valid = false;
    if (fseek(file, 0, SEEK_END) == 0) {
        const long size = ftell(file);
        if (size >= 0 && size <= DD2_FONT_TEST_MAX_BYTES && fseek(file, 0, SEEK_SET) == 0) {
            uint8_t *bytes = malloc(size == 0 ? 1 : (size_t)size);
            if (bytes != NULL) {
                if (fread(bytes, 1, (size_t)size, file) == (size_t)size) {
                    valid =
                        dd2_font_test_export((dd2_byte_view){.data = bytes, .size = (size_t)size});
                }
                free(bytes);
            }
        }
    }
    return fclose(file) == 0 && valid;
}

int main(int argc, char **argv) {
    if (argc == 2) {
        return dd2_font_test_file(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if (argc != 1 || !dd2_font_test_bounds()) {
        return EXIT_FAILURE;
    }
    puts("{\"scope\":\"Owned font glyphs, source lifetime, character and record "
         "bounds\",\"pass\":true}");
    return EXIT_SUCCESS;
}
