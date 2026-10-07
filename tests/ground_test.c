#include "asset_fixture.h"
#include "assets/road.h"
#include "physics/collision_math.h"
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
    DD2_GROUND_TEST_STEPS = 2000,
    DD2_GROUND_TEST_LAYERS = 3,
    DD2_GROUND_TEST_CORNERS = 8,
    DD2_GROUND_TEST_ROOF = 4
};
static const double dd2_ground_test_tolerance = 1e-5;
static const double dd2_ground_test_clearance = 1e-4;
static const double dd2_ground_test_rotating_height = 335;
static const double dd2_ground_test_rotating_speed = -2000;
static const double dd2_ground_test_rotating_spin = 1;
static const double dd2_ground_test_body_length = 900;
static const double dd2_ground_test_rest_tolerance = 0.1;
static const double dd2_ground_test_fast_residual = 1;
static const double dd2_ground_test_body_height = 130;
static const double dd2_ground_test_drop_height = 1000;
static const double dd2_ground_test_fast_velocity = -200000;
static const double dd2_ground_test_slide_velocity = 1000;
static const double dd2_ground_test_overlap = 5;
static const double dd2_ground_test_probe_height = 50;
static const double dd2_ground_test_probe_span = 2000;
static const double dd2_ground_test_yaw = 0.3;
static const double dd2_ground_test_side = 0.7071067811865475244;
static const double dd2_ground_test_rest = 183.05555555555556;
static const double dd2_ground_test_top_time = 0.4;
static const double dd2_ground_test_bottom_time = 0.5;
static const double dd2_ground_test_rotation_height = 400;
static const double dd2_ground_test_angular_speed = 64;
static const double dd2_ground_test_half_rotation = 0.16;

static double dd2_ground_test_energy(const dd2_vehicle *vehicle) {
    const dd2_vehicle_rotation inverse = {.x = -vehicle->rotation.x,
                                          .y = -vehicle->rotation.y,
                                          .z = -vehicle->rotation.z,
                                          .w = vehicle->rotation.w};
    const dd2_vehicle_vector spin = dd2_vehicle_rotate(inverse, vehicle->angular_velocity);
    const double inertia[] = {((900.0 * 900.0) + (260.0 * 260.0)) / 12,
                              ((900.0 * 900.0) + (372.0 * 372.0)) / 12,
                              ((372.0 * 372.0) + (260.0 * 260.0)) / 12};
    return (vehicle->velocity.x * vehicle->velocity.x) +
           (vehicle->velocity.y * vehicle->velocity.y) +
           (vehicle->velocity.z * vehicle->velocity.z) + (inertia[0] * spin.x * spin.x) +
           (inertia[1] * spin.y * spin.y) + (inertia[2] * spin.z * spin.z);
}

typedef struct {
    dd2_road *road;
    dd2_road_surface *surface;
} dd2_ground_test_track;

static bool dd2_ground_test_init(dd2_ground_test_track *track, bool slope, bool large) {
    dd2_surface_test_fixture fixture;
    dd2_surface_test_fixture_init(&fixture, DD2_GROUND_TEST_LAYERS);
    for (unsigned layer = 0; layer < DD2_GROUND_TEST_LAYERS; ++layer) {
        for (unsigned corner = 0; corner < DD2_SURFACE_TEST_VERTICES; ++corner) {
            uint8_t *vertex =
                fixture.vertices + (((size_t)layer * DD2_SURFACE_TEST_VERTICES + corner) *
                                    DD2_SURFACE_TEST_VERTEX_BYTES);
            const int32_t side = large ? DD2_SURFACE_TEST_LONG_SIDE : DD2_SURFACE_TEST_SIDE;
            const int32_t xpos = ((int32_t)(corner % DD2_SURFACE_TEST_ROW_VERTICES) - 1) * side;
            const int32_t zpos = corner < DD2_SURFACE_TEST_ROW_VERTICES ? -side : side;
            const int32_t height =
                ((int32_t)layer * DD2_SURFACE_TEST_HEIGHT) + (slope ? zpos / 10 : 0);
            dd2_test_write_le32(vertex, (uint32_t)xpos);
            dd2_test_write_le32(vertex + DD2_TEST_WORD_BYTES, (uint32_t)height);
            dd2_test_write_le32(vertex + DD2_SURFACE_TEST_Z_OFFSET, (uint32_t)zpos);
        }
    }
    track->road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    track->surface = dd2_road_surface_create(track->road);
    return track->surface != NULL;
}

