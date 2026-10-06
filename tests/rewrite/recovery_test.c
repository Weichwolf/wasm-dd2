#include "asset_fixture.h"
#include "assets/road.h"
#include "game/recovery.h"
#include "physics/damage.h"
#include "physics/road_contact.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"
#include "surface_fixture.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_RECOVERY_TEST_STEPS = 123,
    DD2_RECOVERY_TEST_DRIVE_STEPS = 2,
    DD2_RECOVERY_TEST_ROOF_HEIGHT = 130,
    DD2_RECOVERY_TEST_SIDE_HEIGHT = 186,
    DD2_RECOVERY_TEST_NOSE_HEIGHT = 450,
    DD2_RECOVERY_TEST_RIDE_HEIGHT = 190,
    DD2_RECOVERY_TEST_DISTANCE = 8192,
    DD2_RECOVERY_TEST_AIR_HEIGHT = 1000,
    DD2_RECOVERY_TEST_TOP = 7,
    DD2_RECOVERY_TEST_LEVEL_HEIGHT = 10000,
    DD2_RECOVERY_TEST_CARS = 2
};
static const double dd2_recovery_test_tolerance = 1e-8;
static const double dd2_recovery_test_steering = 0.1;

typedef struct {
    dd2_surface_test_fixture bytes;
    dd2_road *road;
    dd2_road_surface *surface;
} dd2_recovery_test_track;

static bool dd2_recovery_test_track_init(dd2_recovery_test_track *track) {
    *track = (dd2_recovery_test_track){0};
    dd2_surface_test_fixture_init(&track->bytes, DD2_SURFACE_TEST_LEVELS);
    for (size_t layer = 0; layer < DD2_SURFACE_TEST_LEVELS; ++layer) {
        for (size_t corner = 0; corner < DD2_SURFACE_TEST_VERTICES; ++corner) {
            uint8_t *vertex =
                track->bytes.vertices +
                (((layer * DD2_SURFACE_TEST_VERTICES) + corner) * DD2_SURFACE_TEST_VERTEX_BYTES);
            const int32_t xpos = ((int32_t)(corner % DD2_SURFACE_TEST_ROW_VERTICES) - 1) *
                                 DD2_SURFACE_TEST_LONG_SIDE;
            const int32_t zpos = corner < DD2_SURFACE_TEST_ROW_VERTICES
                                     ? -DD2_SURFACE_TEST_LONG_SIDE
                                     : DD2_SURFACE_TEST_LONG_SIDE;
            dd2_test_write_le32(vertex, (uint32_t)xpos);
            dd2_test_write_le32(vertex + DD2_TEST_WORD_BYTES,
                                (uint32_t)(layer * DD2_RECOVERY_TEST_LEVEL_HEIGHT));
            dd2_test_write_le32(vertex + DD2_SURFACE_TEST_Z_OFFSET, (uint32_t)zpos);
        }
    }
    track->road = dd2_road_create(&track->bytes.level, DD2_ROAD_RACING);
    track->surface = dd2_road_surface_create(track->road);
    return track->surface != NULL;
}

static dd2_recovery_frame dd2_recovery_test_frame(const dd2_recovery_test_track *track,
                                                  const dd2_vehicle_damage *damage,
                                                  unsigned count) {
    return (dd2_recovery_frame){
        .road = track->road, .surface = track->surface, .damage = damage, .count = count};
}

