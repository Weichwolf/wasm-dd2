#include "accident_trace.h"
#include "archive_fixture.h"
#include "assets/archive.h"
#include "assets/barriers.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "game/accidents.h"
#include "game/driving.h"
#include "physics/barrier_world.h"
#include "physics/damage.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static dd2_vehicle_collision_storage *dd2_test_storage;

enum { DD2_IMPACT_CARS = DD2_VEHICLE_FLEET_LIMIT, DD2_IMPACT_LEVEL = 8, DD2_IMPACT_STEPS = 400 };
static const double dd2_impact_speed = 10000;
static const double dd2_impact_destruction_speed = 20000;
static const double dd2_impact_separation = 500;
static const double dd2_impact_offset = 200;
static const double dd2_impact_destruction_offset = 400;
static const double dd2_impact_heading_tolerance = 1e-10;

typedef struct {
    dd2_vehicle vehicles[DD2_IMPACT_CARS];
    dd2_vehicle_damage damages[DD2_IMPACT_CARS];
    dd2_accident_driver scores[DD2_IMPACT_CARS];
    dd2_vehicle_collision_report contacts;
} dd2_impact_field;

typedef struct {
    double speed;
    double offset;
} dd2_impact_setup;

static void dd2_impact_observe(const dd2_impact_field *field,
                               dd2_accident_observation *observations) {
    for (unsigned slot = 0; slot < DD2_IMPACT_CARS; ++slot) {
        const dd2_vehicle_vector forward =
            dd2_vehicle_rotate(field->vehicles[slot].rotation, (dd2_vehicle_vector){.z = 1});
        observations[slot] = (dd2_accident_observation){
            .heading = hypot(forward.x, forward.z) > dd2_impact_heading_tolerance
                           ? atan2(forward.x, forward.z)
                           : field->scores[slot].heading,
            .retired = field->damages[slot].retired};
    }
}

static void dd2_impact_write(const dd2_impact_field *field, unsigned step) {
    dd2_test_accident_write((dd2_test_accident_trace){.vehicles = field->vehicles,
                                                      .damages = field->damages,
                                                      .scores = field->scores,
                                                      .contacts = &field->contacts,
                                                      .count = DD2_IMPACT_CARS},
                            step);
}

static bool dd2_impact_step(dd2_impact_field *field, const dd2_road *road,
                            const dd2_road_surface *surface, const dd2_barrier_world *world) {
    dd2_vehicle previous[DD2_IMPACT_CARS] = {0};
    for (unsigned slot = 0; slot < DD2_IMPACT_CARS; ++slot) {
        previous[slot] = field->vehicles[slot];
        const dd2_vehicle_control control = dd2_damage_control(
            &field->damages[slot], (dd2_vehicle_control){.brake = slot < 2 ? 0 : 1});
        if (!dd2_vehicle_step(&field->vehicles[slot], road, surface, control)) {
            return false;
        }
    }
    if (!dd2_vehicle_collide_fleet_report(field->vehicles, previous, DD2_IMPACT_CARS, surface,
                                          world, dd2_test_storage, &field->contacts) ||
        !dd2_damage_step(field->damages, (dd2_damage_frame){.contacts = &field->contacts,
                                                            .count = DD2_IMPACT_CARS})) {
        return false;
    }
    dd2_accident_observation observations[DD2_IMPACT_CARS] = {0};
    dd2_impact_observe(field, observations);
    return dd2_accidents_step(field->scores, (dd2_accident_frame){.contacts = &field->contacts,
                                                                  .vehicles = observations,
                                                                  .count = DD2_IMPACT_CARS});
}

static bool dd2_impact_run(const dd2_driving *driving, const dd2_road *road,
                           const dd2_road_surface *surface, const dd2_barrier_world *world,
                           dd2_impact_setup setup) {
    dd2_impact_field field = {0};
    for (unsigned slot = 0; slot < DD2_IMPACT_CARS; ++slot) {
        field.vehicles[slot] = dd2_driving_vehicles(driving)[slot];
    }
    const dd2_vehicle anchor = field.vehicles[0];
    for (unsigned slot = 0; slot < 2; ++slot) {
        const double direction = slot == 0 ? 1 : -1;
        field.vehicles[slot] = anchor;
        field.vehicles[slot].position.x -= direction * dd2_impact_separation;
        field.vehicles[slot].position.z += direction * setup.offset;
        field.vehicles[slot].velocity.x = direction * setup.speed;
    }
    dd2_accident_observation observations[DD2_IMPACT_CARS] = {0};
    dd2_impact_observe(&field, observations);
    if (!dd2_accidents_reset(field.scores, observations, DD2_IMPACT_CARS)) {
        return false;
    }
    dd2_impact_write(&field, 0);
    for (unsigned step = 1; step <= DD2_IMPACT_STEPS; ++step) {
        if (!dd2_impact_step(&field, road, surface, world)) {
            printf("Rejected controlled impact step=%u\n", step);
            return false;
        }
        dd2_impact_write(&field, step);
    }
    /* Independent owned field reset; original data and the driving owner were
     * never modified by the controlled collision setup. */
    field = (dd2_impact_field){0};
    for (unsigned slot = 0; slot < DD2_IMPACT_CARS; ++slot) {
        field.vehicles[slot] = dd2_driving_vehicles(driving)[slot];
    }
    dd2_impact_observe(&field, observations);
    const bool reset = dd2_accidents_reset(field.scores, observations, DD2_IMPACT_CARS);
    dd2_impact_write(&field, 0);
    return reset;
}

static int dd2_test_run(int argc, char **argv) {
    if ((argc != 2 && argc != 3) || (argc == 3 && strcmp(argv[2], "destruction") != 0)) {
        return EXIT_FAILURE;
    }
    dd2_archive_fixture archive = {0};
    if (!dd2_archive_fixture_open(argv[1], &archive)) {
        return EXIT_FAILURE;
    }
    dd2_asset asset = {0};
    dd2_level_data level = {0};
    dd2_road *road = NULL;
    if (dd2_archive_find(archive.archive, "LEV8\\LEVEL.DAT", &asset) &&
        dd2_level_decode((dd2_byte_view){.data = asset.bytes, .size = asset.size}, &level)) {
        road = dd2_road_create(&level, DD2_ROAD_ARENA);
    }
    dd2_driving *driving = dd2_driving_create(road, DD2_IMPACT_LEVEL);
    dd2_road_surface *surface = dd2_road_surface_create(road);
    dd2_barriers *barriers = dd2_barriers_create(road, DD2_IMPACT_LEVEL);
    dd2_barrier_world *world = dd2_barrier_world_create(barriers);
    const dd2_impact_setup setup =
        argc == 2 ? (dd2_impact_setup){.speed = dd2_impact_speed, .offset = dd2_impact_offset}
                  : (dd2_impact_setup){.speed = dd2_impact_destruction_speed,
                                       .offset = dd2_impact_destruction_offset};
    const bool pass = driving != NULL && surface != NULL && world != NULL &&
                      dd2_impact_run(driving, road, surface, world, setup);
    dd2_barrier_world_destroy(world);
    dd2_barriers_destroy(barriers);
    dd2_road_surface_destroy(surface);
    dd2_driving_destroy(driving);
    dd2_road_destroy(road);
    dd2_archive_fixture_close(&archive);
    return pass ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char **argv) {
    dd2_test_storage = dd2_vehicle_collision_storage_create();
    if (dd2_test_storage == NULL) {
        return EXIT_FAILURE;
    }
    const int result = dd2_test_run(argc, argv);
    dd2_vehicle_collision_storage_destroy(dd2_test_storage);
    return result;
}
