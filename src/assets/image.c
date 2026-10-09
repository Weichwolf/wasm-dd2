#include "assets/image.h"

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zconf.h>
#include <zlib.h>

enum {
    DD2_PNG_SIGNATURE_BYTES = 8,
    DD2_PNG_CHUNK_BYTES = 12,
    DD2_PNG_TYPE_BYTES = 4,
    DD2_PNG_MAX_BYTES = 32 * 1024 * 1024,
    DD2_PNG_MAX_SIDE = 2048,
    DD2_PNG_HEADER_BYTES = 13,
    DD2_PNG_DEPTH = 8,
    DD2_PNG_DEPTH_OFFSET = 8,
    DD2_PNG_COLOR_OFFSET = 9,
    DD2_PNG_COMPRESSION_OFFSET = 10,
    DD2_PNG_FILTER_OFFSET = 11,
    DD2_PNG_INTERLACE_OFFSET = 12,
    DD2_PNG_CHANNELS = 4,
    DD2_PNG_PALETTE_CHANNELS = 3,
    DD2_PNG_MAX_PALETTE_ENTRIES = 256,
    DD2_PNG_GRAY = 0,
    DD2_PNG_RGB = 2,
    DD2_PNG_GRAY_ALPHA = 4,
    DD2_PNG_RGBA = 6,
    DD2_PNG_CASE_BIT = 32,
    DD2_PNG_FILTER_NONE = 0,
    DD2_PNG_FILTER_SUB = 1,
    DD2_PNG_FILTER_UP = 2,
    DD2_PNG_FILTER_AVERAGE = 3,
    DD2_PNG_FILTER_PAETH = 4
};

struct dd2_image {
    size_t width;
    size_t height;
    uint8_t *pixels;
};

typedef struct {
    size_t width;
    size_t height;
    size_t channels;
    uint8_t *compressed;
    size_t compressed_bytes;
    bool header;
    bool palette;
    bool data_started;
    bool data_closed;
    bool ended;
} dd2_png_source;

typedef struct {
    const uint8_t *type;
    const uint8_t *payload;
    size_t length;
} dd2_png_chunk;

static uint32_t dd2_png_word(const uint8_t *bytes) {
    uint32_t value = 0;
    for (size_t index = 0; index < sizeof(value); ++index) {
        value = (value << DD2_BYTE_BITS) | bytes[index];
    }
    return value;
}

static bool dd2_png_type(const uint8_t *type) {
    for (size_t index = 0; index < DD2_PNG_TYPE_BYTES; ++index) {
        if (!((type[index] >= 'A' && type[index] <= 'Z') ||
              (type[index] >= 'a' && type[index] <= 'z'))) {
            return false;
        }
    }
    return true;
}

static bool dd2_png_header(dd2_png_source *source, const uint8_t *bytes, size_t length) {
    if (length != DD2_PNG_HEADER_BYTES || bytes[DD2_PNG_DEPTH_OFFSET] != DD2_PNG_DEPTH ||
        bytes[DD2_PNG_COMPRESSION_OFFSET] != 0 || bytes[DD2_PNG_FILTER_OFFSET] != 0 ||
        bytes[DD2_PNG_INTERLACE_OFFSET] != 0) {
        return false;
    }
    source->width = dd2_png_word(bytes);
    source->height = dd2_png_word(bytes + DD2_PNG_TYPE_BYTES);
    if (source->width == 0 || source->height == 0 || source->width > DD2_PNG_MAX_SIDE ||
        source->height > DD2_PNG_MAX_SIDE) {
        return false;
    }
    switch (bytes[DD2_PNG_COLOR_OFFSET]) {
    case DD2_PNG_GRAY:
        source->channels = 1;
        break;
    case DD2_PNG_RGB:
        source->channels = 3;
        break;
    case DD2_PNG_GRAY_ALPHA:
        source->channels = 2;
        break;
    case DD2_PNG_RGBA:
        source->channels = 4;
        break;
    default:
        return false;
    }
    return true;
}

