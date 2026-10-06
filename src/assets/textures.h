#ifndef DD2_ASSETS_TEXTURES_H
#define DD2_ASSETS_TEXTURES_H

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    DD2_TEXTURE_PAGE_SIDE = 256,
    DD2_TEXTURE_PAGE_COUNT = 32,
    DD2_TEXTURE_PAGE_PIXELS = DD2_TEXTURE_PAGE_SIDE * DD2_TEXTURE_PAGE_SIDE,
    DD2_TEXTURE_PAGE_RGBA_BYTES = DD2_TEXTURE_PAGE_PIXELS * 4,
    DD2_TEXTURE_MAX_PARTS = 10,
    DD2_TEXTURE_SHADES = 16
};

typedef struct dd2_texture_set dd2_texture_set;

typedef struct {
    dd2_byte_view parts[DD2_TEXTURE_MAX_PARTS];
    size_t part_count;
    dd2_byte_view palette;
    dd2_byte_view cluts;
    dd2_byte_view extra_cluts;
} dd2_texture_sources;

typedef struct {
    unsigned page;
    unsigned palette_bank;
    unsigned shade;
    bool cutout;
} dd2_texture_sample;

typedef struct {
    unsigned palette_bank;
    unsigned shade;
    uint8_t index;
} dd2_palette_sample;

/* Resolves one source texel index through a base/ECL shade row. Failed lookup
 * clears the result. This also allows material policies to distinguish palette
 * index zero from a nonzero index with the same RGB color. */
bool dd2_texture_palette_index(const dd2_texture_set *textures, dd2_palette_sample sample,
                               uint8_t *result);

/* Create validates all TX parts and owns the assembled index atlas. PAL, CLT
 * and optional ECL bytes are borrowed and must outlive this object. No GL/OS
 * state is involved. Failed create returns null; null destroy is harmless. */
dd2_texture_set *dd2_texture_set_create(const dd2_texture_sources *sources);
void dd2_texture_set_destroy(dd2_texture_set *textures);
size_t dd2_texture_image_count(const dd2_texture_set *textures);
size_t dd2_texture_palette_bank_count(const dd2_texture_set *textures);
dd2_byte_view dd2_texture_indices(const dd2_texture_set *textures);

/* A page is 256x256 RGBA8, with row zero corresponding to source V=0. Shade
 * ranges from 0 to 15; the original neutral lookup row is 8. Palette banks are
 * selected by polygons, not the reserved word in the texture definition.
 * Cutout makes texels whose low nibble is zero transparent, as in the original.
 * Invalid arguments leave the caller's output bytes unchanged. */
bool dd2_texture_page_rgba(const dd2_texture_set *textures, dd2_texture_sample sample,
                           dd2_byte_buffer output);

#endif
