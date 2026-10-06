#include "assets/textures.h"

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    DD2_TEXTURE_DESCRIPTOR_BYTES = 16,
    DD2_TEXTURE_HEADER_BYTES = 4,
    DD2_TEXTURE_PALETTE_BYTES = 1024,
    DD2_TEXTURE_CLUT_BANK_BYTES = DD2_TEXTURE_SHADES * DD2_TEXTURE_PAGE_SIDE,
    DD2_TEXTURE_MAX_PALETTE_BANKS = 256,
    DD2_TEXTURE_ATLAS_HEIGHT = DD2_TEXTURE_PAGE_COUNT * DD2_TEXTURE_PAGE_SIDE,
    DD2_TEXTURE_ATLAS_BYTES = DD2_TEXTURE_PAGE_COUNT * DD2_TEXTURE_PAGE_PIXELS,
    DD2_TEXTURE_WIDTH_OFFSET = 2,
    DD2_TEXTURE_HEIGHT_OFFSET = 4,
    DD2_TEXTURE_X_OFFSET = 6,
    DD2_TEXTURE_Y_OFFSET = 8,
    DD2_TEXTURE_FORMAT_4 = 4,
    DD2_TEXTURE_FORMAT_8 = 8,
    DD2_TEXTURE_PAYLOAD_MAGIC = 0x54584554,
    DD2_RGBA_CHANNELS = 4,
    DD2_OPAQUE_ALPHA = 255,
    DD2_TEXTURE_OPACITY_MASK = 15
};

struct dd2_texture_set {
    uint8_t *atlas;
    size_t image_count;
    dd2_byte_view palette;
    dd2_byte_view cluts;
    dd2_byte_view extra_cluts;
};

typedef struct {
    size_t width;
    size_t height;
    size_t x;
    size_t y;
} dd2_texture_image;

static bool dd2_texture_sources_valid(const dd2_texture_sources *sources) {
    if (sources == NULL || sources->part_count == 0 ||
        sources->part_count > DD2_TEXTURE_MAX_PARTS || sources->palette.data == NULL ||
        sources->palette.size != DD2_TEXTURE_PALETTE_BYTES || sources->cluts.data == NULL ||
        sources->cluts.size == 0 || sources->cluts.size % DD2_TEXTURE_CLUT_BANK_BYTES != 0 ||
        sources->cluts.size > (size_t)DD2_TEXTURE_MAX_PALETTE_BANKS * DD2_TEXTURE_CLUT_BANK_BYTES ||
        sources->extra_cluts.size % DD2_TEXTURE_CLUT_BANK_BYTES != 0 ||
        (sources->extra_cluts.size != 0 && sources->extra_cluts.data == NULL)) {
        return false;
    }
    const size_t extra_banks = sources->extra_cluts.size / DD2_TEXTURE_CLUT_BANK_BYTES;
    const size_t base_banks = sources->cluts.size / DD2_TEXTURE_CLUT_BANK_BYTES;
    if (extra_banks > DD2_TEXTURE_MAX_PALETTE_BANKS - base_banks) {
        return false;
    }
    for (size_t part = 0; part < sources->part_count; ++part) {
        if (sources->parts[part].data == NULL ||
            sources->parts[part].size < DD2_TEXTURE_HEADER_BYTES) {
            return false;
        }
    }
    return true;
}

static bool dd2_texture_image_decode(const uint8_t *bytes, dd2_texture_image *image) {
    const unsigned format = dd2_read_le16(bytes);
    *image = (dd2_texture_image){.width = dd2_read_le16(bytes + DD2_TEXTURE_WIDTH_OFFSET),
                                 .height = dd2_read_le16(bytes + DD2_TEXTURE_HEIGHT_OFFSET),
                                 .x = dd2_read_le16(bytes + DD2_TEXTURE_X_OFFSET),
                                 .y = dd2_read_le16(bytes + DD2_TEXTURE_Y_OFFSET)};
    return (format == DD2_TEXTURE_FORMAT_4 || format == DD2_TEXTURE_FORMAT_8) &&
           image->width != 0 && image->height != 0 && image->width <= DD2_TEXTURE_PAGE_SIDE &&
           image->height <= DD2_TEXTURE_ATLAS_HEIGHT &&
           image->x <= DD2_TEXTURE_PAGE_SIDE - image->width &&
           image->y <= DD2_TEXTURE_ATLAS_HEIGHT - image->height;
}

