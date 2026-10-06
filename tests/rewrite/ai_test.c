#include "ai/driver.h"
#include "ai/path.h"
#include "asset_fixture.h"
#include "assets/road.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "surface_fixture.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { DD2_AI_TEST_STALL_STEPS = 350 };
static const double dd2_ai_test_tolerance = 1e-8;
static const double dd2_ai_test_height = 190;
static const double dd2_ai_test_lane = 500;
static const double dd2_ai_test_distance = 3000;
static const double dd2_ai_test_wrap = 9000;
static const double dd2_ai_test_curvature = 0.0007853981633974483096;

static dd2_road *dd2_ai_test_road(void) {
    dd2_surface_test_fixture fixture;
    dd2_surface_test_fixture_init(&fixture, 4);
    static const int32_t centers[4][2] = {{0, 0}, {0, 2000}, {2000, 2000}, {2000, 0}};
    for (unsigned strip = 0; strip < 4; ++strip) {
        for (unsigned corner = 0; corner < DD2_SURFACE_TEST_VERTICES; ++corner) {
            uint8_t *vertex =
                fixture.vertices + (((size_t)strip * DD2_SURFACE_TEST_VERTICES + corner) *
                                    DD2_SURFACE_TEST_VERTEX_BYTES);
            const int32_t xpos =
                centers[strip][0] +
                (((int32_t)(corner % DD2_SURFACE_TEST_ROW_VERTICES) - 1) * DD2_SURFACE_TEST_SIDE);
            const int32_t zpos = centers[strip][1] + (corner < DD2_SURFACE_TEST_ROW_VERTICES
                                                          ? -DD2_SURFACE_TEST_SIDE
                                                          : DD2_SURFACE_TEST_SIDE);
            dd2_test_write_le32(vertex, (uint32_t)xpos);
            dd2_test_write_le32(vertex + DD2_TEST_WORD_BYTES, 0);
            dd2_test_write_le32(vertex + DD2_SURFACE_TEST_Z_OFFSET, (uint32_t)zpos);
        }
    }
    return dd2_road_create(&fixture.level, DD2_ROAD_RACING);
}

static bool dd2_ai_test_path(const dd2_road *road) {
    dd2_ai_path_query query = {.cell = 0, .lane = 1.0 / 2, .distance = dd2_ai_test_distance};
    dd2_ai_path_sample sample = {0};
    bool valid = dd2_ai_path(road, query, &sample) &&
                 fabs(sample.point.x - (double)DD2_SURFACE_TEST_SIDE) < dd2_ai_test_tolerance &&
                 fabs(sample.point.z - (2 * DD2_SURFACE_TEST_SIDE)) < dd2_ai_test_tolerance &&
                 fabs(sample.direction.x - 1) < dd2_ai_test_tolerance &&
                 fabs(sample.curvature - dd2_ai_test_curvature) < dd2_ai_test_tolerance;
    query.distance = dd2_ai_test_wrap;
    valid = valid && dd2_ai_path(road, query, &sample) &&
            fabs(sample.point.x) < dd2_ai_test_tolerance &&
            fabs(sample.point.z - (double)DD2_SURFACE_TEST_SIDE) < dd2_ai_test_tolerance &&
            sample.strip == 0;
    const union {
        uint64_t bits;
        double number;
    } invalid = {.bits = UINT64_C(0x7ff8000000000001)};
    query.lane = invalid.number;
    return valid && !dd2_ai_path(road, query, &sample) && sample.width == 0;
}

static bool dd2_ai_test_driver(const dd2_road *road, const dd2_road_surface *surface) {
    dd2_vehicle vehicles[2] = {0};
    if (!dd2_vehicle_reset(
            &vehicles[0],
            (dd2_vehicle_spawn){.position = {.x = dd2_ai_test_lane, .y = dd2_ai_test_height}}) ||
        !dd2_vehicle_reset(
            &vehicles[1],
            (dd2_vehicle_spawn){.position = {.x = -dd2_ai_test_lane, .y = dd2_ai_test_height}})) {
        return false;
    }
    dd2_ai_driver driver = {0};
    if (!dd2_ai_driver_reset(&driver,
                             (dd2_ai_start){.road = road, .cell = 1, .slot = 0, .count = 2})) {
        return false;
    }
    const dd2_ai_observation observation = {
        .road = road, .surface = surface, .vehicles = vehicles, .count = 2, .slot = 0};
    dd2_vehicle_control control = {0};
    bool valid = dd2_ai_driver_step(&driver, &observation, &control) && control.throttle > 0 &&
                 fabs(control.steer) < dd2_ai_test_tolerance && driver.steps == 1;
    for (unsigned step = 0; step < DD2_AI_TEST_STALL_STEPS && valid; ++step) {
        valid = dd2_ai_driver_step(&driver, &observation, &control);
    }
    valid = valid && control.throttle < 0 && driver.reverse_steps > 0;
    const dd2_ai_driver saved = driver;
    const union {
        uint64_t bits;
        double number;
    } invalid = {.bits = UINT64_C(0x7ff8000000000001)};
    vehicles[1].position.x = invalid.number;
    return valid && !dd2_ai_driver_step(&driver, &observation, &control) &&
           saved.cell == driver.cell && saved.lane == driver.lane &&
           saved.base_lane == driver.base_lane && saved.target == driver.target &&
           saved.stuck_steps == driver.stuck_steps && saved.reverse_steps == driver.reverse_steps &&
           saved.steps == driver.steps && control.throttle == 0 && control.steer == 0;
}

int main(void) {
    dd2_road *road = dd2_ai_test_road();
    dd2_road_surface *surface = dd2_road_surface_create(road);
    const bool valid = road != NULL && surface != NULL && dd2_ai_test_path(road) &&
                       dd2_ai_test_driver(road, surface);
    dd2_road_surface_destroy(surface);
    dd2_road_destroy(road);
    puts(valid ? "AI guidance/control/recovery validation: PASS" : "AI validation: FAIL");
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
