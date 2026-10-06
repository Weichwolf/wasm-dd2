#include "assets/barriers.h"
#include "assets/road.h"
#include "physics/barrier_world.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"
#include "surface_fixture.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { DD2_BARRIER_TEST_WALL = 1000, DD2_BARRIER_TEST_RADIUS = 100, DD2_BARRIER_TEST_PROBES = 5 };
static const double dd2_barrier_test_distant_start = 2147483000;
static const double dd2_barrier_test_hit_time = 0.45;
static const double dd2_barrier_test_tolerance = 1e-6;
static const double dd2_barrier_test_yaw_inertia = ((900.0 * 900.0) + (372.0 * 372.0)) / 12.0;
static const double dd2_barrier_test_body_radius = 186;
static const double dd2_barrier_test_body_length = 264;
static const double dd2_barrier_test_quarter_turn = 1.57079632679489661923;
static const double dd2_barrier_test_fast_speed = 200000;
static const double dd2_barrier_test_side_speed = 20000;
static const double dd2_barrier_test_contact_center = 814;
static const double dd2_barrier_test_outside_center = 950;
static const double dd2_barrier_test_rotation_center = 770;
static const double dd2_barrier_test_max_rotation = 0.32;
static const double dd2_barrier_test_half_rotation = 0.16;
static const double dd2_barrier_test_max_angular = 64;
static const double dd2_barrier_test_bridge_height = 1000;
static const double dd2_barrier_test_height = 190;
static const double dd2_barrier_test_front_time = 0.55;
static const double dd2_barrier_test_front_arm = 450;
static const double dd2_barrier_test_impulse = 240000;

static bool dd2_barrier_test_query(const dd2_barrier_world *world) {
    dd2_barrier_contact contact = {0};
    const dd2_barrier_sweep sweep = {
        .start = {.y = dd2_barrier_test_height},
        .end = {.x = DD2_BARRIER_TEST_WALL * 2, .y = dd2_barrier_test_height},
        .radius = DD2_BARRIER_TEST_RADIUS,
        .half_height = DD2_BARRIER_TEST_RADIUS};
    if (!dd2_barrier_world_sweep(world, sweep, &contact, NULL) ||
        fabs(contact.time - dd2_barrier_test_hit_time) > dd2_barrier_test_tolerance ||
        contact.normal.x != -1 || contact.penetration != 0 ||
        fabs(contact.point.x - (double)DD2_BARRIER_TEST_WALL) > dd2_barrier_test_tolerance) {
        puts("swept wall query failed");
        return false;
    }
    const dd2_barrier_sweep distant = {
        .start = {.x = -dd2_barrier_test_distant_start,
                  .y = dd2_barrier_test_height,
                  .z = DD2_BARRIER_TEST_WALL + DD2_BARRIER_TEST_RADIUS},
        .end = {.x = dd2_barrier_test_distant_start,
                .y = dd2_barrier_test_height,
                .z = DD2_BARRIER_TEST_WALL + DD2_BARRIER_TEST_RADIUS},
        .radius = DD2_BARRIER_TEST_RADIUS,
        .half_height = DD2_BARRIER_TEST_RADIUS};
    if (!dd2_barrier_world_sweep(world, distant, &contact, NULL) ||
        fabs(contact.normal.x) > dd2_barrier_test_tolerance ||
        fabs(contact.normal.z - 1) > dd2_barrier_test_tolerance) {
        puts("distant endpoint tangency failed");
        return false;
    }
    dd2_barrier_sweep elevated = sweep;
    elevated.start.y = dd2_barrier_test_bridge_height;
    elevated.end.y = elevated.start.y;
    if (dd2_barrier_world_sweep(world, elevated, &contact, NULL)) {
        puts("wrong bridge level hit");
        return false;
    }
    const union {
        uint64_t bits;
        double number;
    } invalid = {.bits = UINT64_C(0x7ff8000000000001)};
    elevated.radius = invalid.number;
    return !dd2_barrier_world_sweep(world, elevated, &contact, NULL) && contact.time == 0;
}