static bool dd2_texture_atlas_load(dd2_texture_set *textures, const dd2_texture_sources *sources) {
    const dd2_byte_view first = sources->parts[0];
    textures->image_count = dd2_read_le32(first.data);
    if (textures->image_count == 0 ||
        textures->image_count >
            (first.size - DD2_TEXTURE_HEADER_BYTES) / DD2_TEXTURE_DESCRIPTOR_BYTES) {
        return false;
    }
    size_t image_index = 0;
    for (size_t part = 0; part < sources->part_count; ++part) {
        const dd2_byte_view bytes = sources->parts[part];
        size_t offset = part == 0 ? DD2_TEXTURE_HEADER_BYTES +
                                        (textures->image_count * DD2_TEXTURE_DESCRIPTOR_BYTES)
                                  : 0;
        while (offset < bytes.size) {
            if (image_index >= textures->image_count ||
                bytes.size - offset < DD2_TEXTURE_HEADER_BYTES) {
                return false;
            }
            dd2_texture_image image = {0};
            const uint8_t *descriptor = first.data + DD2_TEXTURE_HEADER_BYTES +
                                        (image_index * DD2_TEXTURE_DESCRIPTOR_BYTES);
            if (!dd2_texture_image_decode(descriptor, &image) ||
                dd2_read_le32(bytes.data + offset) != DD2_TEXTURE_PAYLOAD_MAGIC) {
                return false;
            }
            offset += DD2_TEXTURE_HEADER_BYTES;
            const size_t image_bytes = image.width * image.height;
            if (image_bytes > bytes.size - offset) {
                return false;
            }
            for (size_t row = 0; row < image.height; ++row) {
                const size_t destination = ((image.y + row) * DD2_TEXTURE_PAGE_SIDE) + image.x;
                const size_t source = offset + (row * image.width);
                for (size_t column = 0; column < image.width; ++column) {
                    textures->atlas[destination + column] = bytes.data[source + column];
                }
            }
            offset += image_bytes;
            ++image_index;
        }
    }
    return image_index == textures->image_count;
}

dd2_texture_set *dd2_texture_set_create(const dd2_texture_sources *sources) {
    if (!dd2_texture_sources_valid(sources)) {
        return NULL;
    }
    dd2_texture_set *textures = calloc(1, sizeof(*textures));
    if (textures == NULL) {
        return NULL;
    }
    textures->atlas = calloc(DD2_TEXTURE_ATLAS_BYTES, 1);
    if (textures->atlas == NULL || !dd2_texture_atlas_load(textures, sources)) {
        dd2_texture_set_destroy(textures);
        return NULL;
    }
    textures->palette = sources->palette;
    textures->cluts = sources->cluts;
    textures->extra_cluts = sources->extra_cluts;
    return textures;
}

void dd2_texture_set_destroy(dd2_texture_set *textures) {
    if (textures != NULL) {
        free(textures->atlas);
        free(textures);
    }
}

size_t dd2_texture_image_count(const dd2_texture_set *textures) {
    return textures != NULL ? textures->image_count : 0;
}

size_t dd2_texture_palette_bank_count(const dd2_texture_set *textures) {
    return textures != NULL
               ? (textures->cluts.size + textures->extra_cluts.size) / DD2_TEXTURE_CLUT_BANK_BYTES
               : 0;
}

dd2_byte_view dd2_texture_indices(const dd2_texture_set *textures) {
    return textures != NULL
               ? (dd2_byte_view){.data = textures->atlas, .size = DD2_TEXTURE_ATLAS_BYTES}
               : (dd2_byte_view){0};
}

static const uint8_t *dd2_texture_lookup(const dd2_texture_set *textures,
                                         dd2_palette_sample sample) {
    if (textures == NULL || sample.palette_bank >= dd2_texture_palette_bank_count(textures) ||
        sample.shade >= DD2_TEXTURE_SHADES) {
        return NULL;
    }
    size_t offset = (size_t)sample.palette_bank * DD2_TEXTURE_CLUT_BANK_BYTES;
    const uint8_t *cluts = textures->cluts.data;
    if (offset >= textures->cluts.size) {
        offset -= textures->cluts.size;
        cluts = textures->extra_cluts.data;
    }
    return cluts + offset + ((size_t)sample.shade * DD2_TEXTURE_PAGE_SIDE);
}

bool dd2_texture_palette_index(const dd2_texture_set *textures, dd2_palette_sample sample,
                               uint8_t *result) {
    if (result == NULL) {
        return false;
    }
    *result = 0;
    const uint8_t *lookup = dd2_texture_lookup(textures, sample);
    if (lookup == NULL) {
        return false;
    }
    *result = lookup[sample.index];
    return true;
}

bool dd2_texture_page_rgba(const dd2_texture_set *textures, dd2_texture_sample sample,
                           dd2_byte_buffer output) {
    if (textures == NULL || sample.page >= DD2_TEXTURE_PAGE_COUNT ||
        sample.palette_bank >= dd2_texture_palette_bank_count(textures) ||
        sample.shade >= DD2_TEXTURE_SHADES || output.data == NULL ||
        output.size < DD2_TEXTURE_PAGE_RGBA_BYTES) {
        return false;
    }
    const uint8_t *lookup = dd2_texture_lookup(
        textures, (dd2_palette_sample){.palette_bank = sample.palette_bank, .shade = sample.shade});
    const uint8_t *indices = textures->atlas + ((size_t)sample.page * DD2_TEXTURE_PAGE_PIXELS);
    for (size_t index = 0; index < DD2_TEXTURE_PAGE_PIXELS; ++index) {
        const uint8_t texel = indices[index];
        const size_t palette_offset = (size_t)lookup[texel] * DD2_RGBA_CHANNELS;
        for (size_t channel = 0; channel < DD2_RGBA_CHANNELS - 1; ++channel) {
            output.data[(index * DD2_RGBA_CHANNELS) + channel] =
                textures->palette.data[palette_offset + (DD2_RGBA_CHANNELS - 2 - channel)];
        }
        output.data[(index * DD2_RGBA_CHANNELS) + DD2_RGBA_CHANNELS - 1] =
            sample.cutout && (texel & DD2_TEXTURE_OPACITY_MASK) == 0 ? 0 : DD2_OPAQUE_ALPHA;
    }
    return true;
}
