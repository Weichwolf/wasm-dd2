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

enum {
    DD2_VEHICLE_TEST_SETTLE = 1000,
    DD2_VEHICLE_TEST_DRIVE = 2000,
    DD2_VEHICLE_TEST_BRAKE = 800,
    DD2_VEHICLE_TEST_TURN = 200,
    DD2_VEHICLE_TEST_DROP = 1800,
    DD2_VEHICLE_TEST_AIR = 100,
    DD2_VEHICLE_TEST_TOP = 200,
    DD2_VEHICLE_TEST_INITIAL_HEIGHT = 190,
    DD2_VEHICLE_TEST_DROP_HEIGHT = 4000,
    DD2_VEHICLE_TEST_STOP_HEIGHT = 120,
    DD2_VEHICLE_TEST_MIN_SPEED = 1000
};
static const double dd2_vehicle_test_rest_height = 183.05555555555556;
static const double dd2_vehicle_test_tolerance = 1e-5;
static const double dd2_vehicle_test_stop_speed = 0.1;
static const double dd2_vehicle_test_quarter_turn = 1.57079632679489661923;

typedef struct {
    dd2_surface_test_fixture bytes;
    dd2_road *road;
    dd2_road_surface *surface;
} dd2_vehicle_test_track;

static bool dd2_vehicle_test_track_init(dd2_vehicle_test_track *track, unsigned layers,
                                        bool slippery, bool slope) {
    *track = (dd2_vehicle_test_track){0};
    dd2_surface_test_fixture_init(&track->bytes, layers);
    for (unsigned layer = 0; layer < layers; ++layer) {
        for (unsigned corner = 0; corner < DD2_SURFACE_TEST_VERTICES; ++corner) {
            uint8_t *vertex =
                track->bytes.vertices + (((size_t)layer * DD2_SURFACE_TEST_VERTICES + corner) *
                                         DD2_SURFACE_TEST_VERTEX_BYTES);
            const int32_t xpos = ((int32_t)(corner % DD2_SURFACE_TEST_ROW_VERTICES) - 1) *
                                 DD2_SURFACE_TEST_LONG_SIDE;
            const int32_t zpos = corner < DD2_SURFACE_TEST_ROW_VERTICES
                                     ? -DD2_SURFACE_TEST_LONG_SIDE
                                     : DD2_SURFACE_TEST_LONG_SIDE;
            const int32_t height = ((int32_t)layer * DD2_SURFACE_TEST_HEIGHT) +
                                   (slope ? zpos / DD2_VEHICLE_TEST_AIR : 0);
            dd2_test_write_le32(vertex, (uint32_t)xpos);
            dd2_test_write_le32(vertex + DD2_TEST_WORD_BYTES, (uint32_t)height);
            dd2_test_write_le32(vertex + DD2_SURFACE_TEST_Z_OFFSET, (uint32_t)zpos);
        }
        /* Two lane flags in the fixture's 36-byte header + 14-byte lanes. */
        enum { DD2_VEHICLE_TEST_HEADER = 36, DD2_VEHICLE_TEST_LANE = 14 };
        uint8_t *record =
            track->bytes.bytes + DD2_TEST_WORD_BYTES + ((size_t)layer * DD2_SURFACE_TEST_RECORD);
        record[DD2_VEHICLE_TEST_HEADER] = slippery ? 2U : 0U;
        record[DD2_VEHICLE_TEST_HEADER + DD2_VEHICLE_TEST_LANE] = slippery ? 2U : 0U;
    }
    track->road = dd2_road_create(&track->bytes.level, DD2_ROAD_RACING);
    track->surface = dd2_road_surface_create(track->road);
    return track->surface != NULL;
}

static void dd2_vehicle_test_track_destroy(dd2_vehicle_test_track *track) {
    dd2_road_surface_destroy(track->surface);
    dd2_road_destroy(track->road);
}

