#include "asset_fixture.h"
#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/textures.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_EXPORT_MAX_ARCHIVE_BYTES = 64 * 1024 * 1024, DD2_EXPORT_TEXTURE_DEFINITION_BYTES = 12 };

static bool dd2_export_word(FILE *file, uint32_t value) {
    uint8_t bytes[DD2_TEST_WORD_BYTES] = {0};
    dd2_test_write_le32(bytes, value);
    return fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes);
}

static dd2_byte_view dd2_find_view(const dd2_archive *archive, const char *name) {
    dd2_asset asset = {0};
    if (!dd2_archive_find(archive, name, &asset)) {
        return (dd2_byte_view){0};
    }
    return (dd2_byte_view){.data = asset.bytes, .size = asset.size};
}

static dd2_texture_sources dd2_export_texture_sources(const dd2_archive *archive, char code) {
    char palette[] = "LEV0\\LEVEL.PAL";
    char cluts[] = "LEV0\\LEVEL.CLT";
    char extra[] = "LEV0\\LEVEL.ECL";
    char part[] = "LEV0\\LEVEL.TX0";
    palette[3] = code;
    cluts[3] = code;
    extra[3] = code;
    part[3] = code;
    dd2_texture_sources sources = {.palette = dd2_find_view(archive, palette),
                                   .cluts = dd2_find_view(archive, cluts),
                                   .extra_cluts = dd2_find_view(archive, extra)};
    for (size_t index = 0; index < DD2_TEXTURE_MAX_PARTS; ++index) {
        part[sizeof(part) - 2] = (char)('0' + index);
        const dd2_byte_view bytes = dd2_find_view(archive, part);
        if (bytes.data == NULL) {
            break;
        }
        sources.parts[index] = bytes;
        ++sources.part_count;
    }
    return sources;
}