static void dd2_ground_test_destroy(dd2_ground_test_track *track) {
    dd2_road_surface_destroy(track->surface);
    dd2_road_destroy(track->road);
}

static bool dd2_ground_test_sweep(void) {
    dd2_ground_test_track track = {0};
    bool valid = dd2_ground_test_init(&track, false, false);
    dd2_surface_sweep sweep = {
        .start = {.x = -dd2_ground_test_probe_span, .y = dd2_ground_test_drop_height},
        .end = {.x = dd2_ground_test_probe_span, .y = -dd2_ground_test_drop_height}};
    dd2_surface_hit hit = {0};
    valid = valid && dd2_road_surface_sweep(track.surface, sweep, &hit, NULL) &&
            fabs(hit.time - dd2_ground_test_top_time) < dd2_ground_test_tolerance &&
            hit.road.height == 2 * DD2_SURFACE_TEST_HEIGHT && hit.penetration == 0;
    sweep = (dd2_surface_sweep){.start = {.y = dd2_ground_test_probe_height},
                                .end = {.y = -dd2_ground_test_probe_height},
                                .recovery = dd2_ground_test_overlap};
    valid = valid && dd2_road_surface_sweep(track.surface, sweep, &hit, NULL) &&
            fabs(hit.time - dd2_ground_test_bottom_time) < dd2_ground_test_tolerance &&
            hit.road.height == 0;
    sweep.start.y = -dd2_ground_test_probe_height;
    sweep.end.y = dd2_ground_test_probe_height;
    valid = valid && !dd2_road_surface_sweep(track.surface, sweep, &hit, NULL) && hit.time == 0;
    sweep.start.y = -dd2_ground_test_overlap;
    sweep.end.y = sweep.start.y;
    valid = valid && dd2_road_surface_sweep(track.surface, sweep, &hit, NULL) && hit.time == 0 &&
            hit.penetration == dd2_ground_test_overlap && hit.road.height == 0;
    const union {
        uint64_t bits;
        double number;
    } invalid = {.bits = UINT64_C(0x7ff8000000000001)};
    sweep.end.x = invalid.number;
    dd2_surface_statistics stats = {.cell_tests = 1};
    valid = valid && !dd2_road_surface_sweep(track.surface, sweep, &hit, &stats) &&
            hit.penetration == 0 && stats.cell_tests == 0;
    dd2_ground_test_destroy(&track);
    return valid;
}

static double dd2_ground_test_lowest(const dd2_vehicle *vehicle, double gradient) {
    double lowest = dd2_ground_test_drop_height;
    for (unsigned corner = 0; corner < DD2_GROUND_TEST_CORNERS; ++corner) {
        const dd2_vehicle_vector local = {.x = (corner & 1U) != 0 ? 186 : -186,
                                          .y = (corner & 2U) != 0 ? 130 : -130,
                                          .z = (corner & 4U) != 0 ? 450 : -450};
        const dd2_vehicle_vector offset = dd2_vehicle_rotate(vehicle->rotation, local);
        lowest = fmin(lowest, vehicle->position.y + offset.y -
                                  ((vehicle->position.z + offset.z) * gradient));
    }
    return lowest;
}