static bool dd2_vehicle_test_run(dd2_vehicle *vehicle, const dd2_vehicle_test_track *track,
                                 dd2_vehicle_control control, unsigned steps) {
    for (unsigned step = 0; step < steps; ++step) {
        if (!dd2_vehicle_step(vehicle, track->road, track->surface, control)) {
            printf("Vehicle step rejected at %u\n", step);
            return false;
        }
    }
    return true;
}

static bool dd2_vehicle_test_rest(const dd2_vehicle *vehicle, double floor) {
    bool valid = fabs(vehicle->position.y - floor - dd2_vehicle_test_rest_height) <
                     dd2_vehicle_test_tolerance &&
                 fabs(vehicle->velocity.y) < dd2_vehicle_test_tolerance;
    for (size_t wheel = 0; wheel < DD2_VEHICLE_WHEELS; ++wheel) {
        valid = valid && vehicle->wheels[wheel].grounded &&
                fabs(vehicle->wheels[wheel].contact.height - floor) < dd2_vehicle_test_tolerance &&
                vehicle->wheels[wheel].compression > 0 && vehicle->wheels[wheel].load > 0;
    }
    if (!valid) {
        printf("Rest height %.17g velocity %.17g expected floor %.17g\n", vehicle->position.y,
               vehicle->velocity.y, floor);
    }
    return valid;
}

static bool dd2_vehicle_test_driving(void) {
    dd2_vehicle_test_track track;
    bool valid = dd2_vehicle_test_track_init(&track, 1, false, false);
    dd2_vehicle vehicle = {0};
    valid =
        valid &&
        dd2_vehicle_reset(
            &vehicle,
            (dd2_vehicle_spawn){.position =
                                    (dd2_vehicle_vector){.y = DD2_VEHICLE_TEST_INITIAL_HEIGHT},
                                .yaw = 0}) &&
        dd2_vehicle_test_run(&vehicle, &track, (dd2_vehicle_control){0}, DD2_VEHICLE_TEST_SETTLE) &&
        dd2_vehicle_test_rest(&vehicle, 0);
    valid = valid &&
            dd2_vehicle_test_run(&vehicle, &track, (dd2_vehicle_control){.throttle = 1},
                                 DD2_VEHICLE_TEST_DRIVE) &&
            vehicle.velocity.z > (double)DD2_VEHICLE_TEST_MIN_SPEED && vehicle.position.z > 0 &&
            fabs(vehicle.position.x) < dd2_vehicle_test_tolerance;
    valid = valid &&
            dd2_vehicle_test_run(&vehicle, &track, (dd2_vehicle_control){.brake = 1},
                                 DD2_VEHICLE_TEST_BRAKE) &&
            fabs(vehicle.velocity.z) < dd2_vehicle_test_stop_speed;
    const double stopped = vehicle.position.z;
    valid = valid &&
            dd2_vehicle_test_run(&vehicle, &track, (dd2_vehicle_control){.throttle = -1},
                                 DD2_VEHICLE_TEST_DRIVE) &&
            vehicle.velocity.z < -DD2_VEHICLE_TEST_MIN_SPEED && vehicle.position.z < stopped;
    valid =
        valid &&
        dd2_vehicle_test_run(&vehicle, &track, (dd2_vehicle_control){.throttle = -1, .steer = 1},
                             DD2_VEHICLE_TEST_TURN) &&
        vehicle.rotation.y < 0;
    valid = valid &&
            dd2_vehicle_reset(
                &vehicle,
                (dd2_vehicle_spawn){.position =
                                        (dd2_vehicle_vector){.y = DD2_VEHICLE_TEST_INITIAL_HEIGHT},
                                    .yaw = 0}) &&
            dd2_vehicle_test_run(&vehicle, &track, (dd2_vehicle_control){.throttle = 1},
                                 DD2_VEHICLE_TEST_TURN) &&
            dd2_vehicle_test_run(&vehicle, &track, (dd2_vehicle_control){.throttle = 1, .steer = 1},
                                 DD2_VEHICLE_TEST_TURN) &&
            vehicle.rotation.y > 0;
    dd2_vehicle_test_track_destroy(&track);
    return valid;
}