static bool dd2_barrier_test_inside(const dd2_vehicle *vehicle) {
    for (unsigned probe = 0; probe < DD2_BARRIER_TEST_PROBES; ++probe) {
        const double local_z =
            -dd2_barrier_test_body_length + ((double)probe * dd2_barrier_test_body_length / 2);
        const dd2_vehicle_vector rotated =
            dd2_vehicle_rotate(vehicle->rotation, (dd2_vehicle_vector){.z = local_z});
        if (fabs(vehicle->position.x + rotated.x) + dd2_barrier_test_body_radius >
            (double)DD2_BARRIER_TEST_WALL + dd2_barrier_test_tolerance) {
            return false;
        }
    }
    return true;
}

static bool dd2_barrier_test_response(const dd2_barrier_world *world, bool glancing) {
    dd2_vehicle previous = {0};
    if (!dd2_vehicle_reset(&previous,
                           (dd2_vehicle_spawn){.position = {.y = dd2_barrier_test_height},
                                               .yaw = dd2_barrier_test_quarter_turn})) {
        return false;
    }
    dd2_vehicle vehicle = previous;
    vehicle.velocity = (dd2_vehicle_vector){.x = dd2_barrier_test_fast_speed,
                                            .z = glancing ? dd2_barrier_test_side_speed : 0};
    vehicle.position.x += vehicle.velocity.x * DD2_VEHICLE_STEP_SECONDS;
    vehicle.position.z += vehicle.velocity.z * DD2_VEHICLE_STEP_SECONDS;
    dd2_vehicle_impact impact = {0};
    const double initial_energy =
        (vehicle.velocity.x * vehicle.velocity.x) + (vehicle.velocity.z * vehicle.velocity.z);
    if (!dd2_vehicle_collide_barriers(&vehicle, &previous, world, &impact) ||
        impact.contacts == 0 || impact.normal_speed <= 0 || vehicle.velocity.x >= 0 ||
        !dd2_barrier_test_inside(&vehicle) ||
        (vehicle.velocity.x * vehicle.velocity.x) + (vehicle.velocity.z * vehicle.velocity.z) +
                (dd2_barrier_test_yaw_inertia * vehicle.angular_velocity.y *
                 vehicle.angular_velocity.y) >
            initial_energy) {
        printf("wall response failed: glancing=%d contacts=%u position=%.17g velocity=%.17g\n",
               (int)glancing, impact.contacts, vehicle.position.x, vehicle.velocity.x);
        return false;
    }
    const dd2_vehicle saved = vehicle;
    const union {
        uint64_t bits;
        double number;
    } invalid = {.bits = UINT64_C(0x7ff0000000000000)};
    previous.position.y = invalid.number;
    if (dd2_vehicle_collide_barriers(&vehicle, &previous, world, &impact) || impact.contacts != 0 ||
        vehicle.position.x != saved.position.x || vehicle.position.y != saved.position.y ||
        vehicle.position.z != saved.position.z || vehicle.velocity.x != saved.velocity.x ||
        vehicle.velocity.z != saved.velocity.z || vehicle.rotation.y != saved.rotation.y ||
        vehicle.rotation.w != saved.rotation.w) {
        puts("invalid body collision state did not roll back");
        return false;
    }
    return !glancing || vehicle.velocity.z > 0;
}

static bool dd2_barrier_test_overlap(const dd2_barrier_world *world) {
    dd2_vehicle previous = {0};
    if (!dd2_vehicle_reset(&previous,
                           (dd2_vehicle_spawn){.position = {.x = dd2_barrier_test_outside_center,
                                                            .y = dd2_barrier_test_height}})) {
        return false;
    }
    dd2_vehicle vehicle = previous;
    dd2_vehicle_impact impact = {0};
    if (!dd2_vehicle_collide_barriers(&vehicle, &previous, world, &impact) ||
        impact.contacts == 0 || !dd2_barrier_test_inside(&vehicle) || vehicle.velocity.x != 0) {
        puts("stationary overlap correction failed");
        return false;
    }
    if (!dd2_vehicle_reset(&previous,
                           (dd2_vehicle_spawn){.position = {.x = dd2_barrier_test_contact_center,
                                                            .y = dd2_barrier_test_height}})) {
        return false;
    }
    vehicle = previous;
    vehicle.position.x -= 1;
    vehicle.velocity.x = -1;
    return dd2_vehicle_collide_barriers(&vehicle, &previous, world, &impact) &&
           impact.contacts == 0;
}