static bool dd2_ground_test_run(bool side, bool slope, bool sliding) {
    dd2_ground_test_track track = {0};
    bool valid = dd2_ground_test_init(&track, slope, true);
    dd2_vehicle vehicle = {0};
    valid = valid && dd2_vehicle_reset(&vehicle, (dd2_vehicle_spawn){
                                                     .position = {.y = dd2_ground_test_drop_height},
                                                     .yaw = dd2_ground_test_yaw});
    vehicle.rotation =
        side ? (dd2_vehicle_rotation){.z = dd2_ground_test_side, .w = dd2_ground_test_side}
             : (dd2_vehicle_rotation){.z = 1};
    vehicle.velocity.x = sliding ? dd2_ground_test_slide_velocity : 0;
    unsigned contacts = 0;
    const double gradient = slope ? 0.1 : 0;
    for (unsigned step = 0; valid && step < DD2_GROUND_TEST_STEPS; ++step) {
        const dd2_vehicle previous = vehicle;
        dd2_vehicle_impact impact = {0};
        valid = dd2_vehicle_step(&vehicle, track.road, track.surface, (dd2_vehicle_control){0}) &&
                dd2_vehicle_collide_world(&vehicle, &previous, track.surface, NULL, &impact);
        contacts += impact.contacts;
        if (valid && dd2_ground_test_lowest(&vehicle, gradient) <
                         (2 * DD2_SURFACE_TEST_HEIGHT) - dd2_ground_test_rest_tolerance) {
            printf("Ground penetration: step %u lowest %.17g\n", step,
                   dd2_ground_test_lowest(&vehicle, gradient));
            valid = false;
        }
    }
    printf("Ground rest side=%d slope=%d slide=%d y=%.17g v=%.17g,%.17g,%.17g "
           "omega=%.17g,%.17g,%.17g contacts=%u\n",
           (int)side, (int)slope, (int)sliding, vehicle.position.y, vehicle.velocity.x,
           vehicle.velocity.y, vehicle.velocity.z, vehicle.angular_velocity.x,
           vehicle.angular_velocity.y, vehicle.angular_velocity.z, contacts);
    valid = valid && contacts > 0;
    if (!slope && !side) {
        valid = valid &&
                fabs(vehicle.position.y - (2 * DD2_SURFACE_TEST_HEIGHT) -
                     dd2_ground_test_body_height) < dd2_ground_test_rest_tolerance &&
                fabs(vehicle.velocity.y) < dd2_ground_test_rest_tolerance &&
                fabs(vehicle.velocity.x) < dd2_ground_test_rest_tolerance;
    }
    dd2_ground_test_destroy(&track);
    return valid;
}

static bool dd2_ground_test_fast(void) {
    dd2_ground_test_track track = {0};
    bool valid = dd2_ground_test_init(&track, false, true);
    dd2_vehicle previous = {0};
    valid = valid &&
            dd2_vehicle_reset(&previous,
                              (dd2_vehicle_spawn){.position = {.y = dd2_ground_test_drop_height}});
    previous.rotation = (dd2_vehicle_rotation){.z = 1};
    previous.velocity.y = dd2_ground_test_fast_velocity;
    dd2_vehicle next = previous;
    next.position.y += next.velocity.y * DD2_VEHICLE_STEP_SECONDS;
    dd2_vehicle_impact impact = {0};
    valid = valid && dd2_vehicle_collide_world(&next, &previous, track.surface, NULL, &impact) &&
            impact.contacts > 0 &&
            dd2_ground_test_lowest(&next, 0) >=
                (2 * DD2_SURFACE_TEST_HEIGHT) - dd2_ground_test_tolerance &&
            fabs(next.velocity.y) < dd2_ground_test_fast_residual &&
            dd2_ground_test_energy(&next) <= dd2_ground_test_energy(&previous);
    if (!valid) {
        printf("Fast drop y=%.17g v=%.17g omega=%.17g,%.17g contacts=%u\n", next.position.y,
               next.velocity.y, next.angular_velocity.x, next.angular_velocity.z, impact.contacts);
    }
    const dd2_vehicle saved = next;
    const union {
        uint64_t bits;
        double number;
    } invalid = {.bits = UINT64_C(0x7ff0000000000000)};
    previous.position.x = invalid.number;
    valid = valid && !dd2_vehicle_collide_world(&next, &previous, track.surface, NULL, &impact) &&
            next.position.y == saved.position.y && next.velocity.y == saved.velocity.y &&
            impact.contacts == 0;
    dd2_ground_test_destroy(&track);
    return valid;
}