static bool dd2_vehicle_test_landing(void) {
    dd2_vehicle_test_track track;
    bool valid = dd2_vehicle_test_track_init(&track, 3, false, false);
    dd2_vehicle vehicle = {0};
    valid =
        valid &&
        dd2_vehicle_reset(
            &vehicle,
            (dd2_vehicle_spawn){.position =
                                    (dd2_vehicle_vector){.y = DD2_VEHICLE_TEST_INITIAL_HEIGHT},
                                .yaw = 0}) &&
        dd2_vehicle_test_run(&vehicle, &track, (dd2_vehicle_control){0}, DD2_VEHICLE_TEST_SETTLE) &&
        dd2_vehicle_test_rest(&vehicle, 0);
    valid = valid &&
            dd2_vehicle_reset(
                &vehicle,
                (dd2_vehicle_spawn){
                    .position = (dd2_vehicle_vector){.y = DD2_VEHICLE_TEST_DROP_HEIGHT}, .yaw = 0});
    for (unsigned step = 0; valid && step < DD2_VEHICLE_TEST_DROP; ++step) {
        valid = dd2_vehicle_step(&vehicle, track.road, track.surface, (dd2_vehicle_control){0}) &&
                vehicle.position.y >= DD2_VEHICLE_TEST_TOP + DD2_VEHICLE_TEST_STOP_HEIGHT;
    }
    valid = valid && dd2_vehicle_test_rest(&vehicle, DD2_VEHICLE_TEST_TOP);
    /* One step would cross all layers: the swept window catches the top first. */
    valid = valid &&
            dd2_vehicle_reset(
                &vehicle,
                (dd2_vehicle_spawn){
                    .position = (dd2_vehicle_vector){.y = DD2_VEHICLE_TEST_DROP_HEIGHT}, .yaw = 0});
    vehicle.velocity.y = -DD2_SURFACE_TEST_LONG_SIDE;
    valid = valid &&
            dd2_vehicle_step(&vehicle, track.road, track.surface, (dd2_vehicle_control){0}) &&
            vehicle.position.y >= DD2_VEHICLE_TEST_TOP + DD2_VEHICLE_TEST_STOP_HEIGHT &&
            vehicle.velocity.y == 0;
    dd2_vehicle_test_track_destroy(&track);
    return valid;
}

static bool dd2_vehicle_test_airborne(void) {
    dd2_vehicle_test_track track;
    bool valid = dd2_vehicle_test_track_init(&track, 1, false, false);
    dd2_vehicle first = {0};
    valid = valid &&
            dd2_vehicle_reset(
                &first,
                (dd2_vehicle_spawn){
                    .position = (dd2_vehicle_vector){.y = DD2_VEHICLE_TEST_DROP_HEIGHT}, .yaw = 0});
    dd2_vehicle second = first;
    valid = valid &&
            dd2_vehicle_test_run(&first, &track, (dd2_vehicle_control){0}, DD2_VEHICLE_TEST_AIR) &&
            dd2_vehicle_test_run(&second, &track,
                                 (dd2_vehicle_control){.throttle = 1, .steer = 1, .brake = 1},
                                 DD2_VEHICLE_TEST_AIR) &&
            first.position.y < (double)DD2_VEHICLE_TEST_DROP_HEIGHT && first.velocity.y < 0 &&
            first.position.y == second.position.y && first.position.x == second.position.x &&
            first.position.z == second.position.z;
    for (size_t wheel = 0; wheel < DD2_VEHICLE_WHEELS; ++wheel) {
        valid = valid && !first.wheels[wheel].grounded && !second.wheels[wheel].grounded;
    }
    dd2_vehicle_test_track_destroy(&track);
    return valid;
}

