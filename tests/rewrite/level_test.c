#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/textures.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_TEST_LEVEL_HEADER_BYTES = DD2_LEVEL_SECTION_COUNT * DD2_TEST_WORD_BYTES,
    DD2_TEST_VERTEX_BYTES = 12,
    DD2_TEST_DEFINITION_BYTES = 16,
    DD2_TEST_LEVEL_BYTES =
        DD2_TEST_LEVEL_HEADER_BYTES + DD2_TEST_VERTEX_BYTES + DD2_TEST_DEFINITION_BYTES,
    DD2_TEST_UV_VALUE = 190,
    DD2_TEST_ATLAS_BYTES = DD2_TEXTURE_PAGE_COUNT * DD2_TEXTURE_PAGE_PIXELS
};

static void dd2_test_level_fixture(uint8_t *bytes) {
    for (size_t index = 0; index < DD2_TEST_LEVEL_BYTES; ++index) {
        bytes[index] = 0;
    }
    for (size_t index = 0; index < DD2_LEVEL_SECTION_COUNT; ++index) {
        uint32_t offset = DD2_TEST_LEVEL_BYTES;
        if (index <= DD2_LEVEL_ROAD_VERTICES) {
            offset = DD2_TEST_LEVEL_HEADER_BYTES;
        } else if (index <= DD2_LEVEL_TEXTURE_DEFINITIONS) {
            offset = DD2_TEST_LEVEL_HEADER_BYTES + DD2_TEST_VERTEX_BYTES;
        }
        dd2_test_write_le32(bytes + (index * DD2_TEST_WORD_BYTES), offset);
    }
    uint8_t *vertex = bytes + DD2_TEST_LEVEL_HEADER_BYTES;
    dd2_test_write_le32(vertex, UINT32_MAX);
    dd2_test_write_le32(vertex + DD2_TEST_WORD_BYTES, (uint32_t)INT32_MIN);
    dd2_test_write_le32(vertex + ((size_t)2 * DD2_TEST_WORD_BYTES), INT32_MAX);
    uint8_t *definitions = vertex + DD2_TEST_VERTEX_BYTES;
    dd2_test_write_le32(definitions, 1);
    dd2_test_write_le16(definitions + DD2_TEST_WORD_BYTES, DD2_TEXTURE_PAGE_COUNT - 1);
    definitions[DD2_TEST_WORD_BYTES + DD2_TEST_WORD_BYTES] = DD2_TEST_UV_VALUE;
    definitions[DD2_TEST_WORD_BYTES + DD2_TEST_WORD_BYTES + 1] = UINT8_MAX;
}

static bool dd2_test_level_valid(const uint8_t *bytes) {
    dd2_level_data level = {0};
    dd2_track_vertex vertex = {0};
    dd2_texture_definition definition = {0};
    const dd2_byte_view view = {.data = bytes, .size = DD2_TEST_LEVEL_BYTES};
    return dd2_level_decode(view, &level) && level.vertex_count == 1 &&
           level.texture_definition_count == 1 &&
           level.sections[DD2_LEVEL_ROAD_VERTICES].data == bytes + DD2_TEST_LEVEL_HEADER_BYTES &&
           dd2_level_vertex(&level, 0, &vertex) && vertex.x == -1 && vertex.y == INT32_MIN &&
           vertex.z == INT32_MAX && dd2_level_texture_definition(&level, 0, &definition) &&
           definition.page_flags == DD2_TEXTURE_PAGE_COUNT - 1 && definition.reserved == 0 &&
           definition.corners[0].u == DD2_TEST_UV_VALUE && definition.corners[0].v == UINT8_MAX &&
           !dd2_level_vertex(&level, 1, &vertex) && vertex.x == 0 &&
           !dd2_level_vertex(&level, SIZE_MAX, &vertex) && !dd2_level_vertex(NULL, 0, &vertex) &&
           !dd2_level_vertex(&level, 0, NULL) &&
           !dd2_level_texture_definition(&level, 1, &definition) && definition.page_flags == 0 &&
           !dd2_level_texture_definition(NULL, 0, &definition) &&
           !dd2_level_texture_definition(&level, 0, NULL) && !dd2_level_decode(view, NULL);
}

static bool dd2_expect_bad_level(dd2_byte_view bytes) {
    dd2_level_data level = {.vertex_count = 1, .texture_definition_count = 1};
    const bool passed = !dd2_level_decode(bytes, &level) && level.vertex_count == 0 &&
                        level.texture_definition_count == 0;
    for (size_t index = 0; index < DD2_LEVEL_SECTION_COUNT; ++index) {
        if (level.sections[index].data != NULL || level.sections[index].size != 0) {
            return false;
        }
    }
    return passed;
}