static bool dd2_png_accept_chunk(dd2_png_source *source, dd2_png_chunk chunk) {
    if (memcmp(chunk.type, "IHDR", DD2_PNG_TYPE_BYTES) == 0) {
        if (source->header) {
            return false;
        }
        source->header = dd2_png_header(source, chunk.payload, chunk.length);
        return source->header;
    }
    if (!source->header || memcmp(chunk.type, "tRNS", DD2_PNG_TYPE_BYTES) == 0) {
        /* Material transparency uses an explicit alpha channel. */
        return false;
    }
    if (memcmp(chunk.type, "PLTE", DD2_PNG_TYPE_BYTES) == 0) {
        if (source->palette || source->data_started ||
            source->channels < DD2_PNG_PALETTE_CHANNELS || chunk.length == 0 ||
            chunk.length % DD2_PNG_PALETTE_CHANNELS != 0 ||
            chunk.length / DD2_PNG_PALETTE_CHANNELS > DD2_PNG_MAX_PALETTE_ENTRIES) {
            return false;
        }
        source->palette = true;
        return true;
    }
    if (memcmp(chunk.type, "IDAT", DD2_PNG_TYPE_BYTES) == 0) {
        if (source->data_closed) {
            return false;
        }
        source->data_started = true;
        for (size_t index = 0; index < chunk.length; ++index) {
            source->compressed[source->compressed_bytes + index] = chunk.payload[index];
        }
        source->compressed_bytes += chunk.length;
        return true;
    }
    if (memcmp(chunk.type, "IEND", DD2_PNG_TYPE_BYTES) == 0) {
        source->ended = chunk.length == 0 && source->compressed_bytes != 0;
        return source->ended;
    }
    if ((chunk.type[0] & DD2_PNG_CASE_BIT) == 0) {
        return false;
    }
    source->data_closed = source->data_started;
    return true;
}

static bool dd2_png_chunks(dd2_byte_view bytes, dd2_png_source *source) {
    size_t cursor = DD2_PNG_SIGNATURE_BYTES;
    while (bytes.size - cursor >= DD2_PNG_CHUNK_BYTES) {
        const uint32_t length = dd2_png_word(bytes.data + cursor);
        if (length > bytes.size - cursor - DD2_PNG_CHUNK_BYTES) {
            return false;
        }
        const uint8_t *type = bytes.data + cursor + DD2_PNG_TYPE_BYTES;
        const uint8_t *payload = type + DD2_PNG_TYPE_BYTES;
        const uint32_t expected_crc = dd2_png_word(payload + length);
        const uLong actual_crc = crc32(0, type, (uInt)(length + DD2_PNG_TYPE_BYTES));
        if (!dd2_png_type(type) || actual_crc != expected_crc ||
            !dd2_png_accept_chunk(source, (dd2_png_chunk){type, payload, length})) {
            return false;
        }
        cursor += (size_t)length + DD2_PNG_CHUNK_BYTES;
        if (source->ended) {
            return cursor == bytes.size;
        }
    }
    return false;
}

typedef struct {
    unsigned left;
    unsigned above;
    unsigned corner;
} dd2_png_neighbors;

typedef struct {
    uint8_t *values;
    const uint8_t *previous;
    size_t bytes;
    size_t channels;
    uint8_t filter;
} dd2_png_row;

static unsigned dd2_png_paeth(dd2_png_neighbors neighbors) {
    const int prediction = (int)neighbors.left + (int)neighbors.above - (int)neighbors.corner;
    const int left_distance = abs(prediction - (int)neighbors.left);
    const int up_distance = abs(prediction - (int)neighbors.above);
    const int corner_distance = abs(prediction - (int)neighbors.corner);
    if (left_distance <= up_distance && left_distance <= corner_distance) {
        return neighbors.left;
    }
    return up_distance <= corner_distance ? neighbors.above : neighbors.corner;
}

static bool dd2_png_unfilter(dd2_png_row row) {
    if (row.filter > DD2_PNG_FILTER_PAETH) {
        return false;
    }
    for (size_t byte = 0; byte < row.bytes; ++byte) {
        const dd2_png_neighbors neighbors = {
            .left = byte < row.channels ? 0 : row.values[byte - row.channels],
            .above = row.previous == NULL ? 0 : row.previous[byte],
            .corner = row.previous == NULL || byte < row.channels
                          ? 0
                          : row.previous[byte - row.channels]};
        unsigned predictor = 0;
        switch (row.filter) {
        case DD2_PNG_FILTER_SUB:
            predictor = neighbors.left;
            break;
        case DD2_PNG_FILTER_UP:
            predictor = neighbors.above;
            break;
        case DD2_PNG_FILTER_AVERAGE:
            predictor = (neighbors.left + neighbors.above) / 2;
            break;
        case DD2_PNG_FILTER_PAETH:
            predictor = dd2_png_paeth(neighbors);
            break;
        default:
            break;
        }
        row.values[byte] = (uint8_t)((unsigned)row.values[byte] + predictor);
    }
    return true;
}

