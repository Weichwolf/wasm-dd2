#include "assets/font.h"

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    DD2_FONT_FILE_HEADER = 16,
    DD2_FONT_RECORD_HEADER = 16,
    DD2_FONT_GLYPH_BYTES = 4,
    DD2_FONT_WORD_BYTES = 4,
    DD2_FONT_MAGIC_OFFSET = 4,
    DD2_FONT_SIZE_OFFSET = 8,
    DD2_FONT_COUNT_OFFSET = 12,
    DD2_FONT_TEXTURE_SIDE = 256
};
static const uint32_t dd2_font_magic = UINT32_C(0x464f4e54);

typedef struct {
    dd2_font_glyph glyphs[DD2_FONT_GLYPHS];
} dd2_font;

struct dd2_font_bank {
    size_t count;
    dd2_font fonts[];
};

static bool dd2_font_record(dd2_byte_view bytes, dd2_font *font) {
    for (size_t index = 0; index < DD2_FONT_GLYPHS; ++index) {
        const uint8_t *glyph = bytes.data + DD2_FONT_RECORD_HEADER + (index * DD2_FONT_GLYPH_BYTES);
        const dd2_font_glyph decoded = {
            .u = glyph[0], .v = glyph[1], .width = glyph[2], .height = glyph[3]};
        if ((unsigned)decoded.u + decoded.width > DD2_FONT_TEXTURE_SIDE ||
            (unsigned)decoded.v + decoded.height > DD2_FONT_TEXTURE_SIDE) {
            return false;
        }
        font->glyphs[index] = decoded;
    }
    return true;
}

static bool dd2_font_decode(dd2_byte_view bytes, dd2_font_bank *bank) {
    const size_t minimum =
        DD2_FONT_RECORD_HEADER + ((size_t)DD2_FONT_GLYPHS * DD2_FONT_GLYPH_BYTES);
    size_t offset = DD2_FONT_FILE_HEADER;
    for (size_t index = 0; index < bank->count; ++index) {
        if (offset > bytes.size || bytes.size - offset < minimum) {
            return false;
        }
        const size_t available = bytes.size - offset;
        const size_t stride = dd2_read_le32(bytes.data + offset);
        if (stride < minimum) {
            return false;
        }
        if (index + 1 == bank->count) {
            /* The shipped bank's final stride includes an absent trailing word.
             * The original increments its pointer after copying that last table
             * but never reads there. Keep glyph reads inside the logical asset. */
            if (stride != available &&
                !(stride > available && stride - available == DD2_FONT_WORD_BYTES)) {
                return false;
            }
        } else if (stride > available) {
            return false;
        }
        if (!dd2_font_record((dd2_byte_view){.data = bytes.data + offset, .size = available},
                             &bank->fonts[index])) {
            return false;
        }
        if (index + 1 < bank->count) {
            offset += stride;
        }
    }
    return bank->count != 0 || bytes.size == DD2_FONT_FILE_HEADER;
}

dd2_font_bank *dd2_font_bank_create(dd2_byte_view bytes) {
    if (bytes.data == NULL || bytes.size < DD2_FONT_FILE_HEADER ||
        dd2_read_le32(bytes.data + DD2_FONT_MAGIC_OFFSET) != dd2_font_magic ||
        dd2_read_le32(bytes.data + DD2_FONT_SIZE_OFFSET) != bytes.size) {
        return NULL;
    }
    const size_t count = dd2_read_le32(bytes.data + DD2_FONT_COUNT_OFFSET);
    const size_t minimum =
        DD2_FONT_RECORD_HEADER + ((size_t)DD2_FONT_GLYPHS * DD2_FONT_GLYPH_BYTES);
    if (count > (bytes.size - DD2_FONT_FILE_HEADER) / minimum ||
        count > (SIZE_MAX - sizeof(dd2_font_bank)) / sizeof(dd2_font)) {
        return NULL;
    }
    dd2_font_bank *bank = malloc(sizeof(*bank) + (count * sizeof(dd2_font)));
    if (bank == NULL) {
        return NULL;
    }
    bank->count = count;
    if (!dd2_font_decode(bytes, bank)) {
        free(bank);
        return NULL;
    }
    return bank;
}

void dd2_font_bank_destroy(dd2_font_bank *bank) {
    free(bank);
}

size_t dd2_font_bank_count(const dd2_font_bank *bank) {
    return bank == NULL ? 0 : bank->count;
}

bool dd2_font_bank_glyph(const dd2_font_bank *bank, size_t font, uint8_t character,
                         dd2_font_glyph *result) {
    if (result == NULL) {
        return false;
    }
    *result = (dd2_font_glyph){0};
    if (bank == NULL || font >= bank->count || character < DD2_FONT_FIRST_CHARACTER ||
        (unsigned)character - DD2_FONT_FIRST_CHARACTER >= DD2_FONT_GLYPHS) {
        return false;
    }
    *result = bank->fonts[font].glyphs[(unsigned)character - DD2_FONT_FIRST_CHARACTER];
    return true;
}
