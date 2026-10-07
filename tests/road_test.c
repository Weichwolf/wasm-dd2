#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "physics/road_contact.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_TEST_ROAD_HEADER = 36,
    DD2_TEST_ROAD_LANE = 14,
    DD2_TEST_ROAD_RECORD = 64,
    DD2_TEST_ROAD_BYTES = 212,
    DD2_TEST_ROAD_VERTICES = 8,
    DD2_TEST_ROAD_VERTEX_BYTES = 12,
    DD2_TEST_ROAD_Z_OFFSET = 8,
    DD2_TEST_ROAD_FIRST_VERTEX = 16,
    DD2_TEST_ROAD_NEXT = 20,
    DD2_TEST_ROAD_PREVIOUS = 24,
    DD2_TEST_ROAD_BRANCH = 28,
    DD2_TEST_ROAD_GAP_OFFSET = 144,
    DD2_TEST_ROAD_SPLIT = 8,
    DD2_TEST_ROAD_MERGE = 9,
    DD2_TEST_ROAD_KIND_LIMIT = 12,
    DD2_TEST_ROAD_BAD_LANES = 128,
    DD2_TEST_ROAD_SCALE = 10,
    DD2_TEST_ARENA_SIDE = 32,
    DD2_TEST_ARENA_VERTICES = DD2_TEST_ARENA_SIDE * DD2_TEST_ARENA_SIDE,
    DD2_TEST_ARENA_CELLS = 31 * 31
};
static const double dd2_test_contact_tolerance = 1e-10;

typedef struct {
    uint8_t bytes[DD2_TEST_ROAD_BYTES];
    uint8_t vertices[DD2_TEST_ROAD_VERTICES * DD2_TEST_ROAD_VERTEX_BYTES];
    dd2_level_data level;
} dd2_test_road_fixture;

static void dd2_test_road_fixture_init(dd2_test_road_fixture *fixture) {
    *fixture = (dd2_test_road_fixture){0};
    fixture->level.vertex_count = DD2_TEST_ROAD_VERTICES;
    fixture->level.sections[DD2_LEVEL_ROAD_VERTICES] =
        (dd2_byte_view){.data = fixture->vertices, .size = sizeof(fixture->vertices)};
    fixture->level.sections[DD2_LEVEL_ROAD_STRIPS] =
        (dd2_byte_view){.data = fixture->bytes, .size = DD2_TEST_WORD_BYTES + DD2_TEST_ROAD_RECORD};
    dd2_test_write_le32(fixture->bytes, 1);
    fixture->bytes[DD2_TEST_WORD_BYTES] = 1;
    fixture->bytes[DD2_TEST_WORD_BYTES + 1] = 2;
    for (size_t vertex = 0; vertex < DD2_TEST_ROAD_VERTICES; ++vertex) {
        uint8_t *bytes = fixture->vertices + (vertex * DD2_TEST_ROAD_VERTEX_BYTES);
        const uint32_t xpos = (uint32_t)(vertex % 3) * DD2_TEST_ROAD_SCALE;
        const uint32_t zpos = (uint32_t)(vertex / 3) * DD2_TEST_ROAD_SCALE;
        dd2_test_write_le32(bytes, xpos);
        dd2_test_write_le32(bytes + DD2_TEST_WORD_BYTES, xpos + (2 * zpos));
        dd2_test_write_le32(bytes + DD2_TEST_ROAD_Z_OFFSET, zpos);
    }
}

static bool dd2_test_close(double actual, double expected) {
    return fabs(actual - expected) < dd2_test_contact_tolerance;
}

