#include "ai/driver.h"
#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "game/driving.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

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

static bool dd2_drive_test_vector(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return fabs(first.x - second.x) < dd2_drive_test_tolerance &&
           fabs(first.y - second.y) < dd2_drive_test_tolerance &&
           fabs(first.z - second.z) < dd2_drive_test_tolerance;
}

static bool dd2_drive_test_contacts(const dd2_driving *first, const dd2_driving *second) {
    const dd2_vehicle_collision_report *left = dd2_driving_contact_report(first);
    const dd2_vehicle_collision_report *right = dd2_driving_contact_report(second);
    if (left->count != right->count || left->pair_contacts != right->pair_contacts) {
        return false;
    }
    for (unsigned index = 0; index < left->count; ++index) {
        const dd2_vehicle_contact left_contact = left->contacts[index];
        const dd2_vehicle_contact right_contact = right->contacts[index];
        if (left_contact.kind != right_contact.kind || left_contact.first != right_contact.first ||
            left_contact.second != right_contact.second ||
            left_contact.obstacle != right_contact.obstacle ||
            fabs(left_contact.time - right_contact.time) > dd2_drive_test_tolerance ||
            fabs(left_contact.normal_speed - right_contact.normal_speed) >
                dd2_drive_test_tolerance ||
            fabs(left_contact.impulse - right_contact.impulse) > dd2_drive_test_tolerance ||
            !dd2_drive_test_vector(left_contact.point, right_contact.point) ||
            !dd2_drive_test_vector(left_contact.normal, right_contact.normal) ||
            !dd2_drive_test_vector(left_contact.local_points[0], right_contact.local_points[0]) ||
            !dd2_drive_test_vector(left_contact.local_points[1], right_contact.local_points[1])) {
            return false;
        }
    }
    return true;
}

static bool dd2_drive_test_field(const dd2_driving *first, const dd2_driving *second) {
    const dd2_ai_driver *first_drivers = dd2_driving_drivers(first);
    const dd2_ai_driver *second_drivers = dd2_driving_drivers(second);
    for (unsigned slot = 0; slot < dd2_driving_vehicle_count(first); ++slot) {
        const dd2_ai_driver driver = first_drivers[slot];
        const dd2_ai_driver other = second_drivers[slot];
        if (!dd2_drive_test_same(&dd2_driving_vehicles(first)[slot],
                                 &dd2_driving_vehicles(second)[slot]) ||
            fabs(dd2_driving_wheel_rolls(first)[slot] - dd2_driving_wheel_rolls(second)[slot]) >
                dd2_drive_test_tolerance ||
            driver.cell != other.cell || driver.lane != other.lane ||
            driver.base_lane != other.base_lane || driver.target != other.target ||
            driver.stuck_steps != other.stuck_steps ||
            driver.reverse_steps != other.reverse_steps || driver.steps != other.steps) {
            return false;
        }
    }
    return dd2_drive_test_contacts(first, second);
}

static bool dd2_drive_test_clear_contacts(dd2_driving *driving) {
    const dd2_vehicle_collision_report *report = dd2_driving_contact_report(driving);
    const unsigned count = report->count;
    const unsigned pairs = report->pair_contacts;
    const dd2_vehicle_contact contact = report->contacts[0];
    return !dd2_driving_advance(driving, (dd2_driving_frame){.seconds = -1}) &&
           report->count == count && report->pair_contacts == pairs &&
           report->contacts[0].impulse == contact.impulse &&
           dd2_driving_advance(driving, (dd2_driving_frame){0}) && report->count == 0 &&
           report->pair_contacts == 0 && report->impacts[0].contacts == 0;
}

static bool dd2_drive_test_frames(dd2_driving *first, dd2_driving *second) {
    const dd2_vehicle initial = *dd2_driving_vehicle(first);
    const dd2_vehicle_control control = {.throttle = 1, .steer = 0.1};
    bool cleared_contacts = false;
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
        if (!cleared_contacts && dd2_driving_contact_report(first)->count != 0) {
            if (!dd2_drive_test_clear_contacts(first) || !dd2_drive_test_clear_contacts(second)) {
                return false;
            }
            cleared_contacts = true;
        }
    }
    if (!cleared_contacts || !dd2_drive_test_field(first, second) ||
        fabs(dd2_driving_wheel_roll(first) - dd2_driving_wheel_roll(second)) >
            dd2_drive_test_tolerance ||
        dd2_driving_vehicle(first)->steps !=
            DD2_DRIVE_TEST_RESET_STEPS + (DD2_DRIVE_TEST_FRAMES * DD2_DRIVE_TEST_PARTS)) {
        return false;
    }
    if (!dd2_driving_reset(first) || !dd2_driving_reset(second) ||
        !dd2_drive_test_field(first, second) || dd2_driving_contact_report(first)->count != 0 ||
        !dd2_drive_test_same(&initial, dd2_driving_vehicle(first))) {
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
    passed = passed && bad == NULL && !dd2_driving_advance(NULL, (dd2_driving_frame){0}) &&
             dd2_driving_contact_report(NULL) == NULL;
    dd2_driving_destroy(bad);
    dd2_driving_destroy(first);
    dd2_driving_destroy(second);
    dd2_driving_destroy(last);
    dd2_road_destroy(road);
    puts(passed ? "driving timing/reset/start validation: PASS" : "driving validation: FAIL");
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
