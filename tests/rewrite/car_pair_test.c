#include "physics/car_contact.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { DD2_PAIR_TEST_BODIES = 2, DD2_PAIR_TEST_CHAIN = 3 };
static const double dd2_pair_test_height = 5000;
static const double dd2_pair_test_distance = 1000;
static const double dd2_pair_test_fast = 200000;
static const double dd2_pair_test_side_time = 0.814;
static const double dd2_pair_test_front_time = 0.55;
static const double dd2_pair_test_rebound = 40000;
static const double dd2_pair_test_tolerance = 1e-6;
static const double dd2_pair_test_bridge = 300;
static const double dd2_pair_test_glance = 200;
static const double dd2_pair_test_near = 500;
static const double dd2_pair_test_speed = 20000;
static const double dd2_pair_test_max_angular = 64;
static const double dd2_pair_test_half_angle = 0.16;
static const double dd2_pair_test_chain_spacing = 950;
static const double dd2_pair_test_long_step = 400;
static const double dd2_pair_test_tiny_tilt = 1e-12;
static const double dd2_pair_test_impulse = 240000;
static const double dd2_pair_test_quarter_axis = 0.7071067811865475244;
static const double dd2_pair_test_front_arm = 450;

static double dd2_pair_test_energy(const dd2_vehicle *vehicle) {
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
static bool dd2_pair_test_reset(dd2_vehicle *vehicle, double position) {
    return dd2_vehicle_reset(
        vehicle, (dd2_vehicle_spawn){.position = {.y = dd2_pair_test_height, .z = position}});
}
static void dd2_pair_test_motion(const dd2_vehicle *previous, dd2_vehicle *next, unsigned count) {
    for (unsigned body = 0; body < count; ++body) {
        next[body] = previous[body];
        next[body].position.x += previous[body].velocity.x * DD2_VEHICLE_STEP_SECONDS;
        next[body].position.y += previous[body].velocity.y * DD2_VEHICLE_STEP_SECONDS;
        next[body].position.z += previous[body].velocity.z * DD2_VEHICLE_STEP_SECONDS;
    }
}
static bool dd2_pair_test_head_on(bool side) {
    dd2_vehicle previous[DD2_PAIR_TEST_BODIES] = {0};
    bool valid = dd2_pair_test_reset(&previous[0], -dd2_pair_test_distance) &&
                 dd2_pair_test_reset(&previous[1], dd2_pair_test_distance);
    previous[0].velocity.z = dd2_pair_test_fast;
    previous[1].velocity.z = -dd2_pair_test_fast;
    if (side) {
        for (unsigned body = 0; body < DD2_PAIR_TEST_BODIES; ++body) {
            previous[body].position.x = previous[body].position.z;
            previous[body].position.z = 0;
            previous[body].velocity.x = previous[body].velocity.z;
            previous[body].velocity.z = 0;
        }
    }
    dd2_vehicle next[DD2_PAIR_TEST_BODIES] = {0};
    dd2_pair_test_motion(previous, next, DD2_PAIR_TEST_BODIES);
    dd2_car_contact contact = {0};
    valid = valid &&
            dd2_car_contact_sweep(&previous[0], &next[0], &previous[1], &next[1], &contact) &&
            fabs(contact.time - (side ? dd2_pair_test_side_time : dd2_pair_test_front_time)) <
                dd2_pair_test_tolerance &&
            fabs(contact.point.y - dd2_pair_test_height) < dd2_pair_test_tolerance;
    dd2_vehicle_collision_report report = {0};
    valid = valid &&
            dd2_vehicle_collide_fleet_report(next, previous, DD2_PAIR_TEST_BODIES, NULL, NULL,
                                             &report) &&
            report.count == 1 && report.pair_contacts == 1 &&
            report.impacts[0].pair_contacts == 1 && report.impacts[1].pair_contacts == 1;
    const dd2_vehicle_contact recorded = report.contacts[0];
    const double first_point = side ? recorded.local_points[0].x : recorded.local_points[0].z;
    const double second_point = side ? recorded.local_points[1].x : recorded.local_points[1].z;
    const double expected_arm = side ? 186 : 450;
    valid = valid && recorded.kind == DD2_VEHICLE_CONTACT_PAIR && recorded.first == 0 &&
            recorded.second == 1 && recorded.obstacle == UINT32_MAX &&
            fabs(recorded.time - contact.time) < dd2_pair_test_tolerance &&
            fabs(recorded.point.y - dd2_pair_test_height) < dd2_pair_test_tolerance &&
            fabs(recorded.normal_speed - (2 * dd2_pair_test_fast)) < dd2_pair_test_tolerance &&
            fabs(recorded.impulse - dd2_pair_test_impulse) < dd2_pair_test_tolerance &&
            fabs(first_point - expected_arm) < dd2_pair_test_tolerance &&
            fabs(second_point + expected_arm) < dd2_pair_test_tolerance;
    const double first_velocity = side ? next[0].velocity.x : next[0].velocity.z;
    const double second_velocity = side ? next[1].velocity.x : next[1].velocity.z;
    valid = valid && fabs(first_velocity + dd2_pair_test_rebound) < dd2_pair_test_tolerance &&
            fabs(second_velocity - dd2_pair_test_rebound) < dd2_pair_test_tolerance &&
            dd2_pair_test_energy(&next[0]) + dd2_pair_test_energy(&next[1]) <=
                dd2_pair_test_energy(&previous[0]) + dd2_pair_test_energy(&previous[1]);
    valid = valid && !dd2_car_contact_sweep(&next[0], &next[0], &next[1], &next[1], &contact);
    if (!valid) {
        printf("Head-on side=%d pairs=%u v=%.17g,%.17g\n", (int)side, report.pair_contacts,
               first_velocity, second_velocity);
    }
    return valid;
}
static bool dd2_pair_test_glancing(void) {
    dd2_vehicle previous[DD2_PAIR_TEST_BODIES] = {0};
    bool valid = dd2_pair_test_reset(&previous[0], -dd2_pair_test_near) &&
                 dd2_pair_test_reset(&previous[1], dd2_pair_test_near);
    previous[1].position.x = dd2_pair_test_glance;
    previous[0].velocity.z = dd2_pair_test_speed;
    previous[1].velocity.z = -dd2_pair_test_speed;
    dd2_vehicle next[DD2_PAIR_TEST_BODIES] = {0};
    dd2_pair_test_motion(previous, next, DD2_PAIR_TEST_BODIES);
    unsigned pairs = 0;
    valid =
        valid &&
        dd2_vehicle_collide_fleet(next, previous, DD2_PAIR_TEST_BODIES, NULL, NULL, NULL, &pairs) &&
        pairs > 0 && fabs(next[0].angular_velocity.y) > dd2_pair_test_tolerance &&
        fabs(next[1].angular_velocity.y) > dd2_pair_test_tolerance &&
        fabs(next[0].velocity.z + next[1].velocity.z) < dd2_pair_test_tolerance &&
        dd2_pair_test_energy(&next[0]) + dd2_pair_test_energy(&next[1]) <=
            dd2_pair_test_energy(&previous[0]) + dd2_pair_test_energy(&previous[1]);
    if (!valid) {
        printf("Glancing pairs=%u omega=%.17g,%.17g\n", pairs, next[0].angular_velocity.y,
               next[1].angular_velocity.y);
    }
    return valid;
}
static bool dd2_pair_test_rotated_report(void) {
    dd2_vehicle previous[DD2_PAIR_TEST_BODIES] = {0};
    for (unsigned slot = 0; slot < DD2_PAIR_TEST_BODIES; ++slot) {
        if (!dd2_pair_test_reset(&previous[slot], 0)) {
            return false;
        }
        const double direction = slot == 0 ? -1 : 1;
        previous[slot].position.x = direction * dd2_pair_test_distance;
        previous[slot].velocity.x = -direction * dd2_pair_test_fast;
        previous[slot].rotation = (dd2_vehicle_rotation){.y = dd2_pair_test_quarter_axis,
                                                         .w = dd2_pair_test_quarter_axis};
    }
    dd2_vehicle next[DD2_PAIR_TEST_BODIES] = {0};
    dd2_pair_test_motion(previous, next, DD2_PAIR_TEST_BODIES);
    dd2_vehicle_collision_report report = {0};
    return dd2_vehicle_collide_fleet_report(next, previous, DD2_PAIR_TEST_BODIES, NULL, NULL,
                                            &report) &&
           report.count == 1 &&
           fabs(report.contacts[0].time - dd2_pair_test_front_time) < dd2_pair_test_tolerance &&
           fabs(report.contacts[0].local_points[0].x) < dd2_pair_test_tolerance &&
           fabs(report.contacts[0].local_points[1].x) < dd2_pair_test_tolerance &&
           fabs(report.contacts[0].local_points[0].z - dd2_pair_test_front_arm) <
               dd2_pair_test_tolerance &&
           fabs(report.contacts[0].local_points[1].z + dd2_pair_test_front_arm) <
               dd2_pair_test_tolerance;
}
static bool dd2_pair_test_contact_continuity(void) {
    for (unsigned sample = 0; sample < 3; ++sample) {
        dd2_vehicle previous[DD2_PAIR_TEST_BODIES] = {0};
        if (!dd2_pair_test_reset(&previous[0], -dd2_pair_test_near) ||
            !dd2_pair_test_reset(&previous[1], dd2_pair_test_near)) {
            return false;
        }
        previous[1].position.x = dd2_pair_test_glance;
        previous[1].rotation.z = ((double)sample - 1) * dd2_pair_test_tiny_tilt;
        previous[0].velocity.z = dd2_pair_test_speed;
        previous[1].velocity.z = -dd2_pair_test_speed;
        dd2_vehicle next[DD2_PAIR_TEST_BODIES] = {0};
        dd2_pair_test_motion(previous, next, DD2_PAIR_TEST_BODIES);
        dd2_car_contact contact = {0};
        if (!dd2_car_contact_sweep(&previous[0], &next[0], &previous[1], &next[1], &contact) ||
            fabs(contact.time - (1.0 / 2)) > dd2_pair_test_tolerance ||
            fabs(contact.point.x - (dd2_pair_test_glance / 2)) > dd2_pair_test_tolerance ||
            fabs(contact.point.y - dd2_pair_test_height) > dd2_pair_test_tolerance ||
            fabs(contact.point.z) > dd2_pair_test_tolerance) {
            printf("Tiny tilt sample=%u point=%.17g,%.17g,%.17g\n", sample, contact.point.x,
                   contact.point.y, contact.point.z);
            return false;
        }
    }
    return true;
}
static bool dd2_pair_test_rotation(void) {
    dd2_vehicle previous[DD2_PAIR_TEST_BODIES] = {0};
    bool valid = dd2_pair_test_reset(&previous[0], 0) && dd2_pair_test_reset(&previous[1], 0);
    previous[1].position.x = dd2_pair_test_near;
    previous[0].angular_velocity.y = dd2_pair_test_max_angular;
    dd2_vehicle next[DD2_PAIR_TEST_BODIES] = {previous[0], previous[1]};
    next[0].rotation = (dd2_vehicle_rotation){.y = sin(dd2_pair_test_half_angle),
                                              .w = cos(dd2_pair_test_half_angle)};
    unsigned pairs = 0;
    valid =
        valid &&
        dd2_vehicle_collide_fleet(next, previous, DD2_PAIR_TEST_BODIES, NULL, NULL, NULL, &pairs) &&
        pairs > 0 &&
        dd2_pair_test_energy(&next[0]) + dd2_pair_test_energy(&next[1]) <=
            dd2_pair_test_energy(&previous[0]) + dd2_pair_test_energy(&previous[1]);
    if (!valid) {
        printf("Rotation pairs=%u omega=%.17g,%.17g\n", pairs, next[0].angular_velocity.y,
               next[1].angular_velocity.y);
    }
    return valid;
}
static bool dd2_pair_test_chain(void) {
    dd2_vehicle previous[DD2_PAIR_TEST_CHAIN] = {0};
    bool valid = true;
    for (unsigned body = 0; body < DD2_PAIR_TEST_CHAIN; ++body) {
        valid = dd2_pair_test_reset(&previous[body], (double)body * dd2_pair_test_chain_spacing) &&
                valid;
    }
    previous[0].velocity.z = dd2_pair_test_long_step / DD2_VEHICLE_STEP_SECONDS;
    dd2_vehicle next[DD2_PAIR_TEST_CHAIN] = {0};
    dd2_pair_test_motion(previous, next, DD2_PAIR_TEST_CHAIN);
    dd2_vehicle_collision_report report = {0};
    valid = valid &&
            dd2_vehicle_collide_fleet_report(next, previous, DD2_PAIR_TEST_CHAIN, NULL, NULL,
                                             &report) &&
            report.pair_contacts >= 2 && report.count == report.pair_contacts &&
            next[2].velocity.z > 0;
    const double first_time = 1.0 / 8;
    const double second_time = first_time + (1.0 / 4.8);
    valid = valid && report.contacts[0].first == 0 && report.contacts[0].second == 1 &&
            report.contacts[1].first == 1 && report.contacts[1].second == 2 &&
            fabs(report.contacts[0].time - first_time) < dd2_pair_test_tolerance &&
            fabs(report.contacts[1].time - second_time) < dd2_pair_test_tolerance;
    for (unsigned event = 1; event < report.count; ++event) {
        valid = valid && report.contacts[event].time >= report.contacts[event - 1].time;
    }
    const double momentum = next[0].velocity.z + next[1].velocity.z + next[2].velocity.z;
    valid = valid && fabs(momentum - previous[0].velocity.z) < dd2_pair_test_tolerance;
    double first_energy = 0;
    double last_energy = 0;
    for (unsigned body = 0; body < DD2_PAIR_TEST_CHAIN; ++body) {
        first_energy += dd2_pair_test_energy(&previous[body]);
        last_energy += dd2_pair_test_energy(&next[body]);
    }
    if (!valid) {
        printf("Chain pairs=%u momentum=%.17g third=%.17g\n", report.pair_contacts, momentum,
               next[2].velocity.z);
    }
    return valid && last_energy <= first_energy;
}
static bool dd2_pair_test_report_bound(void) {
    dd2_vehicle bodies[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        if (!dd2_pair_test_reset(&bodies[slot], 0)) {
            return false;
        }
    }
    dd2_vehicle_collision_report report = {0};
    if (!dd2_vehicle_collide_fleet_report(bodies, bodies, DD2_VEHICLE_FLEET_LIMIT, NULL, NULL,
                                          &report) ||
        report.count != DD2_VEHICLE_CONTACT_LIMIT || report.pair_contacts != report.count) {
        return false;
    }
    for (unsigned event = 0; event < report.count; ++event) {
        if (report.contacts[event].impulse != 0 || report.contacts[event].normal_speed != 0 ||
            report.contacts[event].time != 0) {
            return false;
        }
    }
    /* Invalid body/count must not leak any previously completed contact data. */
    const dd2_vehicle saved = bodies[0];
    bodies[1].rotation = (dd2_vehicle_rotation){0};
    return !dd2_vehicle_collide_fleet_report(bodies, bodies, DD2_VEHICLE_FLEET_LIMIT, NULL, NULL,
                                             &report) &&
           report.count == 0 && report.pair_contacts == 0 && report.impacts[0].contacts == 0 &&
           bodies[0].position.x == saved.position.x && bodies[0].position.z == saved.position.z &&
           !dd2_vehicle_collide_fleet_report(bodies, bodies, 0, NULL, NULL, &report) &&
           report.count == 0;
}
static bool dd2_pair_test_bridge_and_invalid(void) {
    dd2_vehicle previous[DD2_PAIR_TEST_BODIES] = {0};
    bool valid = dd2_pair_test_reset(&previous[0], -dd2_pair_test_distance) &&
                 dd2_pair_test_reset(&previous[1], dd2_pair_test_distance);
    previous[1].position.y += dd2_pair_test_bridge;
    previous[0].velocity.z = dd2_pair_test_fast;
    previous[1].velocity.z = -dd2_pair_test_fast;
    dd2_vehicle next[DD2_PAIR_TEST_BODIES] = {0};
    dd2_pair_test_motion(previous, next, DD2_PAIR_TEST_BODIES);
    dd2_car_contact contact = {.time = 1};
    valid = valid &&
            !dd2_car_contact_sweep(&previous[0], &next[0], &previous[1], &next[1], &contact) &&
            contact.time == 0;
    const dd2_vehicle saved = next[0];
    const union {
        uint64_t bits;
        double number;
    } invalid = {.bits = UINT64_C(0x7ff8000000000001)};
    previous[1].position.x = invalid.number;
    dd2_vehicle_impact impacts[DD2_PAIR_TEST_BODIES] = {{.contacts = 1}, {.contacts = 1}};
    unsigned pairs = 1;
    valid = valid &&
            !dd2_vehicle_collide_fleet(next, previous, DD2_PAIR_TEST_BODIES, NULL, NULL, impacts,
                                       &pairs) &&
            next[0].position.z == saved.position.z && next[0].velocity.z == saved.velocity.z &&
            impacts[0].contacts == 0 && impacts[1].contacts == 0 && pairs == 0;
    return valid;
}
int main(void) {
    if (!dd2_pair_test_head_on(false) || !dd2_pair_test_head_on(true) ||
        !dd2_pair_test_glancing() || !dd2_pair_test_contact_continuity() ||
        !dd2_pair_test_rotation() || !dd2_pair_test_chain() ||
        !dd2_pair_test_bridge_and_invalid() || !dd2_pair_test_report_bound() ||
        !dd2_pair_test_rotated_report()) {
        puts("car pair contacts: FAIL");
        return EXIT_FAILURE;
    }
    puts("car pair contacts: PASS");
    return EXIT_SUCCESS;
}