static bool dd2_test_level_bounds(void) {
    uint8_t bytes[DD2_TEST_LEVEL_BYTES] = {0};
    dd2_test_level_fixture(bytes);
    const dd2_byte_view view = {.data = bytes, .size = sizeof(bytes)};
    dd2_level_data invalid_metadata = {.vertex_count = 1, .texture_definition_count = 1};
    dd2_track_vertex vertex = {0};
    dd2_texture_definition definition = {0};
    if (dd2_level_vertex(&invalid_metadata, 0, &vertex) ||
        dd2_level_texture_definition(&invalid_metadata, 0, &definition)) {
        return false;
    }
    if (!dd2_test_level_valid(bytes) || !dd2_expect_bad_level((dd2_byte_view){0}) ||
        !dd2_expect_bad_level(
            (dd2_byte_view){.data = bytes, .size = DD2_TEST_LEVEL_HEADER_BYTES - 1})) {
        return false;
    }
    dd2_test_write_le32(bytes, 0);
    if (!dd2_expect_bad_level(view)) {
        return false;
    }
    dd2_test_level_fixture(bytes);
    dd2_test_write_le32(bytes + DD2_TEST_WORD_BYTES, DD2_TEST_LEVEL_HEADER_BYTES - 1);
    if (!dd2_expect_bad_level(view)) {
        return false;
    }
    dd2_test_level_fixture(bytes);
    dd2_test_write_le32(bytes + DD2_TEST_WORD_BYTES, UINT32_MAX);
    if (!dd2_expect_bad_level(view)) {
        return false;
    }
    dd2_test_level_fixture(bytes);
    dd2_test_write_le32(bytes + ((size_t)DD2_LEVEL_SPRITES * DD2_TEST_WORD_BYTES),
                        DD2_TEST_LEVEL_HEADER_BYTES + DD2_TEST_VERTEX_BYTES - 1);
    if (!dd2_expect_bad_level(view)) {
        return false;
    }
    dd2_test_level_fixture(bytes);
    dd2_test_write_le32(bytes + DD2_TEST_LEVEL_HEADER_BYTES + DD2_TEST_VERTEX_BYTES, UINT32_MAX);
    return dd2_expect_bad_level(view);
}

static bool dd2_expect_bad_textures(const dd2_texture_sources *sources) {
    dd2_texture_set *textures = dd2_texture_set_create(sources);
    const bool passed = textures == NULL;
    dd2_texture_set_destroy(textures);
    return passed;
}

static bool dd2_test_texture_bad_sources(dd2_test_texture_fixture *fixture) {
    dd2_texture_sources sources = dd2_test_texture_sources(fixture);
    if (!dd2_expect_bad_textures(NULL)) {
        return false;
    }
    sources.part_count = 0;
    if (!dd2_expect_bad_textures(&sources)) {
        return false;
    }
    sources.part_count = DD2_TEXTURE_MAX_PARTS + 1;
    if (!dd2_expect_bad_textures(&sources)) {
        return false;
    }
    sources = dd2_test_texture_sources(fixture);
    --sources.palette.size;
    if (!dd2_expect_bad_textures(&sources)) {
        return false;
    }
    sources = dd2_test_texture_sources(fixture);
    sources.cluts.size = 1;
    if (!dd2_expect_bad_textures(&sources)) {
        return false;
    }
    sources = dd2_test_texture_sources(fixture);
    sources.extra_cluts.data = NULL;
    if (!dd2_expect_bad_textures(&sources)) {
        return false;
    }
    sources = dd2_test_texture_sources(fixture);
    sources.part_count = 1;
    if (!dd2_expect_bad_textures(&sources)) {
        return false;
    }
    sources = dd2_test_texture_sources(fixture);
    sources.parts[1].data = NULL;
    if (!dd2_expect_bad_textures(&sources)) {
        return false;
    }
    sources = dd2_test_texture_sources(fixture);
    --sources.parts[1].size;
    return dd2_expect_bad_textures(&sources);
}

static bool dd2_test_texture_bad_descriptors(dd2_test_texture_fixture *fixture) {
    dd2_texture_sources sources = dd2_test_texture_sources(fixture);
    dd2_test_write_le32(fixture->first, UINT32_MAX);
    if (!dd2_expect_bad_textures(&sources)) {
        return false;
    }
    sources = dd2_test_texture_sources(fixture);
    dd2_test_write_le16(fixture->first + DD2_TEST_WORD_BYTES, 2);
    if (!dd2_expect_bad_textures(&sources)) {
        return false;
    }
    sources = dd2_test_texture_sources(fixture);
    dd2_test_write_le16(fixture->first + DD2_TEST_WORD_BYTES + 2, 0);
    if (!dd2_expect_bad_textures(&sources)) {
        return false;
    }
    sources = dd2_test_texture_sources(fixture);
    dd2_test_write_le16(fixture->first + DD2_TEST_WORD_BYTES + DD2_TEST_TX_X_OFFSET,
                        DD2_TEXTURE_PAGE_SIDE - 1);
    if (!dd2_expect_bad_textures(&sources)) {
        return false;
    }
    sources = dd2_test_texture_sources(fixture);
    dd2_test_write_le16(fixture->first + DD2_TEST_WORD_BYTES + DD2_TEST_TX_Y_OFFSET, UINT16_MAX);
    if (!dd2_expect_bad_textures(&sources)) {
        return false;
    }
    sources = dd2_test_texture_sources(fixture);
    dd2_test_write_le32(fixture->first + DD2_TEST_TX_DATA_OFFSET, 0);
    return dd2_expect_bad_textures(&sources);
}

