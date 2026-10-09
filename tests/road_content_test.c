#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/road.h"
#include "physics/road_contact.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_ROAD_TEST_HEADER = 32,
    DD2_ROAD_TEST_VERTEX_BYTES = 12,
    DD2_ROAD_TEST_RECORD = 28,
    DD2_ROAD_TEST_VERTICES = 4,
    DD2_ROAD_TEST_STRIPS = 3,
    DD2_ROAD_TEST_CELLS = 3,
    DD2_ROAD_TEST_FIRST_STRIP = 80,
    DD2_ROAD_TEST_FIRST_CELL = 164,
    DD2_ROAD_TEST_BYTES = 248,
    DD2_ROAD_TEST_WORD = 4,
    DD2_ROAD_TEST_FIRST = 12,
    DD2_ROAD_TEST_MAIN = 16,
    DD2_ROAD_TEST_KIND = 22,
    DD2_ROAD_TEST_LANES = 23,
    DD2_ROAD_TEST_PADDING = 25,
    DD2_ROAD_TEST_CELL_STRIP = 16,
    DD2_ROAD_TEST_CELL_LANE = 20,
    DD2_ROAD_TEST_MASK = 26,
    DD2_ROAD_TEST_VERTEX_COUNT = 16,
    DD2_ROAD_TEST_STRIP_COUNT = 20,
    DD2_ROAD_TEST_MAIN_COUNT = 24,
    DD2_ROAD_TEST_CELL_COUNT = 28,
    DD2_ROAD_TEST_UNITS = 160,
    DD2_ROAD_TEST_UNIT_OFFSET = 12
};
static const double dd2_road_test_tolerance = 1e-12;
static const double dd2_road_test_height = 8;

static void dd2_road_test_content(uint8_t *bytes) {
    const char magic[] = "DD2ROAD1";
    for (size_t index = 0; index < DD2_ROAD_TEST_BYTES; ++index) {
        bytes[index] = 0;
    }
    for (size_t index = 0; index < sizeof(magic) - 1; ++index) {
        bytes[index] = (uint8_t)magic[index];
    }
    dd2_test_write_le32(bytes + DD2_ROAD_TEST_UNIT_OFFSET, DD2_ROAD_TEST_UNITS);
    dd2_test_write_le32(bytes + DD2_ROAD_TEST_VERTEX_COUNT, DD2_ROAD_TEST_VERTICES);
    dd2_test_write_le32(bytes + DD2_ROAD_TEST_STRIP_COUNT, DD2_ROAD_TEST_STRIPS);
    dd2_test_write_le32(bytes + DD2_ROAD_TEST_MAIN_COUNT, DD2_ROAD_TEST_STRIPS);
    dd2_test_write_le32(bytes + DD2_ROAD_TEST_CELL_COUNT, DD2_ROAD_TEST_CELLS);
    const uint32_t vertices[DD2_ROAD_TEST_VERTICES][3] = {
        {0, 0, 0}, {10, 10, 0}, {10, 30, 10}, {0, 20, 10}};
    for (size_t index = 0; index < DD2_ROAD_TEST_VERTICES; ++index) {
        for (size_t axis = 0; axis < 3; ++axis) {
            dd2_test_write_le32(bytes + DD2_ROAD_TEST_HEADER +
                                    (index * DD2_ROAD_TEST_VERTEX_BYTES) +
                                    (axis * DD2_ROAD_TEST_WORD),
                                vertices[index][axis]);
        }
    }
    for (uint32_t index = 0; index < DD2_ROAD_TEST_STRIPS; ++index) {
        uint8_t *strip = bytes + DD2_ROAD_TEST_FIRST_STRIP + ((size_t)index * DD2_ROAD_TEST_RECORD);
        dd2_test_write_le32(strip, (index + 1) % DD2_ROAD_TEST_STRIPS);
        dd2_test_write_le32(strip + DD2_ROAD_TEST_WORD,
                            (index + DD2_ROAD_TEST_STRIPS - 1) % DD2_ROAD_TEST_STRIPS);
        dd2_test_write_le32(strip + ((size_t)2 * DD2_ROAD_TEST_WORD), DD2_ROAD_NO_STRIP);
        dd2_test_write_le32(strip + DD2_ROAD_TEST_FIRST, index);
        dd2_test_write_le32(strip + DD2_ROAD_TEST_MAIN, index);
        strip[DD2_ROAD_TEST_KIND] = 1;
        strip[DD2_ROAD_TEST_LANES] = 1;
        uint8_t *cell = bytes + DD2_ROAD_TEST_FIRST_CELL + ((size_t)index * DD2_ROAD_TEST_RECORD);
        for (uint32_t corner = 0; corner < DD2_ROAD_CORNERS; ++corner) {
            dd2_test_write_le32(cell + ((size_t)corner * DD2_ROAD_TEST_WORD), corner);
        }
        dd2_test_write_le32(cell + DD2_ROAD_TEST_CELL_STRIP, index);
        cell[DD2_ROAD_TEST_MASK] = 3;
    }
}