static bool dd2_ground_test_rotation(void) {
    dd2_ground_test_track track = {0};
    bool valid = dd2_ground_test_init(&track, false, true);
    dd2_vehicle previous = {0};
    valid = valid &&
            dd2_vehicle_reset(
                &previous, (dd2_vehicle_spawn){.position = {.y = dd2_ground_test_rotation_height}});
    previous.rotation = (dd2_vehicle_rotation){.z = 1};
    previous.angular_velocity.x = dd2_ground_test_angular_speed;
    dd2_vehicle next = previous;
    next.rotation = (dd2_vehicle_rotation){.y = -sin(dd2_ground_test_half_rotation),
                                           .z = cos(dd2_ground_test_half_rotation)};
    dd2_vehicle_impact impact = {0};
    valid = valid && dd2_vehicle_collide_world(&next, &previous, track.surface, NULL, &impact) &&
            impact.contacts > 0 &&
            dd2_ground_test_lowest(&next, 0) >=
                (2 * DD2_SURFACE_TEST_HEIGHT) - dd2_ground_test_rest_tolerance &&
            dd2_ground_test_energy(&next) <= dd2_ground_test_energy(&previous);
    if (!valid) {
        printf("Rotation ground y=%.17g lowest=%.17g contacts=%u\n", next.position.y,
               dd2_ground_test_lowest(&next, 0), impact.contacts);
    }
    dd2_ground_test_destroy(&track);
    return valid;
}

static bool dd2_ground_test_report(void) {
    dd2_ground_test_track track = {0};
    bool valid = dd2_ground_test_init(&track, false, true);
    dd2_vehicle previous = {0};
    valid = valid &&
            dd2_vehicle_reset(&previous,
                              (dd2_vehicle_spawn){.position = {.y = dd2_ground_test_drop_height}});
    previous.rotation = (dd2_vehicle_rotation){.z = 1};
    previous.velocity.y = dd2_ground_test_fast_velocity;
    dd2_vehicle next = previous;
    next.position.y += next.velocity.y * DD2_VEHICLE_STEP_SECONDS;
    dd2_vehicle_collision_report report = {0};
    valid = valid &&
            dd2_vehicle_collide_fleet_report(&next, &previous, 1, track.surface, NULL, &report) &&
            report.count > 0 && report.pair_contacts == 0 &&
            report.impacts[0].contacts == report.count;
    const dd2_vehicle_contact contact = report.contacts[0];
    const double height = 2 * DD2_SURFACE_TEST_HEIGHT;
    const double time = (dd2_ground_test_drop_height - height - dd2_ground_test_body_height) /
                        (-dd2_ground_test_fast_velocity * DD2_VEHICLE_STEP_SECONDS);
    valid =
        valid && contact.kind == DD2_VEHICLE_CONTACT_GROUND && contact.first == 0 &&
        contact.second == DD2_VEHICLE_NO_PARTNER &&
        contact.obstacle < dd2_road_cell_count(track.road) &&
        fabs(contact.time - time) < dd2_ground_test_tolerance &&
        fabs(contact.point.y - height) < dd2_ground_test_tolerance &&
        fabs(contact.local_points[0].y - dd2_ground_test_body_height) < dd2_ground_test_tolerance &&
        contact.local_points[1].x == 0 && contact.local_points[1].y == 0 &&
        contact.local_points[1].z == 0 && contact.normal.y == 1 &&
        contact.normal_speed == -dd2_ground_test_fast_velocity && contact.impulse > 0;
    previous.position.y = height + dd2_ground_test_body_height - dd2_ground_test_overlap;
    previous.velocity.y = 0;
    next = previous;
    valid = valid &&
            dd2_vehicle_collide_fleet_report(&next, &previous, 1, track.surface, NULL, &report) &&
            report.count == DD2_GROUND_TEST_ROOF && report.pair_contacts == 0 &&
            report.impacts[0].contacts == DD2_GROUND_TEST_ROOF &&
            fabs(next.position.y - height - dd2_ground_test_body_height -
                 dd2_ground_test_clearance) < dd2_ground_test_tolerance &&
            next.position.x == previous.position.x && next.position.z == previous.position.z &&
            next.steps == previous.steps && dd2_ground_test_energy(&next) == 0 &&
            next.rotation.z == previous.rotation.z && next.rotation.w == previous.rotation.w;
    for (unsigned index = 0; valid && index < report.count; ++index) {
        const dd2_vehicle_contact support = report.contacts[index];
        valid = support.kind == DD2_VEHICLE_CONTACT_GROUND && support.first == 0 &&
                support.second == DD2_VEHICLE_NO_PARTNER &&
                support.obstacle < dd2_road_cell_count(track.road) && support.time == 0 &&
                support.normal_speed == 0 && support.impulse == 0 && support.normal.y == 1 &&
                fabs(support.point.y - height) < dd2_ground_test_tolerance &&
                fabs(support.local_points[0].y - dd2_ground_test_body_height +
                     dd2_ground_test_overlap) < dd2_ground_test_tolerance;
        for (unsigned earlier = 0; valid && earlier < index; ++earlier) {
            const dd2_vehicle_vector other = report.contacts[earlier].point;
            valid = support.point.x != other.x || support.point.z != other.z;
        }
    }
    dd2_ground_test_destroy(&track);
    return valid;
}