static bool dd2_test_pixel(unsigned color, dd2_byte_buffer pixels, size_t index) {
    const size_t offset = index * DD2_TEST_COLOR_CHANNELS;
    return pixels.data[offset] == color && pixels.data[offset + 1] == UINT8_MAX - color &&
           pixels.data[offset + 2] == DD2_TEST_PALETTE_BLUE && pixels.data[offset + 3] == UINT8_MAX;
}

static bool dd2_test_texture_views(dd2_texture_set *textures, dd2_byte_buffer pixels) {
    const dd2_byte_view indices = dd2_texture_indices(textures);
    dd2_texture_sample sample = {.page = 0, .palette_bank = 0, .shade = DD2_TEST_NEUTRAL_SHADE};
    const size_t edge = DD2_TEXTURE_PAGE_PIXELS - 2;
    if (dd2_texture_image_count(textures) != 2 || dd2_texture_palette_bank_count(textures) != 2 ||
        indices.size != DD2_TEST_ATLAS_BYTES || indices.data[edge] != DD2_TEST_TEXEL_A ||
        indices.data[edge + 1] != DD2_TEST_TEXEL_B ||
        indices.data[DD2_TEXTURE_PAGE_PIXELS] != DD2_TEST_TEXEL_E ||
        indices.data[DD2_TEXTURE_PAGE_PIXELS + DD2_TEXTURE_PAGE_SIDE - 2] != DD2_TEST_TEXEL_C ||
        !dd2_texture_page_rgba(textures, sample, pixels) ||
        !dd2_test_pixel(DD2_TEST_TEXEL_A, pixels, edge) ||
        !dd2_test_pixel(DD2_TEST_TEXEL_B, pixels, edge + 1)) {
        return false;
    }
    sample.cutout = true;
    if (!dd2_texture_page_rgba(textures, sample, pixels) ||
        pixels.data[((edge + 1) * DD2_TEST_COLOR_CHANNELS) + 3] != 0) {
        return false;
    }
    sample.page = 1;
    if (!dd2_texture_page_rgba(textures, sample, pixels) ||
        !dd2_test_pixel(DD2_TEST_TEXEL_E, pixels, 0)) {
        return false;
    }
    sample.palette_bank = 1;
    if (!dd2_texture_page_rgba(textures, sample, pixels) ||
        !dd2_test_pixel(DD2_TEST_TEXEL_E + 1, pixels, 0)) {
        return false;
    }
    sample.palette_bank = 0;
    sample.shade = 0;
    if (!dd2_texture_page_rgba(textures, sample, pixels) ||
        !dd2_test_pixel(UINT8_MAX - DD2_TEST_TEXEL_E, pixels, 0)) {
        return false;
    }
    pixels.data[0] = DD2_TEST_UNUSED_BYTE;
    sample.page = DD2_TEXTURE_PAGE_COUNT;
    if (dd2_texture_page_rgba(textures, sample, pixels)) {
        return false;
    }
    sample.page = 0;
    sample.shade = DD2_TEXTURE_SHADES;
    if (dd2_texture_page_rgba(textures, sample, pixels)) {
        return false;
    }
    sample.shade = 0;
    sample.palette_bank = 2;
    if (dd2_texture_page_rgba(textures, sample, pixels)) {
        return false;
    }
    sample.palette_bank = 0;
    return !dd2_texture_page_rgba(NULL, sample, pixels) &&
           !dd2_texture_page_rgba(textures, sample, (dd2_byte_buffer){0}) &&
           !dd2_texture_page_rgba(
               textures, sample, (dd2_byte_buffer){.data = pixels.data, .size = pixels.size - 1}) &&
           pixels.data[0] == DD2_TEST_UNUSED_BYTE && dd2_texture_image_count(NULL) == 0 &&
           dd2_texture_palette_bank_count(NULL) == 0 && dd2_texture_indices(NULL).data == NULL;
}

static bool dd2_test_textures(void) {
    dd2_test_texture_fixture fixture = {0};
    const dd2_texture_sources sources = dd2_test_texture_sources(&fixture);
    dd2_texture_set *textures = dd2_texture_set_create(&sources);
    if (textures == NULL) {
        return false;
    }
    uint8_t *pixels = malloc(DD2_TEXTURE_PAGE_RGBA_BYTES);
    const bool passed =
        pixels != NULL &&
        dd2_test_texture_views(
            textures, (dd2_byte_buffer){.data = pixels, .size = DD2_TEXTURE_PAGE_RGBA_BYTES});
    free(pixels);
    dd2_texture_set_destroy(textures);
    dd2_texture_set_destroy(NULL);
    return passed && dd2_test_texture_bad_sources(&fixture) &&
           dd2_test_texture_bad_descriptors(&fixture);
}

int main(void) {
    if (!dd2_test_level_bounds() || !dd2_test_textures()) {
        return EXIT_FAILURE;
    }
    puts("{\"scope\":\"typed level data and texture decoding bounds\",\"pass\":true}");
    return EXIT_SUCCESS;
}
