#include "asset_fixture.h"
#include "assets/car_class.h"
#include "assets/model.h"
#include "assets/road.h"
#include "assets/track.h"
#include "assets/world.h"
#include "model_fixture.h"
#include "world_fixture.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_TRACK_TEST_SCENE_BYTES = DD2_WORLD_TEST_BYTES + (5 * 36),
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
                           "wheel-primary", "wheel-secondary", "sky"};
    dd2_test_write_le32(scene + DD2_WORLD_TEST_TEMPLATE_COUNT, DD2_TRACK_MODEL_COUNT);
    for (size_t index = 0; index < DD2_TRACK_MODEL_COUNT; ++index) {
        uint8_t *item = scene + DD2_WORLD_TEST_TEMPLATE + (index * DD2_TRACK_TEST_TEMPLATE_BYTES);
        for (size_t byte = 0; byte < DD2_TRACK_TEST_TEMPLATE_BYTES; ++byte) {
            item[byte] = 0;
        }
        dd2_model_test_name(item, names[index]);
        dd2_test_write_le32(item + DD2_WORLD_TEST_NAME,
                            (uint32_t)(index % DD2_WORLD_TEST_RESOURCES));
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

static bool dd2_track_test_owner(void) {
    uint8_t scene_storage[DD2_TRACK_TEST_SCENE_BYTES + 1] = {0};
    uint8_t road_storage[DD2_TRACK_TEST_ROAD_BYTES + 1] = {0};
    uint8_t *scene = scene_storage + 1;
    uint8_t *road = road_storage + 1;
    dd2_track_test_source((dd2_track_test_buffers){.scene = scene, .road = road});
    dd2_world_test_loader_state state = {0};
    dd2_track *track = dd2_track_create_prepared(
        DD2_TRACK_TEST_ARENA,
        (dd2_track_prepared_source){.scene = {scene, DD2_TRACK_TEST_SCENE_BYTES},
                                    .road = {road, DD2_TRACK_TEST_ROAD_BYTES}},
        dd2_world_test_loader, &state);
    for (size_t byte = 0; byte < sizeof(scene_storage); ++byte) {
        scene_storage[byte] = 0;
    }
    for (size_t byte = 0; byte < sizeof(road_storage); ++byte) {
        road_storage[byte] = 0;
    }
    bool passed = track != NULL && dd2_track_number(track) == DD2_TRACK_TEST_ARENA &&
                  dd2_world_resource_count(dd2_track_world(track)) == DD2_WORLD_TEST_RESOURCES &&
                  dd2_road_cell_count(dd2_track_road(track)) == 1 &&
                  state.calls == DD2_WORLD_TEST_RESOURCES && dd2_track_level(track) == NULL &&
                  dd2_track_textures(track) == NULL && dd2_track_scene(track) == NULL &&
                  dd2_track_car(track) == NULL && dd2_track_wheel(track, 0) == NULL &&
                  dd2_track_car_livery(track, 0, DD2_CAR_ROOKIE) == NULL;
    for (unsigned kind = 0; kind < DD2_TRACK_MODEL_COUNT; ++kind) {
        passed = passed && dd2_model_index_count(
                               dd2_track_prepared_model(track, (dd2_track_model_kind)kind)) == 3;
    }
    passed = passed && dd2_track_prepared_model(track, DD2_TRACK_MODEL_COUNT) == NULL &&
             dd2_track_prepared_model(track, (dd2_track_model_kind)-1) == NULL;
    dd2_track_destroy(track);
    return passed;
}

static bool dd2_track_test_rejections(void) {
    uint8_t scene[DD2_TRACK_TEST_SCENE_BYTES] = {0};
    uint8_t road[DD2_TRACK_TEST_ROAD_BYTES] = {0};
    dd2_track_test_source((dd2_track_test_buffers){.scene = scene, .road = road});
    dd2_track_prepared_source source = {.scene = {scene, sizeof(scene)},
                                        .road = {road, sizeof(road)}};
    const unsigned invalid[] = {0, DD2_TRACK_COUNT + 1, 1};
    for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        dd2_world_test_loader_state state = {0};
        dd2_track *track =
            dd2_track_create_prepared(invalid[index], source, dd2_world_test_loader, &state);
        const bool rejected = track == NULL && state.calls == 0;
        dd2_track_destroy(track);
        if (!rejected) {
            return false;
        }
    }
    for (size_t missing = 0; missing < DD2_TRACK_MODEL_COUNT; ++missing) {
        dd2_track_test_source((dd2_track_test_buffers){.scene = scene, .road = road});
        scene[DD2_WORLD_TEST_TEMPLATE + (missing * DD2_TRACK_TEST_TEMPLATE_BYTES)] = 'x';
        dd2_world_test_loader_state state = {0};
        dd2_track *track =
            dd2_track_create_prepared(DD2_TRACK_TEST_ARENA, source, dd2_world_test_loader, &state);
        const bool rejected = track == NULL && state.calls == DD2_WORLD_TEST_RESOURCES;
        dd2_track_destroy(track);
        if (!rejected) {
            return false;
        }
    }
    dd2_track_test_source((dd2_track_test_buffers){.scene = scene, .road = road});
    for (size_t fail = 1; fail <= DD2_WORLD_TEST_RESOURCES; ++fail) {
        dd2_world_test_loader_state state = {.fail_at = fail};
        dd2_track *track =
            dd2_track_create_prepared(DD2_TRACK_TEST_ARENA, source, dd2_world_test_loader, &state);
        const bool rejected = track == NULL && state.calls == fail;
        dd2_track_destroy(track);
        if (!rejected) {
            return false;
        }
    }
    return true;
}

static bool dd2_track_test_providers(void) {
    const dd2_track_provider provider = {.load = dd2_track_test_load};
    dd2_track *track = dd2_track_load(provider, DD2_TRACK_TEST_ARENA);
    bool passed = track != NULL;
    dd2_track_destroy(track);
    track = dd2_track_load(provider, DD2_TRACK_TEST_OTHER_ARENA);
    passed = passed && track == NULL;
    dd2_track_destroy(track);
    const unsigned invalid[] = {0, DD2_TRACK_COUNT + 1};
    for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        track = dd2_track_load(provider, invalid[index]);
        passed = passed && track == NULL;
        dd2_track_destroy(track);
    }
    return passed && dd2_track_load((dd2_track_provider){0}, DD2_TRACK_TEST_ARENA) == NULL &&
           dd2_track_load(dd2_track_reference_provider(NULL), DD2_TRACK_TEST_ARENA) == NULL &&
           dd2_track_number(NULL) == 0 && dd2_track_world(NULL) == NULL &&
           dd2_track_prepared_model(NULL, DD2_TRACK_MODEL_CLOSE) == NULL;
}

int main(void) {
    if (!dd2_track_test_owner() || !dd2_track_test_rejections() || !dd2_track_test_providers()) {
        return EXIT_FAILURE;
    }
    puts("Prepared track ownership, required models, layout and provider rollback: PASS");
    return EXIT_SUCCESS;
}
