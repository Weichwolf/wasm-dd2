#ifndef DD2_TEST_SURFACE_FIXTURE_H
#define DD2_TEST_SURFACE_FIXTURE_H

#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/level.h"

#include <stddef.h>
#include <stdint.h>

enum {
    DD2_SURFACE_TEST_LEVELS = 8,
    DD2_SURFACE_TEST_RECORD = 64,
    DD2_SURFACE_TEST_ROW_VERTICES = 3,
    DD2_SURFACE_TEST_VERTICES = 6,
    DD2_SURFACE_TEST_VERTEX_BYTES = 12,
    DD2_SURFACE_TEST_Z_OFFSET = 8,
    DD2_SURFACE_TEST_FIRST_VERTEX = 16,
    DD2_SURFACE_TEST_NEXT = 20,
    DD2_SURFACE_TEST_PREVIOUS = 24,
    DD2_SURFACE_TEST_SIDE = 1000,
    DD2_SURFACE_TEST_HEIGHT = 100,
    DD2_SURFACE_TEST_LONG_SIDE = 1000000
};

typedef struct {
    uint8_t bytes[DD2_TEST_WORD_BYTES + (DD2_SURFACE_TEST_LEVELS * DD2_SURFACE_TEST_RECORD)];
    uint8_t vertices[DD2_SURFACE_TEST_LEVELS * DD2_SURFACE_TEST_VERTICES *
                     DD2_SURFACE_TEST_VERTEX_BYTES];
    dd2_level_data level;
} dd2_surface_test_fixture;

static inline void dd2_surface_test_fixture_init(dd2_surface_test_fixture *fixture,
                                                 unsigned levels) {
    *fixture = (dd2_surface_test_fixture){0};
    fixture->level.vertex_count = (size_t)levels * DD2_SURFACE_TEST_VERTICES;
    fixture->level.sections[DD2_LEVEL_ROAD_VERTICES] =
        (dd2_byte_view){.data = fixture->vertices,
                        .size = fixture->level.vertex_count * DD2_SURFACE_TEST_VERTEX_BYTES};
    fixture->level.sections[DD2_LEVEL_ROAD_STRIPS] =
        (dd2_byte_view){.data = fixture->bytes,
                        .size = DD2_TEST_WORD_BYTES + ((size_t)levels * DD2_SURFACE_TEST_RECORD)};
    dd2_test_write_le32(fixture->bytes, levels);
    for (unsigned layer = 0; layer < levels; ++layer) {
        uint8_t *record =
            fixture->bytes + DD2_TEST_WORD_BYTES + ((size_t)layer * DD2_SURFACE_TEST_RECORD);
        record[0] = 1;
        record[1] = 2;
        dd2_test_write_le16(record + DD2_SURFACE_TEST_FIRST_VERTEX,
                            (uint16_t)(layer * DD2_SURFACE_TEST_VERTICES));
        dd2_test_write_le32(record + DD2_SURFACE_TEST_NEXT,
                            ((layer + 1) % levels) * DD2_SURFACE_TEST_RECORD);
        dd2_test_write_le32(record + DD2_SURFACE_TEST_PREVIOUS,
                            ((layer + levels - 1) % levels) * DD2_SURFACE_TEST_RECORD);
        for (unsigned corner = 0; corner < DD2_SURFACE_TEST_VERTICES; ++corner) {
            uint8_t *vertex =
                fixture->vertices + (((size_t)layer * DD2_SURFACE_TEST_VERTICES + corner) *
                                     DD2_SURFACE_TEST_VERTEX_BYTES);
            const int32_t xpos =
                ((int32_t)(corner % DD2_SURFACE_TEST_ROW_VERTICES) - 1) * DD2_SURFACE_TEST_SIDE;
            const int32_t zpos = corner < DD2_SURFACE_TEST_ROW_VERTICES ? -DD2_SURFACE_TEST_SIDE
                                                                        : DD2_SURFACE_TEST_SIDE;
            dd2_test_write_le32(vertex, (uint32_t)xpos);
            dd2_test_write_le32(vertex + DD2_TEST_WORD_BYTES, layer * DD2_SURFACE_TEST_HEIGHT);
            dd2_test_write_le32(vertex + DD2_SURFACE_TEST_Z_OFFSET, (uint32_t)zpos);
        }
    }
}

#endif
