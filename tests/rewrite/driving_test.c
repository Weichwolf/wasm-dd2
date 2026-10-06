#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "game/driving.h"
#include "physics/vehicle.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_DRIVE_TEST_SIDE = 32,
    DD2_DRIVE_TEST_VERTICES = DD2_DRIVE_TEST_SIDE * DD2_DRIVE_TEST_SIDE,
    DD2_DRIVE_TEST_VERTEX_BYTES = 12,
    DD2_DRIVE_TEST_LANE_BYTES = 14,
    DD2_DRIVE_TEST_Z_OFFSET = 8,
    DD2_DRIVE_TEST_SPACING = 2000,
    DD2_DRIVE_TEST_ORIGIN = 32000,
    DD2_DRIVE_TEST_ARENA = 8,
    DD2_DRIVE_TEST_LAST_ARENA = 11,
    DD2_DRIVE_TEST_FRAMES = 400,
    DD2_DRIVE_TEST_PARTS = 5,
    DD2_DRIVE_TEST_RESET_STEPS = 200
};
static const double dd2_drive_test_frame_seconds = 0.025;
static const double dd2_drive_test_partial_seconds = 0.004;
static const double dd2_drive_test_resume_seconds = 0.001;
static const double dd2_drive_test_tolerance = 1e-8;
static const double dd2_drive_test_start = -12000;
static const double dd2_drive_test_quarter_turn = 1.57079632679489661923;

static dd2_road *dd2_drive_test_road(void) {
    uint8_t vertices[DD2_DRIVE_TEST_VERTICES * DD2_DRIVE_TEST_VERTEX_BYTES] = {0};
    uint8_t source[DD2_DRIVE_TEST_VERTICES * DD2_DRIVE_TEST_LANE_BYTES] = {0};
    for (unsigned index = 0; index < DD2_DRIVE_TEST_VERTICES; ++index) {
        uint8_t *vertex = vertices + ((size_t)index * DD2_DRIVE_TEST_VERTEX_BYTES);
        const int32_t xpos = ((int32_t)(index % DD2_DRIVE_TEST_SIDE) * DD2_DRIVE_TEST_SPACING) -
                             DD2_DRIVE_TEST_ORIGIN;
        const int32_t zpos = ((int32_t)(index / DD2_DRIVE_TEST_SIDE) * DD2_DRIVE_TEST_SPACING) -
                             DD2_DRIVE_TEST_ORIGIN;
        dd2_test_write_le32(vertex, (uint32_t)xpos);
        dd2_test_write_le32(vertex + DD2_DRIVE_TEST_Z_OFFSET, (uint32_t)zpos);
    }
    dd2_level_data level = {.vertex_count = DD2_DRIVE_TEST_VERTICES};
    level.sections[DD2_LEVEL_ROAD_VERTICES] =
        (dd2_byte_view){.data = vertices, .size = sizeof(vertices)};
    level.sections[DD2_LEVEL_ROAD_STRIPS] = (dd2_byte_view){.data = source, .size = sizeof(source)};
    return dd2_road_create(&level, DD2_ROAD_ARENA);
}

static bool dd2_drive_test_same(const dd2_vehicle *first, const dd2_vehicle *second) {
    return first->steps == second->steps &&
           fabs(first->position.x - second->position.x) < dd2_drive_test_tolerance &&
           fabs(first->position.y - second->position.y) < dd2_drive_test_tolerance &&
           fabs(first->position.z - second->position.z) < dd2_drive_test_tolerance &&
           fabs(first->velocity.x - second->velocity.x) < dd2_drive_test_tolerance &&
           fabs(first->velocity.y - second->velocity.y) < dd2_drive_test_tolerance &&
           fabs(first->velocity.z - second->velocity.z) < dd2_drive_test_tolerance &&
           fabs(first->rotation.x - second->rotation.x) < dd2_drive_test_tolerance &&
           fabs(first->rotation.y - second->rotation.y) < dd2_drive_test_tolerance &&
           fabs(first->rotation.z - second->rotation.z) < dd2_drive_test_tolerance &&
           fabs(first->rotation.w - second->rotation.w) < dd2_drive_test_tolerance;
}

