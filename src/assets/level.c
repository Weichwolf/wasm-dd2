#include "assets/level.h"

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    DD2_DISK_WORD_BYTES = 4,
    DD2_DISK_VERTEX_BYTES = 12,
    DD2_DISK_TEXTURE_DEFINITION_BYTES = 12
};

bool dd2_level_decode(dd2_byte_view bytes, dd2_level_data *result) {
    if (result == NULL) {
        return false;
    }
    *result = (dd2_level_data){0};
    const size_t header_bytes = (size_t)DD2_LEVEL_SECTION_COUNT * DD2_DISK_WORD_BYTES;
    if (bytes.data == NULL || bytes.size < header_bytes ||
        dd2_read_le32(bytes.data) != header_bytes) {
        return false;
    }
    dd2_level_data level = {0};
    for (size_t index = 0; index < DD2_LEVEL_SECTION_COUNT; ++index) {
        const size_t start = dd2_read_le32(bytes.data + (index * DD2_DISK_WORD_BYTES));
        const size_t end = index + 1 < DD2_LEVEL_SECTION_COUNT
                               ? dd2_read_le32(bytes.data + ((index + 1) * DD2_DISK_WORD_BYTES))
                               : bytes.size;
        if (start < header_bytes || start > end || end > bytes.size) {
            return false;
        }
        level.sections[index] = (dd2_byte_view){.data = bytes.data + start, .size = end - start};
    }
    const dd2_byte_view vertices = level.sections[DD2_LEVEL_ROAD_VERTICES];
    const dd2_byte_view definitions = level.sections[DD2_LEVEL_TEXTURE_DEFINITIONS];
    if (vertices.size % DD2_DISK_VERTEX_BYTES != 0 || definitions.size < DD2_DISK_WORD_BYTES ||
        (definitions.size - DD2_DISK_WORD_BYTES) % DD2_DISK_TEXTURE_DEFINITION_BYTES != 0) {
        return false;
    }
    level.vertex_count = vertices.size / DD2_DISK_VERTEX_BYTES;
    level.texture_definition_count = dd2_read_le32(definitions.data);
    if (level.texture_definition_count !=
        (definitions.size - DD2_DISK_WORD_BYTES) / DD2_DISK_TEXTURE_DEFINITION_BYTES) {
        return false;
    }
    *result = level;
    return true;
}

bool dd2_level_vertex(const dd2_level_data *level, size_t index, dd2_track_vertex *result) {
    if (result == NULL) {
        return false;
    }
    *result = (dd2_track_vertex){0};
    if (level == NULL || index >= level->vertex_count) {
        return false;
    }
    const dd2_byte_view section = level->sections[DD2_LEVEL_ROAD_VERTICES];
    if (section.data == NULL || index >= section.size / DD2_DISK_VERTEX_BYTES) {
        return false;
    }
    const uint8_t *bytes = section.data + (index * DD2_DISK_VERTEX_BYTES);
    *result = (dd2_track_vertex){.x = dd2_read_le_i32(bytes),
                                 .y = dd2_read_le_i32(bytes + DD2_DISK_WORD_BYTES),
                                 .z = dd2_read_le_i32(bytes + ((size_t)2 * DD2_DISK_WORD_BYTES))};
    return true;
}

bool dd2_level_texture_definition(const dd2_level_data *level, size_t index,
                                  dd2_texture_definition *result) {
    if (result == NULL) {
        return false;
    }
    *result = (dd2_texture_definition){0};
    if (level == NULL || index >= level->texture_definition_count) {
        return false;
    }
    const dd2_byte_view section = level->sections[DD2_LEVEL_TEXTURE_DEFINITIONS];
    if (section.data == NULL || section.size < DD2_DISK_WORD_BYTES ||
        index >= (section.size - DD2_DISK_WORD_BYTES) / DD2_DISK_TEXTURE_DEFINITION_BYTES) {
        return false;
    }
    const uint8_t *bytes =
        section.data + DD2_DISK_WORD_BYTES + (index * DD2_DISK_TEXTURE_DEFINITION_BYTES);
    result->page_flags = dd2_read_le16(bytes);
    result->reserved = dd2_read_le16(bytes + 2);
    for (size_t corner = 0; corner < DD2_TEXTURE_CORNERS; ++corner) {
        result->corners[corner] =
            (dd2_texture_uv){.u = bytes[DD2_DISK_WORD_BYTES + (corner * 2)],
                             .v = bytes[DD2_DISK_WORD_BYTES + (corner * 2) + 1]};
    }
    return true;
}