static bool dd2_barrier_test_rotation(const dd2_barrier_world *world) {
    dd2_vehicle previous = {0};
    if (!dd2_vehicle_reset(&previous,
                           (dd2_vehicle_spawn){.position = {.x = dd2_barrier_test_rotation_center,
                                                            .y = dd2_barrier_test_height}})) {
        return false;
    }
    dd2_vehicle vehicle = previous;
    vehicle.rotation = (dd2_vehicle_rotation){.y = sin(dd2_barrier_test_half_rotation),
                                              .w = cos(dd2_barrier_test_half_rotation)};
    vehicle.angular_velocity.y = dd2_barrier_test_max_angular;
    dd2_vehicle_impact impact = {0};
    const bool valid = dd2_vehicle_collide_barriers(&vehicle, &previous, world, &impact) &&
                       impact.contacts != 0 && dd2_barrier_test_inside(&vehicle) &&
                       fabs(vehicle.angular_velocity.y) < dd2_barrier_test_max_angular;
    if (!valid) {
        printf("rotating body failed: contacts=%u yaw velocity=%.17g angle=%.17g\n",
               impact.contacts, vehicle.angular_velocity.y, dd2_barrier_test_max_rotation);
    }
    return valid;
}

static bool dd2_barrier_test_report(const dd2_barrier_world *world) {
    dd2_vehicle previous = {0};
    if (!dd2_vehicle_reset(&previous,
                           (dd2_vehicle_spawn){.position = {.y = dd2_barrier_test_height},
                                               .yaw = dd2_barrier_test_quarter_turn})) {
        return false;
    }
    previous.velocity.x = dd2_barrier_test_fast_speed;
    dd2_vehicle next = previous;
    next.position.x += previous.velocity.x * DD2_VEHICLE_STEP_SECONDS;
    dd2_vehicle_collision_report report = {0};
    if (!dd2_vehicle_collide_fleet_report(&next, &previous, 1, NULL, world, &report) ||
        report.count != 1 || report.pair_contacts != 0 || report.impacts[0].contacts != 1) {
        printf("Report wall count=%u pairs=%u impacts=%u\n", report.count, report.pair_contacts,
               report.impacts[0].contacts);
        return false;
    }
    const dd2_vehicle_contact contact = report.contacts[0];
    /* The fixture's second boundary is the wall at positive X. */
    return contact.kind == DD2_VEHICLE_CONTACT_BARRIER && contact.first == 0 &&
           contact.second == DD2_VEHICLE_NO_PARTNER && contact.obstacle == 1 &&
           fabs(contact.time - dd2_barrier_test_front_time) < dd2_barrier_test_tolerance &&
           fabs(contact.point.x - (double)DD2_BARRIER_TEST_WALL) < dd2_barrier_test_tolerance &&
           contact.normal.x == -1 && contact.normal_speed == dd2_barrier_test_fast_speed &&
           fabs(contact.impulse - dd2_barrier_test_impulse) < dd2_barrier_test_tolerance &&
           fabs(contact.local_points[0].x) < dd2_barrier_test_tolerance &&
           fabs(contact.local_points[0].z - dd2_barrier_test_front_arm) <
               dd2_barrier_test_tolerance &&
           contact.local_points[1].x == 0 && contact.local_points[1].y == 0 &&
           contact.local_points[1].z == 0;
}

int main(void) {
    dd2_surface_test_fixture fixture = {0};
    dd2_surface_test_fixture_init(&fixture, 1);
    dd2_road *road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    dd2_barriers *barriers = dd2_barriers_create(road, 1);
    dd2_barrier_world *world = dd2_barrier_world_create(barriers);
    bool passed = world != NULL && dd2_barriers_count(barriers) == 2 &&
                  dd2_barrier_test_query(world) && dd2_barrier_test_response(world, false) &&
                  dd2_barrier_test_response(world, true) && dd2_barrier_test_overlap(world) &&
                  dd2_barrier_test_rotation(world) && dd2_barrier_test_report(world);
    dd2_barrier_world_destroy(world);
    dd2_barriers_destroy(barriers);
    dd2_road_destroy(road);
    /* Arena creation borrows no road geometry after copying its radius. Use the
     * existing arena fixture through the driving owner tests; circle export
     * additionally checks all four original radii and swept boundary events. */
    puts(passed ? "barrier sweeps/body response: PASS" : "barrier sweeps/body response: FAIL");
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