static bool dd2_drive_test_frames(dd2_driving *first, dd2_driving *second) {
    const dd2_vehicle initial = *dd2_driving_vehicle(first);
    const dd2_vehicle_control control = {.throttle = 1, .steer = 0.1};
    for (unsigned frame = 0; frame < DD2_DRIVE_TEST_FRAMES; ++frame) {
        if (!dd2_driving_advance(first, (dd2_driving_frame){.seconds = dd2_drive_test_frame_seconds,
                                                            .control = control})) {
            return false;
        }
        for (unsigned part = 0; part < DD2_DRIVE_TEST_PARTS; ++part) {
            if (!dd2_driving_advance(
                    second,
                    (dd2_driving_frame){.seconds = DD2_VEHICLE_STEP_SECONDS, .control = control})) {
                return false;
            }
        }
    }
    if (!dd2_drive_test_same(dd2_driving_vehicle(first), dd2_driving_vehicle(second)) ||
        fabs(dd2_driving_wheel_roll(first) - dd2_driving_wheel_roll(second)) >
            dd2_drive_test_tolerance ||
        dd2_driving_vehicle(first)->steps !=
            DD2_DRIVE_TEST_RESET_STEPS + (DD2_DRIVE_TEST_FRAMES * DD2_DRIVE_TEST_PARTS)) {
        return false;
    }
    if (!dd2_driving_reset(first) || !dd2_drive_test_same(&initial, dd2_driving_vehicle(first))) {
        return false;
    }
    /* A partial frame followed by pause must not leak into resumed simulation. */
    if (!dd2_driving_advance(first, (dd2_driving_frame){.seconds = dd2_drive_test_partial_seconds,
                                                        .control = control})) {
        return false;
    }
    dd2_driving_suspend(first);
    return dd2_driving_advance(first, (dd2_driving_frame){.seconds = dd2_drive_test_resume_seconds,
                                                          .control = control}) &&
           dd2_drive_test_same(&initial, dd2_driving_vehicle(first));
}

static bool dd2_drive_test_rejection(dd2_driving *driving) {
    const dd2_vehicle initial = *dd2_driving_vehicle(driving);
    const union {
        uint64_t bits;
        double number;
    } invalid = {.bits = UINT64_C(0x7ff8000000000001)};
    const dd2_driving_frame bad[] = {{.seconds = invalid.number},
                                     {.seconds = -1},
                                     {.seconds = 1},
                                     {.control = {.throttle = invalid.number}},
                                     {.control = {.brake = -1}},
                                     {.control = {.steer = 2}}};
    for (size_t index = 0; index < sizeof(bad) / sizeof(bad[0]); ++index) {
        if (dd2_driving_advance(driving, bad[index]) ||
            !dd2_drive_test_same(&initial, dd2_driving_vehicle(driving))) {
            return false;
        }
    }
    return true;
}

int main(void) {
    dd2_road *road = dd2_drive_test_road();
    dd2_driving *first = dd2_driving_create(road, DD2_DRIVE_TEST_ARENA);
    dd2_driving *second = dd2_driving_create(road, DD2_DRIVE_TEST_ARENA);
    dd2_driving *last = dd2_driving_create(road, DD2_DRIVE_TEST_LAST_ARENA);
    bool passed = first != NULL && second != NULL && last != NULL;
    if (passed) {
        const dd2_vehicle_spawn *spawn = dd2_driving_start(first);
        const dd2_vehicle_spawn *other = dd2_driving_start(last);
        passed = spawn->position.x == 0 && spawn->position.z == dd2_drive_test_start &&
                 spawn->yaw == 0 && other->position.x == dd2_drive_test_start &&
                 other->position.z == 0 && other->yaw == dd2_drive_test_quarter_turn &&
                 dd2_drive_test_frames(first, second) && dd2_drive_test_rejection(first);
    }
    dd2_driving *bad = dd2_driving_create(road, 1);
    passed = passed && bad == NULL && !dd2_driving_advance(NULL, (dd2_driving_frame){0});
    dd2_driving_destroy(bad);
    dd2_driving_destroy(first);
    dd2_driving_destroy(second);
    dd2_driving_destroy(last);
    dd2_road_destroy(road);
    puts(passed ? "driving timing/reset/start validation: PASS" : "driving validation: FAIL");
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
