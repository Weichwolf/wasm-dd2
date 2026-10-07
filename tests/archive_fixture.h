#ifndef DD2_TEST_ARCHIVE_FIXTURE_H
#define DD2_TEST_ARCHIVE_FIXTURE_H
#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/textures.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { DD2_FIXTURE_MAX_ARCHIVE_BYTES = 64 * 1024 * 1024 };
typedef struct {
    uint8_t *bytes;
    dd2_archive *archive;
} dd2_archive_fixture;

static inline void dd2_archive_fixture_close(dd2_archive_fixture *fixture) {
    dd2_archive_close(fixture->archive);
    free(fixture->bytes);
    *fixture = (dd2_archive_fixture){0};
}

static inline bool dd2_archive_fixture_open(const char *path, dd2_archive_fixture *fixture) {
    *fixture = (dd2_archive_fixture){0};
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    size_t size = 0;
    uint8_t *bytes = NULL;
    dd2_archive *archive = NULL;
    if (fseek(file, 0, SEEK_END) == 0) {
        const long length = ftell(file);
        if (length > 0 && length <= DD2_FIXTURE_MAX_ARCHIVE_BYTES &&
            fseek(file, 0, SEEK_SET) == 0) {
            size = (size_t)length;
            bytes = malloc(size);
            if (bytes != NULL && fread(bytes, 1, size, file) != size) {
                free(bytes);
                bytes = NULL;
            }
        }
    }
    const bool closed = fclose(file) == 0;
    if (!closed || bytes == NULL || dd2_archive_open(bytes, size, &archive) != DD2_ARCHIVE_OK) {
        dd2_archive_close(archive);
        free(bytes);
        return false;
    }
    *fixture = (dd2_archive_fixture){.bytes = bytes, .archive = archive};
    return true;
}
static inline dd2_byte_view dd2_find_view(const dd2_archive *archive, const char *name) {
    dd2_asset asset = {0};
    if (!dd2_archive_find(archive, name, &asset)) {
        return (dd2_byte_view){0};
    }
    return (dd2_byte_view){.data = asset.bytes, .size = asset.size};
}

static inline dd2_texture_sources dd2_export_texture_sources(const dd2_archive *archive,
                                                             char code) {
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

#endif