static bool dd2_recovery_test_clock(const dd2_recovery_test_track *track, unsigned floor,
                                    unsigned pose) {
    const double root = sqrt(1.0 / 2);
    const dd2_vehicle_rotation rotations[] = {
        {.z = 1}, {.z = root, .w = root}, {.x = root, .w = root}};
    const double heights[] = {DD2_RECOVERY_TEST_ROOF_HEIGHT, DD2_RECOVERY_TEST_SIDE_HEIGHT,
                              DD2_RECOVERY_TEST_NOSE_HEIGHT};
    dd2_vehicle vehicle = {0};
    if (!dd2_vehicle_reset(&vehicle,
                           (dd2_vehicle_spawn){.position = {.y = floor + heights[pose]}})) {
        return false;
    }
    vehicle.rotation = rotations[pose];
    vehicle.velocity.x = 1;
    vehicle.steering = dd2_recovery_test_steering;
    vehicle.steps = DD2_RECOVERY_TEST_STEPS;
    dd2_recovery_driver state = {0};
    const dd2_vehicle_damage damage = {.regions = {0.5}};
    const dd2_recovery_frame frame = dd2_recovery_test_frame(track, &damage, 1);
    for (unsigned tick = 1; tick < DD2_RECOVERY_REST_STEPS; ++tick) {
        if (!dd2_recovery_step(&state, &vehicle, frame) || state.rest_steps != tick ||
            !state.overturned || state.recoveries != 0 ||
            vehicle.position.y != floor + heights[pose] ||
            vehicle.steps != DD2_RECOVERY_TEST_STEPS) {
            printf("Recovery prematurely changes roof/side pose at tick %u\n", tick);
            return false;
        }
    }
    if (!dd2_recovery_step(&state, &vehicle, frame) || state.overturned || state.rest_steps != 0 ||
        state.recoveries != 1 || vehicle.steps != DD2_RECOVERY_TEST_STEPS ||
        fabs(vehicle.position.y - floor - (double)DD2_RECOVERY_TEST_RIDE_HEIGHT) >
            2 * DD2_ROAD_EDGE_TOLERANCE ||
        vehicle.velocity.x != 1 || vehicle.velocity.y != 0 ||
        vehicle.steering != dd2_recovery_test_steering ||
        fabs(dd2_vehicle_rotate(vehicle.rotation, (dd2_vehicle_vector){.y = 1}).y - 1) >
            dd2_recovery_test_tolerance) {
        printf("Recovery deadline/pose differs: floor=%u pose=%u height=%.17g\n", floor, pose,
               vehicle.position.y);
        return false;
    }
    for (unsigned wheel = 0; wheel < DD2_VEHICLE_WHEELS; ++wheel) {
        if (!vehicle.wheels[wheel].grounded ||
            fabs(vehicle.wheels[wheel].contact.height - floor) > dd2_recovery_test_tolerance) {
            return false;
        }
    }
    /* A recovered car immediately has real tire support and can accelerate;
     * no hidden extra integration tick was used to build its wheel geometry. */
    for (unsigned tick = 0; tick < DD2_RECOVERY_TEST_DRIVE_STEPS; ++tick) {
        if (!dd2_vehicle_step(&vehicle, track->road, track->surface,
                              (dd2_vehicle_control){.throttle = 1})) {
            return false;
        }
    }
    return vehicle.steps == DD2_RECOVERY_TEST_STEPS + DD2_RECOVERY_TEST_DRIVE_STEPS &&
           vehicle.velocity.z > 0;
}

static bool dd2_recovery_test_npc(const dd2_recovery_test_track *track) {
    dd2_vehicle vehicles[DD2_RECOVERY_TEST_CARS] = {0};
    if (!dd2_vehicle_reset(&vehicles[0],
                           (dd2_vehicle_spawn){.position = {.y = DD2_RECOVERY_TEST_RIDE_HEIGHT}}) ||
        !dd2_vehicle_reset(&vehicles[1],
                           (dd2_vehicle_spawn){.position = {.x = DD2_RECOVERY_TEST_DISTANCE,
                                                            .y = DD2_RECOVERY_TEST_ROOF_HEIGHT}})) {
        return false;
    }
    vehicles[1].rotation = (dd2_vehicle_rotation){.z = 1};
    dd2_recovery_driver states[DD2_RECOVERY_TEST_CARS] = {0};
    dd2_vehicle_damage damage[DD2_RECOVERY_TEST_CARS] = {0};
    const dd2_recovery_frame frame = dd2_recovery_test_frame(track, damage, DD2_RECOVERY_TEST_CARS);
    for (unsigned tick = 0; tick <= DD2_RECOVERY_REST_STEPS; ++tick) {
        if (!dd2_recovery_step(states, vehicles, frame) || !states[1].overturned ||
            states[1].recoveries != 0) {
            return false;
        }
    }
    /* The exact original distance boundary blocks the NPC. Becoming distant
     * later must retry the elapsed deadline instead of missing tick 100 forever. */
    vehicles[0].position.x = -1;
    if (!dd2_recovery_step(states, vehicles, frame) || states[1].overturned ||
        states[1].recoveries != 1) {
        return false;
    }
    vehicles[1].position.y = DD2_RECOVERY_TEST_ROOF_HEIGHT;
    vehicles[1].rotation = (dd2_vehicle_rotation){.z = 1};
    damage[1].regions[0] = 1;
    damage[1].retired = true;
    for (unsigned tick = 0; tick <= DD2_RECOVERY_REST_STEPS; ++tick) {
        if (!dd2_recovery_step(states, vehicles, frame)) {
            return false;
        }
    }
    return states[1].rest_steps == DD2_RECOVERY_REST_STEPS && states[1].overturned &&
           states[1].recoveries == 1 && vehicles[1].rotation.z == 1 && damage[1].retired;
}