static bool dd2_vehicle_test_slope_and_grip(void) {
    dd2_vehicle_test_track dry;
    dd2_vehicle_test_track wet;
    dd2_vehicle_test_track slope;
    bool valid = dd2_vehicle_test_track_init(&dry, 1, false, false);
    valid = dd2_vehicle_test_track_init(&wet, 1, true, false) && valid;
    valid = dd2_vehicle_test_track_init(&slope, 1, false, true) && valid;
    dd2_vehicle first = {0};
    valid = valid &&
            dd2_vehicle_reset(
                &first, (dd2_vehicle_spawn){
                            .position = (dd2_vehicle_vector){.y = DD2_VEHICLE_TEST_INITIAL_HEIGHT},
                            .yaw = 0});
    dd2_vehicle second = first;
    valid = valid &&
            dd2_vehicle_test_run(&first, &dry, (dd2_vehicle_control){.throttle = 1},
                                 DD2_VEHICLE_TEST_TURN) &&
            dd2_vehicle_test_run(&second, &wet, (dd2_vehicle_control){.throttle = 1},
                                 DD2_VEHICLE_TEST_TURN) &&
            first.velocity.z > second.velocity.z;
    valid = valid &&
            dd2_vehicle_reset(
                &first,
                (dd2_vehicle_spawn){.position =
                                        (dd2_vehicle_vector){.y = DD2_VEHICLE_TEST_INITIAL_HEIGHT},
                                    .yaw = 0}) &&
            dd2_vehicle_test_run(&first, &slope, (dd2_vehicle_control){.brake = 1},
                                 DD2_VEHICLE_TEST_SETTLE);
    const dd2_vehicle_vector up_axis =
        dd2_vehicle_rotate(first.rotation, (dd2_vehicle_vector){.y = 1});
    valid = valid && up_axis.y > dd2_vehicle_test_stop_speed && up_axis.z < 0;
    dd2_vehicle_test_track_destroy(&dry);
    dd2_vehicle_test_track_destroy(&wet);
    dd2_vehicle_test_track_destroy(&slope);
    return valid;
}

static double dd2_vehicle_test_binary64(uint64_t bits) {
    const union {
        uint64_t bits;
        double value;
    } encoded = {.bits = bits};
    return encoded.value;
}

static bool dd2_vehicle_test_number_equal(const double *first, const double *second) {
    const unsigned char *left = (const unsigned char *)first;
    const unsigned char *right = (const unsigned char *)second;
    for (size_t index = 0; index < sizeof(*first); ++index) {
        if (left[index] != right[index]) {
            return false;
        }
    }
    return true;
}

static bool dd2_vehicle_test_vector_equal(const dd2_vehicle_vector *first,
                                          const dd2_vehicle_vector *second) {
    return dd2_vehicle_test_number_equal(&first->x, &second->x) &&
           dd2_vehicle_test_number_equal(&first->y, &second->y) &&
           dd2_vehicle_test_number_equal(&first->z, &second->z);
}

static bool dd2_vehicle_test_unchanged(const dd2_vehicle *saved, const dd2_vehicle *vehicle) {
    bool valid =
        dd2_vehicle_test_vector_equal(&saved->position, &vehicle->position) &&
        dd2_vehicle_test_vector_equal(&saved->velocity, &vehicle->velocity) &&
        dd2_vehicle_test_vector_equal(&saved->angular_velocity, &vehicle->angular_velocity) &&
        dd2_vehicle_test_number_equal(&saved->rotation.x, &vehicle->rotation.x) &&
        dd2_vehicle_test_number_equal(&saved->rotation.y, &vehicle->rotation.y) &&
        dd2_vehicle_test_number_equal(&saved->rotation.z, &vehicle->rotation.z) &&
        dd2_vehicle_test_number_equal(&saved->rotation.w, &vehicle->rotation.w) &&
        dd2_vehicle_test_number_equal(&saved->steering, &vehicle->steering) &&
        saved->steps == vehicle->steps;
    for (size_t index = 0; index < DD2_VEHICLE_WHEELS; ++index) {
        const dd2_vehicle_wheel *first = &saved->wheels[index];
        const dd2_vehicle_wheel *second = &vehicle->wheels[index];
        valid = valid && dd2_vehicle_test_vector_equal(&first->mount, &second->mount) &&
                dd2_vehicle_test_vector_equal(&first->center, &second->center) &&
                dd2_vehicle_test_number_equal(&first->compression, &second->compression) &&
                dd2_vehicle_test_number_equal(&first->load, &second->load) &&
                first->grounded == second->grounded &&
                first->contact.cell == second->contact.cell &&
                first->contact.triangle == second->contact.triangle &&
                dd2_vehicle_test_number_equal(&first->contact.height, &second->contact.height);
        for (size_t axis = 0; axis < 3; ++axis) {
            valid = valid && dd2_vehicle_test_number_equal(&first->contact.normal[axis],
                                                           &second->contact.normal[axis]);
        }
    }
    return valid;
}