static bool dd2_test_road_contacts(void) {
    dd2_test_road_fixture fixture;
    dd2_test_road_fixture_init(&fixture);
    dd2_road *road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    dd2_road_contact contact = {0};
    bool valid = road != NULL && dd2_road_main_count(road) == 1 && dd2_road_cell_count(road) == 2 &&
                 dd2_road_contact_cell(road, 0, (dd2_road_point){.x = 2, .z = 3}, &contact) &&
                 dd2_test_close(contact.height, 2 + (2 * 3)) && contact.triangle == 0;
    const double magnitude = sqrt(1 + 1 + (2 * 2));
    valid =
        valid && dd2_test_close(contact.normal[0], -1 / magnitude) &&
        dd2_test_close(contact.normal[1], 1 / magnitude) &&
        dd2_test_close(contact.normal[2], -2 / magnitude) &&
        dd2_road_contact_cell(
            road, 0, (dd2_road_point){.x = DD2_TEST_ROAD_SCALE - 2, .z = DD2_TEST_ROAD_SCALE - 2},
            &contact) &&
        dd2_test_close(contact.height, (DD2_TEST_ROAD_SCALE - 2) * 3) && contact.triangle == 1;
    valid = valid && dd2_road_contact_cell(road, 0, (dd2_road_point){0}, &contact) &&
            dd2_test_close(contact.height, 0) &&
            !dd2_road_contact_cell(road, 0, (dd2_road_point){.x = -1}, &contact) &&
            contact.height == 0 &&
            !dd2_road_contact_cell(road, DD2_TEST_ROAD_BAD_LANES, (dd2_road_point){0}, &contact) &&
            !dd2_road_contact_cell(road, 0, (dd2_road_point){0}, NULL);
    /* The owned road must survive releasing/overwriting its source storage. */
    for (size_t index = 0; index < sizeof(fixture.vertices); ++index) {
        fixture.vertices[index] = 0;
    }
    valid = valid && dd2_road_contact_cell(road, 0, (dd2_road_point){.x = 2, .z = 3}, &contact) &&
            dd2_test_close(contact.height, 2 + (2 * 3));
    dd2_road_destroy(road);
    road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    valid = valid && road != NULL && !dd2_road_contact_cell(road, 0, (dd2_road_point){0}, &contact);
    dd2_road_destroy(road);
    return valid;
}

static bool dd2_test_road_shapes(void) {
    static const uint8_t first_masks[DD2_TEST_ROAD_KIND_LIMIT] = {0, 3, 2, 3, 2, 1,
                                                                  3, 1, 3, 3, 3, 3};
    static const uint8_t last_masks[DD2_TEST_ROAD_KIND_LIMIT] = {0, 3, 3, 2, 2, 3,
                                                                 1, 1, 3, 3, 3, 3};
    for (unsigned kind = 1; kind < DD2_TEST_ROAD_KIND_LIMIT; ++kind) {
        dd2_test_road_fixture fixture;
        dd2_test_road_fixture_init(&fixture);
        fixture.bytes[DD2_TEST_WORD_BYTES] = (uint8_t)kind;
        dd2_test_write_le16(fixture.bytes + DD2_TEST_WORD_BYTES + DD2_TEST_ROAD_FIRST_VERTEX, 1);
        dd2_road *road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
        const dd2_road_cell *cells = dd2_road_cells(road);
        const bool valid = road != NULL && cells[0].triangle_mask == first_masks[kind] &&
                           cells[1].triangle_mask == last_masks[kind];
        dd2_road_destroy(road);
        if (!valid) {
            return false;
        }
    }
    return true;
}