static bool dd2_png_rows(const dd2_png_source *source, uint8_t *decoded, dd2_image *image) {
    const size_t row_bytes = source->width * source->channels;
    const size_t stride = row_bytes + 1;
    for (size_t row = 0; row < source->height; ++row) {
        uint8_t *values = decoded + (row * stride) + 1;
        const uint8_t *previous = row == 0 ? NULL : values - stride;
        if (!dd2_png_unfilter((dd2_png_row){.values = values,
                                            .previous = previous,
                                            .bytes = row_bytes,
                                            .channels = source->channels,
                                            .filter = decoded[row * stride]})) {
            return false;
        }
        for (size_t column = 0; column < source->width; ++column) {
            const uint8_t *pixel = values + (column * source->channels);
            uint8_t *rgba = image->pixels + (((row * source->width) + column) * DD2_PNG_CHANNELS);
            rgba[0] = pixel[0];
            rgba[1] = source->channels < 3 ? pixel[0] : pixel[1];
            rgba[2] = source->channels < 3 ? pixel[0] : pixel[2];
            rgba[3] = source->channels == 2 || source->channels == 4 ? pixel[source->channels - 1]
                                                                     : UINT8_MAX;
        }
    }
    return true;
}

dd2_image *dd2_image_create_png(dd2_byte_view bytes) {
    const char signature[] = "\211PNG\r\n\032\n";
    if (bytes.data == NULL || bytes.size < DD2_PNG_SIGNATURE_BYTES ||
        bytes.size > DD2_PNG_MAX_BYTES ||
        memcmp(bytes.data, signature, DD2_PNG_SIGNATURE_BYTES) != 0) {
        return NULL;
    }
    dd2_png_source source = {.compressed = malloc(bytes.size)};
    if (source.compressed == NULL) {
        return NULL;
    }
    if (!dd2_png_chunks(bytes, &source)) {
        free(source.compressed);
        return NULL;
    }
    const size_t row_bytes = source.width * source.channels;
    const size_t decoded_bytes = (row_bytes + 1) * source.height;
    uint8_t *decoded = malloc(decoded_bytes);
    dd2_image *image = calloc(1, sizeof(*image));
    if (decoded == NULL || image == NULL) {
        free(source.compressed);
        free(decoded);
        free(image);
        return NULL;
    }
    image->width = source.width;
    image->height = source.height;
    image->pixels = malloc(source.width * source.height * DD2_PNG_CHANNELS);
    uLongf destination_bytes = (uLongf)decoded_bytes;
    uLong compressed_bytes = (uLong)source.compressed_bytes;
    const bool passed =
        image->pixels != NULL &&
        uncompress2(decoded, &destination_bytes, source.compressed, &compressed_bytes) == Z_OK &&
        destination_bytes == decoded_bytes && compressed_bytes == source.compressed_bytes &&
        dd2_png_rows(&source, decoded, image);
    free(source.compressed);
    free(decoded);
    if (!passed) {
        dd2_image_destroy(image);
        return NULL;
    }
    return image;
}

void dd2_image_destroy(dd2_image *image) {
    if (image != NULL) {
        free(image->pixels);
        free(image);
    }
}
size_t dd2_image_width(const dd2_image *image) {
    return image != NULL ? image->width : 0;
}
size_t dd2_image_height(const dd2_image *image) {
    return image != NULL ? image->height : 0;
}
size_t dd2_image_rgba_bytes(const dd2_image *image) {
    return image != NULL ? image->width * image->height * DD2_PNG_CHANNELS : 0;
}
const uint8_t *dd2_image_pixels(const dd2_image *image) {
    return image != NULL ? image->pixels : NULL;
}