static bool dd2_vehicle_test_invalid(void) {
    dd2_vehicle_test_track track;
    bool valid = dd2_vehicle_test_track_init(&track, 1, false, false);
    dd2_vehicle vehicle = {0};
    valid = valid && dd2_vehicle_reset(
                         &vehicle,
                         (dd2_vehicle_spawn){
                             .position = (dd2_vehicle_vector){.y = DD2_VEHICLE_TEST_INITIAL_HEIGHT},
                             .yaw = dd2_vehicle_test_quarter_turn});
    const dd2_vehicle_vector forward =
        dd2_vehicle_rotate(vehicle.rotation, (dd2_vehicle_vector){.z = 1});
    valid = valid && fabs(forward.x - 1) < dd2_vehicle_test_tolerance &&
            fabs(forward.z) < dd2_vehicle_test_tolerance;
    if (!valid) {
        printf("Invalid test setup/rotation: forward %.17g %.17g\n", forward.x, forward.z);
    }
    const double nan_value = dd2_vehicle_test_binary64(UINT64_C(0x7ff8000000000000));
    const double infinity = dd2_vehicle_test_binary64(UINT64_C(0x7ff0000000000000));
    const dd2_vehicle_control controls[] = {{.throttle = nan_value}, {.brake = infinity},
                                            {.steer = nan_value},    {.throttle = 2},
                                            {.brake = -1},           {.steer = -2}};
    dd2_vehicle saved = vehicle;
    for (size_t index = 0; index < sizeof(controls) / sizeof(controls[0]); ++index) {
        const bool accepted =
            dd2_vehicle_step(&vehicle, track.road, track.surface, controls[index]);
        const bool unchanged = dd2_vehicle_test_unchanged(&saved, &vehicle);
        if (accepted || !unchanged) {
            printf("Invalid control %zu: accepted %d unchanged %d\n", index, (int)accepted,
                   (int)unchanged);
        }
        valid = valid && !accepted && unchanged;
    }
    vehicle.position.x = nan_value;
    saved = vehicle;
    const bool nan_accepted =
        dd2_vehicle_step(&vehicle, track.road, track.surface, (dd2_vehicle_control){0});
    const bool nan_unchanged = dd2_vehicle_test_unchanged(&saved, &vehicle);
    if (nan_accepted || !nan_unchanged) {
        printf("Invalid position: accepted %d unchanged %d\n", (int)nan_accepted,
               (int)nan_unchanged);
    }
    valid = valid && !nan_accepted && nan_unchanged;
    vehicle.position.x = 0;
    vehicle.rotation.w = 0;
    const bool invalid_rotation =
        dd2_vehicle_step(&vehicle, track.road, track.surface, (dd2_vehicle_control){0});
    const bool invalid_height = dd2_vehicle_reset(
        &vehicle, (dd2_vehicle_spawn){.position = (dd2_vehicle_vector){.y = infinity}, .yaw = 0});
    const bool invalid_yaw = dd2_vehicle_reset(
        &vehicle, (dd2_vehicle_spawn){.position = (dd2_vehicle_vector){0}, .yaw = nan_value});
    const bool invalid_pointer =
        dd2_vehicle_step(NULL, track.road, track.surface, (dd2_vehicle_control){0});
    if (invalid_rotation || invalid_height || invalid_yaw || invalid_pointer) {
        printf("Invalid core checks: rotation %d height %d yaw %d pointer %d\n",
               (int)invalid_rotation, (int)invalid_height, (int)invalid_yaw, (int)invalid_pointer);
    }
    valid = valid && !invalid_rotation && !invalid_height && !invalid_yaw && !invalid_pointer;
    dd2_vehicle_test_track_destroy(&track);
    return valid;
}