static bool dd2_test_road_extremes(void) {
    dd2_test_road_fixture fixture;
    dd2_test_road_fixture_init(&fixture);
    const int32_t coordinates[DD2_TEST_ROAD_VERTICES][3] = {{INT32_MIN, INT32_MIN, INT32_MIN},
                                                            {INT32_MAX, INT32_MIN, INT32_MIN},
                                                            {0, 0, 0},
                                                            {INT32_MIN, INT32_MAX, INT32_MAX},
                                                            {INT32_MAX, INT32_MAX, INT32_MAX},
                                                            {0, 0, 0},
                                                            {0, 0, 0},
                                                            {0, 0, 0}};
    for (size_t index = 0; index < DD2_TEST_ROAD_VERTICES; ++index) {
        for (size_t axis = 0; axis < 3; ++axis) {
            dd2_test_write_le32(fixture.vertices + (index * DD2_TEST_ROAD_VERTEX_BYTES) +
                                    (axis * DD2_TEST_WORD_BYTES),
                                (uint32_t)coordinates[index][axis]);
        }
    }
    dd2_road *road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    dd2_road_contact contact = {0};
    const bool valid =
        road != NULL && dd2_road_contact_cell(road, 0, (dd2_road_point){0}, &contact) &&
        dd2_test_close(contact.height, 0) && dd2_test_close(contact.normal[1], 1 / sqrt(2)) &&
        !dd2_road_contact_cell(road, 0, (dd2_road_point){.x = (double)INT32_MIN - 1}, &contact);
    dd2_road_destroy(road);
    return valid;
}

static void dd2_test_road_graph_fixture(dd2_test_road_fixture *fixture) {
    dd2_test_road_fixture_init(fixture);
    fixture->level.sections[DD2_LEVEL_ROAD_STRIPS].size = sizeof(fixture->bytes);
    dd2_test_write_le32(fixture->bytes, 3);
    uint8_t *split = fixture->bytes + DD2_TEST_WORD_BYTES;
    uint8_t *merge = split + DD2_TEST_ROAD_RECORD;
    uint8_t *branch = split + DD2_TEST_ROAD_GAP_OFFSET;
    split[0] = DD2_TEST_ROAD_SPLIT;
    merge[0] = DD2_TEST_ROAD_MERGE;
    branch[0] = 1;
    merge[1] = 2;
    branch[1] = 2;
    dd2_test_write_le32(split + DD2_TEST_ROAD_NEXT, DD2_TEST_ROAD_RECORD);
    dd2_test_write_le32(split + DD2_TEST_ROAD_PREVIOUS, DD2_TEST_ROAD_RECORD);
    dd2_test_write_le32(split + DD2_TEST_ROAD_BRANCH, DD2_TEST_ROAD_GAP_OFFSET);
    dd2_test_write_le32(merge + DD2_TEST_ROAD_BRANCH, DD2_TEST_ROAD_GAP_OFFSET);
    dd2_test_write_le32(branch + DD2_TEST_ROAD_NEXT, DD2_TEST_ROAD_RECORD);
}

static bool dd2_test_road_graph(void) {
    dd2_test_road_fixture fixture;
    dd2_test_road_graph_fixture(&fixture);
    dd2_road *road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    const dd2_road_strip *strips = dd2_road_strips(road);
    bool valid = road != NULL && dd2_road_strip_count(road) == 3 &&
                 dd2_road_main_count(road) == 2 && strips[0].next == 1 && strips[0].branch == 2 &&
                 strips[1].branch == 2 && strips[2].next == 1 &&
                 strips[2].main_order == DD2_ROAD_NO_STRIP;
    dd2_road_destroy(road);
    dd2_test_write_le32(fixture.bytes + DD2_TEST_WORD_BYTES + DD2_TEST_ROAD_RECORD +
                            DD2_TEST_ROAD_NEXT,
                        DD2_TEST_ROAD_RECORD);
    road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    valid = valid && road == NULL;
    dd2_road_destroy(road);
    return valid;
}

