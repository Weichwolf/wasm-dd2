#ifndef DD2_TEST_TRACK_FIXTURE_H
#define DD2_TEST_TRACK_FIXTURE_H

#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/car.h"
#include "assets/road.h"
#include "assets/track.h"
#include "model_fixture.h"
#include "world_fixture.h"

#include <stddef.h>
#include <stdint.h>

enum {
    DD2_TRACK_TEST_TEMPLATES =
        DD2_TRACK_MODEL_COUNT + (DD2_CAR_LIVERIES * DD2_TRACK_BODY_LODS) + DD2_TRACK_SKY_PATCHES,
    DD2_TRACK_TEST_SCENE_BYTES = DD2_WORLD_TEST_BYTES + ((DD2_TRACK_TEST_TEMPLATES - 1) * 36),
    DD2_TRACK_TEST_DECIMAL_RADIX = 10,
    DD2_TRACK_TEST_TEMPLATE_BYTES = 36,
    DD2_TRACK_TEST_ROAD_BYTES = 108,
    DD2_TRACK_TEST_ARENA = 8,
    DD2_TRACK_TEST_OTHER_ARENA = 9,
    DD2_TRACK_TEST_LAYOUT = 8,
    DD2_TRACK_TEST_UNITS = 12,
    DD2_TRACK_TEST_VERTICES = 16,
    DD2_TRACK_TEST_CELLS = 28,
    DD2_TRACK_TEST_FIRST_VERTEX = 32,
    DD2_TRACK_TEST_VERTEX_BYTES = 12,
    DD2_TRACK_TEST_FIRST_CELL = 80,
    DD2_TRACK_TEST_CELL_OWNER = 96,
    DD2_TRACK_TEST_CELL_MASK = 106,
    DD2_TRACK_TEST_UNITS_PER_METER = 160
};

typedef struct {
    uint8_t *scene;
    uint8_t *road;
} dd2_track_test_buffers;

static void dd2_track_test_source(dd2_track_test_buffers buffers) {
    uint8_t *scene = buffers.scene;
    uint8_t *road = buffers.road;
    dd2_world_test_source(scene);
    const char *names[] = {"car-close",     "car-medium",      "car-distant",
                           "wheel-primary", "wheel-secondary", "detached-trunk"};
    dd2_test_write_le32(scene + DD2_WORLD_TEST_TEMPLATE_COUNT, DD2_TRACK_TEST_TEMPLATES);
    for (size_t index = 0; index < DD2_TRACK_MODEL_COUNT; ++index) {
        uint8_t *item = scene + DD2_WORLD_TEST_TEMPLATE + (index * DD2_TRACK_TEST_TEMPLATE_BYTES);
        for (size_t byte = 0; byte < DD2_TRACK_TEST_TEMPLATE_BYTES; ++byte) {
            item[byte] = 0;
        }
        dd2_model_test_name(item, names[index]);
        dd2_test_write_le32(item + DD2_WORLD_TEST_NAME,
                            (uint32_t)(index % DD2_WORLD_TEST_RESOURCES));
    }
    for (unsigned livery = 0; livery < DD2_CAR_LIVERIES; ++livery) {
        for (unsigned detail = 0; detail < DD2_TRACK_BODY_LODS; ++detail) {
            char name[DD2_WORLD_TEST_NAME] = {0};
            size_t cursor = 0;
            while (names[detail][cursor] != '\0') {
                name[cursor] = names[detail][cursor];
                ++cursor;
            }
            name[cursor++] = '-';
            name[cursor++] = (char)('0' + (livery / DD2_TRACK_TEST_DECIMAL_RADIX));
            name[cursor] = (char)('0' + (livery % DD2_TRACK_TEST_DECIMAL_RADIX));
            const size_t index = DD2_TRACK_MODEL_COUNT + (livery * DD2_TRACK_BODY_LODS) + detail;
            uint8_t *item =
                scene + DD2_WORLD_TEST_TEMPLATE + (index * DD2_TRACK_TEST_TEMPLATE_BYTES);
            dd2_model_test_name(item, name);
            dd2_test_write_le32(item + DD2_WORLD_TEST_NAME, detail);
        }
    }
    const char *sky_names[DD2_TRACK_SKY_PATCHES] = {"sky-lower-0", "sky-lower-1", "sky-lower-2",
                                                    "sky-lower-3", "sky-upper-0", "sky-upper-1",
                                                    "sky-upper-2", "sky-upper-3"};
    for (unsigned patch = 0; patch < DD2_TRACK_SKY_PATCHES; ++patch) {
        const size_t index = DD2_TRACK_TEST_TEMPLATES - DD2_TRACK_SKY_PATCHES + patch;
        uint8_t *item = scene + DD2_WORLD_TEST_TEMPLATE + (index * DD2_TRACK_TEST_TEMPLATE_BYTES);
        dd2_model_test_name(item, sky_names[patch]);
    }
    dd2_model_test_name(road, "DD2ROAD1");
    dd2_test_write_le32(road + DD2_TRACK_TEST_LAYOUT, DD2_ROAD_ARENA);
    dd2_test_write_le32(road + DD2_TRACK_TEST_UNITS, DD2_TRACK_TEST_UNITS_PER_METER);
    dd2_test_write_le32(road + DD2_TRACK_TEST_VERTICES, DD2_ROAD_CORNERS);
    dd2_test_write_le32(road + DD2_TRACK_TEST_CELLS, 1);
    const uint32_t positions[DD2_ROAD_CORNERS][3] = {
        {0, 0, 0}, {160, 0, 0}, {160, 0, 160}, {0, 0, 160}};
    for (size_t vertex = 0; vertex < DD2_ROAD_CORNERS; ++vertex) {
        for (size_t axis = 0; axis < 3; ++axis) {
            dd2_test_write_le32(road + DD2_TRACK_TEST_FIRST_VERTEX +
                                    (vertex * DD2_TRACK_TEST_VERTEX_BYTES) +
                                    (axis * DD2_WORLD_TEST_WORD),
                                positions[vertex][axis]);
        }
        dd2_test_write_le32(road + DD2_TRACK_TEST_FIRST_CELL + (vertex * DD2_WORLD_TEST_WORD),
                            (uint32_t)vertex);
    }
    dd2_test_write_le32(road + DD2_TRACK_TEST_CELL_OWNER, DD2_ROAD_NO_STRIP);
    road[DD2_TRACK_TEST_CELL_MASK] = 3;
}

static dd2_track *dd2_track_test_load(const void *user, unsigned number) {
    (void)user;
    (void)number;
    uint8_t scene[DD2_TRACK_TEST_SCENE_BYTES] = {0};
    uint8_t road[DD2_TRACK_TEST_ROAD_BYTES] = {0};
    dd2_track_test_source((dd2_track_test_buffers){.scene = scene, .road = road});
    dd2_world_test_loader_state state = {0};
    return dd2_track_create_prepared(
        DD2_TRACK_TEST_ARENA,
        (dd2_track_prepared_source){.scene = {scene, sizeof(scene)}, .road = {road, sizeof(road)}},
        dd2_world_test_loader, &state);
}

#endif