static bool dd2_vehicle_test_rotations(void) {
    enum { DD2_ROTATION_TEST_SIDE = 3, DD2_ROTATION_TEST_LONG = 5 };
    const double half_sine = sin(dd2_vehicle_test_quarter_turn / 2);
    const double half_cosine = cos(dd2_vehicle_test_quarter_turn / 2);
    const double tolerance = 1e-12;
    const dd2_vehicle_vector inputs[] = {
        {.x = 1},
        {.y = 1},
        {.z = 1},
        {.x = 2, .y = -DD2_ROTATION_TEST_SIDE, .z = DD2_ROTATION_TEST_LONG}};
    /* Positive quarter turns permute signed Cartesian coordinates. These
     * expected vectors are independent of the quaternion implementation. */
    const struct {
        dd2_vehicle_rotation rotation;
        dd2_vehicle_vector expected[sizeof(inputs) / sizeof(inputs[0])];
    } cases[] = {{{.w = 1},
                  {{.x = 1},
                   {.y = 1},
                   {.z = 1},
                   {.x = 2, .y = -DD2_ROTATION_TEST_SIDE, .z = DD2_ROTATION_TEST_LONG}}},
                 {{.x = half_sine, .w = half_cosine},
                  {{.x = 1},
                   {.z = 1},
                   {.y = -1},
                   {.x = 2, .y = -DD2_ROTATION_TEST_LONG, .z = -DD2_ROTATION_TEST_SIDE}}},
                 {{.y = half_sine, .w = half_cosine},
                  {{.z = -1},
                   {.y = 1},
                   {.x = 1},
                   {.x = DD2_ROTATION_TEST_LONG, .y = -DD2_ROTATION_TEST_SIDE, .z = -2}}},
                 {{.z = half_sine, .w = half_cosine},
                  {{.y = 1},
                   {.x = -1},
                   {.z = 1},
                   {.x = DD2_ROTATION_TEST_SIDE, .y = 2, .z = DD2_ROTATION_TEST_LONG}}}};
    for (size_t rotation = 0; rotation < sizeof(cases) / sizeof(cases[0]); ++rotation) {
        for (size_t vector = 0; vector < sizeof(inputs) / sizeof(inputs[0]); ++vector) {
            const dd2_vehicle_vector expected = cases[rotation].expected[vector];
            const dd2_vehicle_vector output =
                dd2_vehicle_rotate(cases[rotation].rotation, inputs[vector]);
            if (fabs(output.x - expected.x) > tolerance ||
                fabs(output.y - expected.y) > tolerance ||
                fabs(output.z - expected.z) > tolerance) {
                printf("Vehicle rotation case=%zu vector=%zu failed\n", rotation, vector);
                return false;
            }
        }
    }
    return true;
}

int main(void) {
    const struct {
        const char *name;
        bool (*check)(void);
    } checks[] = {
        {"rotations", dd2_vehicle_test_rotations},       {"driving", dd2_vehicle_test_driving},
        {"landing", dd2_vehicle_test_landing},           {"airborne", dd2_vehicle_test_airborne},
        {"slope/grip", dd2_vehicle_test_slope_and_grip}, {"invalid", dd2_vehicle_test_invalid}};
    for (size_t index = 0; index < sizeof(checks) / sizeof(checks[0]); ++index) {
        if (!checks[index].check()) {
            printf("Vehicle %s failed\n", checks[index].name);
            return EXIT_FAILURE;
        }
    }
    puts("vehicle dynamics: PASS");
    return EXIT_SUCCESS;
}