static bool dd2_recovery_test_rejection(const dd2_recovery_test_track *track) {
    dd2_vehicle vehicle = {0};
    if (!dd2_vehicle_reset(&vehicle,
                           (dd2_vehicle_spawn){.position = {.y = DD2_RECOVERY_TEST_AIR_HEIGHT}})) {
        return false;
    }
    vehicle.rotation = (dd2_vehicle_rotation){.z = 1};
    const dd2_vehicle_damage damage = {0};
    dd2_recovery_frame frame = dd2_recovery_test_frame(track, &damage, 1);
    dd2_recovery_driver state = {.rest_steps = 1, .overturned = true};
    if (!dd2_recovery_step(&state, &vehicle, frame) || state.rest_steps != 0 || state.overturned) {
        return false;
    }
    vehicle.position.y = DD2_RECOVERY_TEST_ROOF_HEIGHT;
    vehicle.angular_velocity.x = 1;
    if (!dd2_recovery_step(&state, &vehicle, frame) || state.overturned) {
        return false;
    }
    vehicle.angular_velocity.x = 0;
    vehicle.velocity.y = DD2_RECOVERY_TEST_AIR_HEIGHT;
    if (!dd2_recovery_step(&state, &vehicle, frame) || state.overturned) {
        return false;
    }
    vehicle.velocity.y = 0;
    state = (dd2_recovery_driver){
        .rest_steps = DD2_RECOVERY_REST_STEPS - 1, .recoveries = UINT64_MAX, .overturned = true};
    if (dd2_recovery_step(&state, &vehicle, frame) ||
        state.rest_steps != DD2_RECOVERY_REST_STEPS - 1 || vehicle.rotation.z != 1) {
        return false;
    }
    frame.count = DD2_VEHICLE_FLEET_LIMIT + 1;
    return !dd2_recovery_step(&state, &vehicle, frame) &&
           !dd2_recovery_step(NULL, &vehicle, frame) &&
           !dd2_vehicle_refresh_wheels(&vehicle, NULL, track->surface) && vehicle.rotation.z == 1;
}

int main(void) {
    dd2_recovery_test_track track = {0};
    bool passed = dd2_recovery_test_track_init(&track);
    for (unsigned pose = 0; pose < 3 && passed; ++pose) {
        passed = dd2_recovery_test_clock(&track, 0, pose) &&
                 dd2_recovery_test_clock(
                     &track, DD2_RECOVERY_TEST_TOP * DD2_RECOVERY_TEST_LEVEL_HEIGHT, pose);
    }
    if (!passed) {
        puts("Recovery track/landing fixture failed");
    }
    if (passed) {
        passed = dd2_recovery_test_npc(&track);
        if (!passed) {
            puts("Recovery NPC distance/retirement failed");
        }
    }
    if (passed) {
        passed = dd2_recovery_test_rejection(&track);
        if (!passed) {
            puts("Recovery rejection/rollback failed");
        }
    }
    dd2_road_surface_destroy(track.surface);
    dd2_road_destroy(track.road);
    puts(passed ? "supported overturns and source-timed recovery: PASS" : "recovery: FAIL");
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