/* The swept corner chord and the instantaneous rotated corner differ here
 * by more than the clearance. They must remain one physical support. */
static bool dd2_ground_test_fleet_rotation(void) {
    dd2_ground_test_track track = {0};
    bool valid = dd2_ground_test_init(&track, false, true);
    dd2_vehicle previous = {0};
    valid = valid &&
            dd2_vehicle_reset(
                &previous, (dd2_vehicle_spawn){.position = {.y = dd2_ground_test_rotating_height}});
    previous.rotation = (dd2_vehicle_rotation){.z = 1};
    previous.velocity.y = dd2_ground_test_rotating_speed;
    previous.angular_velocity.z = dd2_ground_test_rotating_spin;
    dd2_vehicle next = previous;
    next.position.y += next.velocity.y * DD2_VEHICLE_STEP_SECONDS;
    next.rotation = dd2_collision_turn(&next, DD2_VEHICLE_STEP_SECONDS);
    dd2_vehicle_collision_report report = {0};
    valid = valid &&
            dd2_vehicle_collide_fleet_report(&next, &previous, 1, track.surface, NULL, &report) &&
            report.unresolved_sweeps == 0 && report.count > 2 &&
            report.count < DD2_VEHICLE_CONTACT_LIMIT && report.contacts[0].time > 0 &&
            report.contacts[0].time < 1 && report.contacts[1].time == report.contacts[0].time &&
            report.contacts[2].time > report.contacts[0].time &&
            fabs(fabs(report.contacts[0].point.z - report.contacts[1].point.z) -
                 dd2_ground_test_body_length) < dd2_ground_test_tolerance &&
            dd2_ground_test_lowest(&next, 0) >=
                (2 * DD2_SURFACE_TEST_HEIGHT) - dd2_ground_test_tolerance &&
            dd2_ground_test_energy(&next) <= dd2_ground_test_energy(&previous) &&
            next.steps == previous.steps;
    dd2_ground_test_destroy(&track);
    return valid;
}

static bool dd2_ground_test_upright(void) {
    dd2_ground_test_track track = {0};
    bool valid = dd2_ground_test_init(&track, false, true);
    dd2_vehicle vehicle = {0};
    valid = valid && dd2_vehicle_reset(
                         &vehicle, (dd2_vehicle_spawn){.position = {.y = dd2_ground_test_rest}});
    for (unsigned step = 0; valid && step < DD2_GROUND_TEST_STEPS; ++step) {
        const dd2_vehicle previous = vehicle;
        dd2_vehicle_impact impact = {0};
        valid = dd2_vehicle_step(&vehicle, track.road, track.surface, (dd2_vehicle_control){0}) &&
                dd2_vehicle_collide_world(&vehicle, &previous, track.surface, NULL, &impact) &&
                impact.contacts == 0;
    }
    valid = valid && fabs(vehicle.position.y - dd2_ground_test_rest) < dd2_ground_test_tolerance;
    dd2_ground_test_destroy(&track);
    return valid;
}

int main(void) {
    if (!dd2_ground_test_sweep() || !dd2_ground_test_run(false, false, false) ||
        !dd2_ground_test_run(false, false, true) || !dd2_ground_test_run(true, false, false) ||
        !dd2_ground_test_run(false, true, true) || !dd2_ground_test_fast() ||
        !dd2_ground_test_rotation() || !dd2_ground_test_upright() || !dd2_ground_test_report() ||
        !dd2_ground_test_fleet_rotation()) {
        puts("body ground contacts: FAIL");
        return EXIT_FAILURE;
    }
    puts("body ground contacts: PASS");
    return EXIT_SUCCESS;
}