static bool dd2_export_level_data(FILE *file, const dd2_level_data *level, dd2_byte_view original) {
    for (size_t index = 0; index < DD2_LEVEL_SECTION_COUNT; ++index) {
        const dd2_byte_view section = level->sections[index];
        if (!dd2_export_word(file, (uint32_t)(section.data - original.data)) ||
            !dd2_export_word(file, (uint32_t)section.size)) {
            return false;
        }
    }
    for (size_t index = 0; index < level->vertex_count; ++index) {
        dd2_track_vertex vertex = {0};
        if (!dd2_level_vertex(level, index, &vertex) ||
            !dd2_export_word(file, (uint32_t)vertex.x) ||
            !dd2_export_word(file, (uint32_t)vertex.y) ||
            !dd2_export_word(file, (uint32_t)vertex.z)) {
            return false;
        }
    }
    for (size_t index = 0; index < level->texture_definition_count; ++index) {
        dd2_texture_definition definition = {0};
        if (!dd2_level_texture_definition(level, index, &definition)) {
            return false;
        }
        uint8_t bytes[DD2_EXPORT_TEXTURE_DEFINITION_BYTES] = {0};
        dd2_test_write_le16(bytes, definition.page_flags);
        dd2_test_write_le16(bytes + 2, definition.reserved);
        for (size_t corner = 0; corner < DD2_TEXTURE_CORNERS; ++corner) {
            bytes[DD2_TEST_WORD_BYTES + (corner * 2)] = definition.corners[corner].u;
            bytes[DD2_TEST_WORD_BYTES + (corner * 2) + 1] = definition.corners[corner].v;
        }
        if (fwrite(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
            return false;
        }
    }
    return true;
}

static bool dd2_export_page(FILE *file, const dd2_texture_set *textures, dd2_texture_sample sample,
                            dd2_byte_buffer buffer) {
    return dd2_texture_page_rgba(textures, sample, buffer) &&
           fwrite(buffer.data, 1, buffer.size, file) == buffer.size;
}

static bool dd2_export_texture_pages(FILE *file, const dd2_texture_set *textures) {
    const dd2_byte_view atlas = dd2_texture_indices(textures);
    if (fwrite(atlas.data, 1, atlas.size, file) != atlas.size) {
        return false;
    }
    uint8_t *pixels = malloc(DD2_TEXTURE_PAGE_RGBA_BYTES);
    if (pixels == NULL) {
        return false;
    }
    const dd2_byte_buffer buffer = {.data = pixels, .size = DD2_TEXTURE_PAGE_RGBA_BYTES};
    bool passed = true;
    for (unsigned page = 0; page < DD2_TEXTURE_PAGE_COUNT && passed; ++page) {
        passed = dd2_export_page(
            file, textures,
            (dd2_texture_sample){.page = page, .shade = DD2_TEST_NEUTRAL_SHADE, .cutout = true},
            buffer);
    }
    const unsigned last_bank = (unsigned)(dd2_texture_palette_bank_count(textures) - 1);
    passed = passed &&
             dd2_export_page(file, textures,
                             (dd2_texture_sample){.page = 0, .palette_bank = last_bank}, buffer) &&
             dd2_export_page(file, textures,
                             (dd2_texture_sample){.page = 0,
                                                  .palette_bank = last_bank,
                                                  .shade = DD2_TEXTURE_SHADES - 1,
                                                  .cutout = true},
                             buffer);
    free(pixels);
    return passed;
}

static bool dd2_export_level(FILE *file, const dd2_archive *archive, char code) {
    char name[] = "LEV0\\LEVEL.DAT";
    name[3] = code;
    const dd2_byte_view bytes = dd2_find_view(archive, name);
    dd2_level_data level = {0};
    if (!dd2_level_decode(bytes, &level)) {
        return false;
    }
    const dd2_texture_sources sources = dd2_export_texture_sources(archive, code);
    dd2_texture_set *textures = dd2_texture_set_create(&sources);
    if (textures == NULL) {
        return false;
    }
    const bool passed =
        fwrite(&code, 1, 1, file) == 1 && dd2_export_word(file, (uint32_t)level.vertex_count) &&
        dd2_export_word(file, (uint32_t)level.texture_definition_count) &&
        dd2_export_word(file, (uint32_t)dd2_texture_image_count(textures)) &&
        dd2_export_word(file, (uint32_t)dd2_texture_palette_bank_count(textures)) &&
        dd2_export_level_data(file, &level, bytes) && dd2_export_texture_pages(file, textures);
    dd2_texture_set_destroy(textures);
    return passed;
}

static bool dd2_export_archive(dd2_byte_view bytes, const char *path) {
    const char allowed[] = "/tmp/wasm-dd2/";
    if (strncmp(path, allowed, sizeof(allowed) - 1) != 0 || strstr(path, "..") != NULL) {
        return false;
    }
    dd2_archive *archive = NULL;
    if (dd2_archive_open(bytes.data, bytes.size, &archive) != DD2_ARCHIVE_OK) {
        return false;
    }
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        dd2_archive_close(archive);
        return false;
    }
    const char levels[] = "0123456789ABF";
    bool passed = true;
    for (size_t index = 0; index < sizeof(levels) - 1 && passed; ++index) {
        passed = dd2_export_level(file, archive, levels[index]);
    }
    const bool closed = fclose(file) == 0;
    dd2_archive_close(archive);
    return passed && closed;
}

typedef struct {
    const char *archive;
    const char *output;
} dd2_export_paths;

static bool dd2_export_original(dd2_export_paths paths) {
    FILE *file = fopen(paths.archive, "rb");
    if (file == NULL) {
        return false;
    }
    uint8_t *bytes = NULL;
    size_t size = 0;
    if (fseek(file, 0, SEEK_END) == 0) {
        const long length = ftell(file);
        if (length > 0 && length <= DD2_EXPORT_MAX_ARCHIVE_BYTES && fseek(file, 0, SEEK_SET) == 0) {
            size = (size_t)length;
            bytes = malloc(size);
            if (bytes != NULL && fread(bytes, 1, size, file) != size) {
                free(bytes);
                bytes = NULL;
            }
        }
    }
    const bool closed = fclose(file) == 0;
    const bool passed =
        closed && bytes != NULL &&
        dd2_export_archive((dd2_byte_view){.data = bytes, .size = size}, paths.output);
    free(bytes);
    return passed;
}

int main(int argc, char **argv) {
    if (argc != 3 ||
        !dd2_export_original((dd2_export_paths){.archive = argv[1], .output = argv[2]})) {
        return EXIT_FAILURE;
    }
    puts("{\"scope\":\"original level and texture exports\",\"pass\":true}");
    return EXIT_SUCCESS;
}
