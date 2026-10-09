#ifndef DD2_ASSETS_IMAGE_H
#define DD2_ASSETS_IMAGE_H

#include "assets/bytes.h"

#include <stddef.h>
#include <stdint.h>

typedef struct dd2_image dd2_image;

/* Owns validated 8-bit noninterlaced PNG pixels, expanded to top-first RGBA8.
 * Supports grayscale, grayscale-alpha, RGB and RGBA. CRC, chunk ordering,
 * dimensions, filters and exact compressed/decoded extents are checked.
 * Color samples are retained without gamma/profile conversion. Palette-indexed,
 * interlaced, non-8-bit and color-key transparency (tRNS) files are rejected;
 * authored transparent images must use an explicit alpha channel.
 * Input bytes can be released immediately after construction. */
dd2_image *dd2_image_create_png(dd2_byte_view bytes);
void dd2_image_destroy(dd2_image *image);
size_t dd2_image_width(const dd2_image *image);
size_t dd2_image_height(const dd2_image *image);
size_t dd2_image_rgba_bytes(const dd2_image *image);
const uint8_t *dd2_image_pixels(const dd2_image *image);

#endif