static bool dd2_road_test_owner(void) {
    // Input is deliberately unaligned and overwritten after ownership transfer.
    uint8_t storage[DD2_ROAD_TEST_BYTES + 1] = {0};
    uint8_t *bytes = storage + 1;
    dd2_road_test_content(bytes);
    dd2_road *road = dd2_road_create_prepared((dd2_byte_view){bytes, DD2_ROAD_TEST_BYTES});
    for (size_t index = 0; index < DD2_ROAD_TEST_BYTES; ++index) {
        bytes[index] = 0;
    }
    dd2_road_contact contact = {0};
    bool passed = road != NULL && dd2_road_vertex_count(road) == DD2_ROAD_TEST_VERTICES &&
                  dd2_road_strip_count(road) == DD2_ROAD_TEST_STRIPS &&
                  dd2_road_main_count(road) == DD2_ROAD_TEST_STRIPS &&
                  dd2_road_cell_count(road) == DD2_ROAD_TEST_CELLS &&
                  dd2_road_contact_cell(road, 1, (dd2_road_point){.x = 2, .z = 3}, &contact) &&
                  fabs(contact.height - dd2_road_test_height) < dd2_road_test_tolerance;
    for (size_t index = 0; passed && index < dd2_road_strip_count(road); ++index) {
        const dd2_road_strip *strip = &dd2_road_strips(road)[index];
        passed = strip->first_cell == index && strip->source_offset == 0 &&
                 strip->source_number == 0 && strip->first_vertex == 0 &&
                 strip->source_lane_start == 0;
    }
    dd2_road_destroy(road);
    return passed;
}

typedef struct {
    size_t offset;
    uint32_t value;
} dd2_road_test_change;

static bool dd2_road_test_rejections(void) {
    uint8_t bytes[DD2_ROAD_TEST_BYTES + 1] = {0};
    dd2_road_test_content(bytes);
    for (size_t length = 0; length < DD2_ROAD_TEST_BYTES; ++length) {
        dd2_road *road = dd2_road_create_prepared((dd2_byte_view){bytes, length});
        const bool rejected = road == NULL;
        dd2_road_destroy(road);
        if (!rejected) {
            return false;
        }
    }
    const dd2_road_test_change changes[] = {
        {0, 0},
        {DD2_ROAD_TEST_UNIT_OFFSET, 1},
        {DD2_ROAD_TEST_VERTEX_COUNT, UINT32_MAX},
        {DD2_ROAD_TEST_STRIP_COUNT, UINT32_MAX},
        {DD2_ROAD_TEST_MAIN_COUNT, 1},
        {DD2_ROAD_TEST_CELL_COUNT, UINT32_MAX},
        {DD2_ROAD_TEST_FIRST_STRIP, DD2_ROAD_TEST_STRIPS},
        {DD2_ROAD_TEST_FIRST_STRIP + DD2_ROAD_TEST_WORD, DD2_ROAD_TEST_STRIPS},
        {DD2_ROAD_TEST_FIRST_STRIP + (2 * DD2_ROAD_TEST_WORD), 0},
        {DD2_ROAD_TEST_FIRST_STRIP + DD2_ROAD_TEST_FIRST, 1},
        {DD2_ROAD_TEST_FIRST_STRIP + DD2_ROAD_TEST_MAIN, 1},
        {DD2_ROAD_TEST_FIRST_CELL, DD2_ROAD_TEST_VERTICES},
        {DD2_ROAD_TEST_FIRST_CELL + DD2_ROAD_TEST_CELL_STRIP, DD2_ROAD_TEST_STRIPS},
        {DD2_ROAD_TEST_FIRST_CELL + DD2_ROAD_TEST_CELL_LANE, 1},
        {DD2_ROAD_TEST_HEADER, INT32_MAX}};
    for (size_t index = 0; index < sizeof(changes) / sizeof(changes[0]); ++index) {
        dd2_road_test_content(bytes);
        dd2_test_write_le32(bytes + changes[index].offset, changes[index].value);
        dd2_road *road = dd2_road_create_prepared((dd2_byte_view){bytes, DD2_ROAD_TEST_BYTES});
        const bool rejected = road == NULL;
        dd2_road_destroy(road);
        if (!rejected) {
            return false;
        }
    }
    return true;
}

int main(void) {
    if (dd2_road_create_prepared((dd2_byte_view){0}) != NULL || !dd2_road_test_owner() ||
        !dd2_road_test_rejections()) {
        return EXIT_FAILURE;
    }
    puts("{\"scope\":\"owned road ownership, topology, bounds and rejection\",\"pass\":true}");
    return EXIT_SUCCESS;
}