static bool dd2_test_road_rejections(void) {
    static const size_t offsets[] = {0,
                                     0,
                                     DD2_TEST_WORD_BYTES,
                                     DD2_TEST_WORD_BYTES + 1,
                                     DD2_TEST_WORD_BYTES + DD2_TEST_ROAD_NEXT,
                                     DD2_TEST_WORD_BYTES + DD2_TEST_ROAD_NEXT};
    static const uint32_t values[] = {
        0, UINT32_MAX, DD2_TEST_ROAD_KIND_LIMIT, DD2_TEST_ROAD_BAD_LANES, 2, DD2_TEST_WORD_BYTES};
    for (size_t index = 0; index < sizeof(offsets) / sizeof(offsets[0]); ++index) {
        dd2_test_road_fixture fixture;
        dd2_test_road_fixture_init(&fixture);
        dd2_test_write_le32(fixture.bytes + offsets[index], values[index]);
        dd2_road *road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
        const bool rejected = road == NULL;
        dd2_road_destroy(road);
        if (!rejected) {
            return false;
        }
    }
    dd2_test_road_fixture fixture;
    dd2_test_road_fixture_init(&fixture);
    fixture.level.sections[DD2_LEVEL_ROAD_STRIPS].size -= DD2_TEST_WORD_BYTES;
    dd2_road *road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    const bool rejected = road == NULL;
    dd2_road_destroy(road);
    dd2_road_destroy(NULL);
    return rejected && dd2_road_create(NULL, DD2_ROAD_RACING) == NULL &&
           dd2_road_cell_count(NULL) == 0 && dd2_road_strip_count(NULL) == 0 &&
           dd2_road_vertices(NULL) == NULL;
}

static bool dd2_test_arena(void) {
    uint8_t *vertices = calloc(DD2_TEST_ARENA_VERTICES, DD2_TEST_ROAD_VERTEX_BYTES);
    uint8_t *source = calloc(DD2_TEST_ARENA_VERTICES, DD2_TEST_ROAD_LANE);
    if (vertices == NULL || source == NULL) {
        free(vertices);
        free(source);
        return false;
    }
    for (size_t index = 0; index < DD2_TEST_ARENA_VERTICES; ++index) {
        uint8_t *vertex = vertices + (index * DD2_TEST_ROAD_VERTEX_BYTES);
        dd2_test_write_le32(vertex, (uint32_t)(index % DD2_TEST_ARENA_SIDE));
        dd2_test_write_le32(vertex + DD2_TEST_ROAD_Z_OFFSET,
                            (uint32_t)(index / DD2_TEST_ARENA_SIDE));
    }
    dd2_level_data level = {.vertex_count = DD2_TEST_ARENA_VERTICES};
    level.sections[DD2_LEVEL_ROAD_VERTICES] = (dd2_byte_view){
        .data = vertices, .size = (size_t)DD2_TEST_ARENA_VERTICES * DD2_TEST_ROAD_VERTEX_BYTES};
    level.sections[DD2_LEVEL_ROAD_STRIPS] = (dd2_byte_view){
        .data = source, .size = (size_t)DD2_TEST_ARENA_VERTICES * DD2_TEST_ROAD_LANE};
    dd2_road *road = dd2_road_create(&level, DD2_ROAD_ARENA);
    dd2_road_contact contact = {0};
    bool valid = road != NULL && dd2_road_cell_count(road) == DD2_TEST_ARENA_CELLS &&
                 dd2_road_strip_count(road) == 0 &&
                 dd2_road_contact_cell(
                     road, DD2_TEST_ARENA_CELLS - 1,
                     (dd2_road_point){.x = DD2_TEST_ARENA_SIDE - 1, .z = DD2_TEST_ARENA_SIDE - 1},
                     &contact) &&
                 contact.height == 0 && dd2_test_close(contact.normal[1], 1);
    dd2_road_destroy(road);
    level.sections[DD2_LEVEL_ROAD_STRIPS].size -= 1;
    road = dd2_road_create(&level, DD2_ROAD_ARENA);
    valid = valid && road == NULL;
    dd2_road_destroy(road);
    free(vertices);
    free(source);
    return valid;
}

int main(void) {
    if (!dd2_test_road_extremes()) {
        puts("road contact coordinate extremes failed");
        return EXIT_FAILURE;
    }
    if (!dd2_test_road_contacts() || !dd2_test_road_shapes() || !dd2_test_road_graph() ||
        !dd2_test_road_rejections() || !dd2_test_arena()) {
        puts("road topology/contact check failed");
        return EXIT_FAILURE;
    }
    puts("road topology/contact: PASS");
    return EXIT_SUCCESS;
}
