#ifndef DD2_ASSETS_FONT_H
#define DD2_ASSETS_FONT_H

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { DD2_FONT_FIRST_CHARACTER = 32, DD2_FONT_GLYPHS = 96 };

typedef struct dd2_font_bank dd2_font_bank;

typedef struct {
    uint8_t u;
    uint8_t v;
    uint8_t width;
    uint8_t height;
} dd2_font_glyph;

/* Decode the original FONT.BNK glyph tables into owned storage. The input may
 * be released after creation. Texture pixels and sprite/font bindings are
 * separate assets; zero-sized glyphs are valid unavailable characters.
 * Invalid/truncated layouts or allocation failure return null. */
dd2_font_bank *dd2_font_bank_create(dd2_byte_view bytes);
void dd2_font_bank_destroy(dd2_font_bank *bank);
size_t dd2_font_bank_count(const dd2_font_bank *bank);

/* Characters 32..127 index the original 96-entry tables. A valid zero-sized
 * glyph is returned successfully; invalid arguments clear the output. */
bool dd2_font_bank_glyph(const dd2_font_bank *bank, size_t font, uint8_t character,
                         dd2_font_glyph *result);

#endif
