#include "contact_patch_fixture.h"
#include "physics/car_contact.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_PAIR_TEST_BODIES = 2,
    DD2_PAIR_TEST_CHAIN = 3,
    DD2_PAIR_ROTATION_CASES = 60,
    DD2_PAIR_ROOT_STEPS = 60
};
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
static const double dd2_pair_test_side_arm = 186;
static const double dd2_pair_test_vertical_arm = 130;
static const double dd2_pair_test_band_width = 1e-6;
static const double dd2_pair_test_band_overlap = 0.1;
static const double dd2_pair_test_band_speed = 1000;
static const double dd2_pair_test_band_spin = 0.01;

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
static bool dd2_pair_test_common_motion(void) {
    static const double angles[] = {0, 0.25, 1.1, 1.5707963267948966, 2.9};
    static const double common_speed = 1000;
    for (size_t angle = 0; angle < sizeof(angles) / sizeof(angles[0]); ++angle) {
        dd2_vehicle previous[DD2_VEHICLE_FLEET_LIMIT] = {0};
        for (unsigned body = 0; body < DD2_VEHICLE_FLEET_LIMIT; ++body) {
            const double along = (double)body * 2 * dd2_pair_test_front_arm;
            if (!dd2_vehicle_reset(
                    &previous[body],
                    (dd2_vehicle_spawn){.position = {.x = sin(angles[angle]) * along,
                                                     .y = dd2_pair_test_height,
                                                     .z = cos(angles[angle]) * along},
                                        .yaw = angles[angle]})) {
                return false;
            }
            previous[body].velocity = (dd2_vehicle_vector){.x = common_speed * cos(angles[angle]),
                                                           .z = -common_speed * sin(angles[angle])};
        }
        dd2_vehicle next[DD2_VEHICLE_FLEET_LIMIT] = {0};
        dd2_pair_test_motion(previous, next, DD2_VEHICLE_FLEET_LIMIT);
        dd2_vehicle_collision_report report = {0};
        if (!dd2_vehicle_collide_fleet_report(next, previous, DD2_VEHICLE_FLEET_LIMIT, NULL, NULL,
                                              &report) ||
            report.count != 0) {
            printf("Common motion angle=%.17g contacts=%u\n", angles[angle], report.count);
            return false;
        }
        for (unsigned body = 0; body < DD2_VEHICLE_FLEET_LIMIT; ++body) {
            const double travel =
                ((next[body].position.x - previous[body].position.x) * cos(angles[angle])) -
                ((next[body].position.z - previous[body].position.z) * sin(angles[angle]));
            if (fabs(travel - (common_speed * DD2_VEHICLE_STEP_SECONDS)) >
                dd2_pair_test_tolerance) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_pair_test_small_closing(void) {
    static const double separation = 2e-6;
    static const double travel = 4e-5;
    dd2_vehicle previous[DD2_PAIR_TEST_BODIES] = {0};
    if (!dd2_pair_test_reset(&previous[0], 0) ||
        !dd2_pair_test_reset(&previous[1], (2 * dd2_pair_test_front_arm) + separation)) {
        return false;
    }
    previous[0].velocity.z = travel / DD2_VEHICLE_STEP_SECONDS;
    dd2_vehicle next[DD2_PAIR_TEST_BODIES] = {0};
    dd2_pair_test_motion(previous, next, DD2_PAIR_TEST_BODIES);
    dd2_car_contact contact = {0};
    if (!dd2_car_contact_sweep(&previous[0], &next[0], &previous[1], &next[1], &contact) ||
        fabs(contact.time - (separation / travel)) > dd2_pair_test_tolerance) {
        return false;
    }
    dd2_vehicle_collision_report report = {0};
    return dd2_vehicle_collide_fleet_report(next, previous, DD2_PAIR_TEST_BODIES, NULL, NULL,
                                            &report) &&
           report.count == 1 && report.contacts[0].impulse > 0;
}

typedef struct {
    unsigned axis;
    double turn;
    double sign;
    bool collision;
} dd2_pair_rotation_case;
typedef struct {
    double extent;
    double side;
    double peak;
    double gap;
    double turn;
} dd2_pair_rotation_geometry;
static double dd2_pair_test_rotation_time(dd2_pair_rotation_geometry geometry) {
    double low = 0;
    double high = geometry.peak;
    for (unsigned iteration = 0; iteration < DD2_PAIR_ROOT_STEPS; ++iteration) {
        const double middle = (low + high) / 2;
        if ((geometry.extent * cos(middle)) + (geometry.side * sin(middle)) - geometry.extent <
            geometry.gap) {
            low = middle;
        } else {
            high = middle;
        }
    }
    const double tangent = tan(high / 2);
    return tangent / (sin(geometry.turn / 2) + ((1 - cos(geometry.turn / 2)) * tangent));
}
static bool dd2_pair_test_rotation_case(dd2_pair_rotation_case test) {
    const double extent = test.axis == 2 ? 186 : 450;
    const double side = test.axis == 1 ? 186 : 130;
    const double peak = fmin(test.turn, atan2(side, extent));
    const double reach = (extent * cos(peak)) + (side * sin(peak)) - extent;
    const double gap = test.collision ? reach / 2 : reach + 0.01;
    dd2_vehicle start[2] = {0};
    dd2_vehicle end[2] = {0};
    if (!dd2_pair_test_reset(&start[0], 0) || !dd2_pair_test_reset(&start[1], 0)) {
        return false;
    }
    if (test.axis == 2) {
        start[1].position.x = (2 * extent) + gap;
    } else {
        start[1].position.z = (2 * extent) + gap;
    }
    end[0] = start[0];
    end[1] = start[1];
    const double sine = sin(test.turn / 2) * test.sign;
    end[0].rotation = (dd2_vehicle_rotation){.x = test.axis == 0 ? sine : 0,
                                             .y = test.axis == 1 ? sine : 0,
                                             .z = test.axis == 2 ? sine : 0,
                                             .w = cos(test.turn / 2)};
    dd2_car_contact hit = {0};
    const bool found = dd2_car_contact_sweep(&start[0], &end[0], &start[1], &end[1], &hit);
    if (found != test.collision || hit.unresolved) {
        return false;
    }
    if (!found) {
        return true;
    }
    /* Independent corner reach and quaternion interpolation determine the
     * first contact, without using the sweep's SAT axes or envelopes. */
    const double expected = dd2_pair_test_rotation_time((dd2_pair_rotation_geometry){
        .extent = extent, .side = side, .peak = peak, .gap = gap, .turn = test.turn});
    const double quaternion_y = expected * sin(test.turn / 2);
    const double quaternion_w = 1 - (expected * (1 - cos(test.turn / 2)));
    const double angle = 2 * atan2(quaternion_y, quaternion_w);
    const double rate = (side * cos(angle) - extent * sin(angle)) * 2 * sin(test.turn / 2) /
                        ((quaternion_y * quaternion_y) + (quaternion_w * quaternion_w));
    const double tolerance = (3 * dd2_pair_test_tolerance) / rate;
    if (fabs(hit.time - expected) > tolerance || hit.penetration > (3 * dd2_pair_test_tolerance)) {
        printf("Rotating precision axis=%u turn=%.17g hit=%.17g expected=%.17g depth=%.17g\n",
               test.axis, test.turn, hit.time, expected, hit.penetration);
        return false;
    }
    return true;
}
static bool dd2_pair_test_rotation_precision(void) {
    static const double turns[] = {0.0005, 0.005, 0.05, 0.32, 1};
    for (unsigned index = 0; index < DD2_PAIR_ROTATION_CASES; ++index) {
        const dd2_pair_rotation_case test = {.axis = index / 20,
                                             .turn = turns[(index / 4) % 5],
                                             .sign = ((index / 2) % 2) == 0 ? 1 : -1,
                                             .collision = (index % 2) != 0};
        if (!dd2_pair_test_rotation_case(test)) {
            return false;
        }
    }
    return true;
}
static bool dd2_pair_test_touching_crossing(void) {
    dd2_vehicle start[2] = {0};
    dd2_vehicle end[2] = {0};
    if (!dd2_pair_test_reset(&start[0], -2 * dd2_pair_test_front_arm) ||
        !dd2_pair_test_reset(&start[1], 0)) {
        return false;
    }
    end[0] = start[0];
    end[1] = start[1];
    end[0].position.z = 2 * dd2_pair_test_front_arm;
    dd2_car_contact hit = {0};
    return dd2_car_contact_sweep(&start[0], &end[0], &start[1], &end[1], &hit) && !hit.unresolved &&
           hit.time == 0;
}

static dd2_vehicle_rotation dd2_pair_test_pitch_yaw(dd2_vehicle_vector angles) {
    return (dd2_vehicle_rotation){.x = sin(angles.x / 2) * cos(angles.y / 2),
                                  .y = cos(angles.x / 2) * sin(angles.y / 2),
                                  .z = -sin(angles.x / 2) * sin(angles.y / 2),
                                  .w = cos(angles.x / 2) * cos(angles.y / 2)};
}
static bool dd2_pair_test_near_parallel_features(void) {
    static const dd2_vehicle_vector angles[DD2_PAIR_TEST_BODIES] = {
        {.x = -0.00023104685182753293, .y = -1.5707963267948966},
        {.x = 0.00043841613351469372, .y = -1.5707963267948966}};
    static const dd2_vehicle_vector offset = {
        .x = -899.99900000000002, .y = 0.212744303728, .z = -0.36603131272848299};
    dd2_vehicle cars[DD2_PAIR_TEST_BODIES] = {0};
    if (!dd2_pair_test_reset(&cars[0], 0) || !dd2_pair_test_reset(&cars[1], 0)) {
        return false;
    }
    cars[1].position.x += offset.x;
    cars[1].position.y += offset.y;
    cars[1].position.z += offset.z;
    cars[0].rotation = dd2_pair_test_pitch_yaw(angles[0]);
    dd2_car_contact hits[DD2_PAIR_TEST_BODIES] = {0};
    for (unsigned variant = 0; variant < DD2_PAIR_TEST_BODIES; ++variant) {
        dd2_vehicle_vector changed = angles[1];
        changed.y += (variant == 0 ? -1 : 1) * dd2_pair_test_tiny_tilt;
        cars[1].rotation = dd2_pair_test_pitch_yaw(changed);
        if (!dd2_car_contact_sweep(&cars[0], &cars[0], &cars[1], &cars[1], &hits[variant]) ||
            hits[variant].unresolved || hits[variant].penetration <= 0) {
            return false;
        }
    }
    /* A 2e-12 yaw perturbation must not choose a different distant edge
     * when face/cross SAT depths describe the same contact within tolerance. */
    const double difference =
        hypot(hypot(hits[0].point.x - hits[1].point.x, hits[0].point.y - hits[1].point.y),
              hits[0].point.z - hits[1].point.z);
    return difference < dd2_pair_test_tolerance;
}

static double dd2_pair_test_band_center(double half, double angle, bool roll) {
    const double shift = (roll ? 1 : -1) * dd2_pair_test_vertical_arm * sin(angle);
    double low = fmax(-half, (-half * cos(angle)) + shift);
    double high = fmin(half, (half * cos(angle)) + shift);
    /* Each face direction reserves half the contact tolerance for flat-edge
     * ties. The remaining height spread cuts a rectangle analytically. */
    const double spread = (2 * half * fabs(sin(angle))) - (dd2_pair_test_band_width / 2);
    if (spread > 0) {
        const double width = 2 * half * cos(angle) * dd2_pair_test_band_width / spread;
        if ((angle > 0) == roll) {
            high = fmin(high, low + width);
        } else {
            low = fmax(low, high - width);
        }
    }
    return (low + high) / 2;
}

typedef struct {
    dd2_vehicle_vector origin;
    double angle;
    bool roll;
} dd2_pair_band_case;

static bool dd2_pair_test_face_case(dd2_pair_band_case input) {
    dd2_vehicle cars[DD2_PAIR_TEST_BODIES] = {0};
    dd2_vehicle_spawn spawn = {.position = input.origin};
    if (!dd2_vehicle_reset(&cars[0], spawn)) {
        return false;
    }
    spawn.position.y += (2 * dd2_pair_test_vertical_arm) - dd2_pair_test_band_overlap;
    if (!dd2_vehicle_reset(&cars[1], spawn)) {
        return false;
    }
    cars[1].rotation = (dd2_vehicle_rotation){.x = input.roll ? 0 : sin(input.angle / 2),
                                              .z = input.roll ? sin(input.angle / 2) : 0,
                                              .w = cos(input.angle / 2)};
    dd2_car_contact hit = {0};
    if (!dd2_car_contact_sweep(&cars[0], &cars[0], &cars[1], &cars[1], &hit) || hit.unresolved ||
        hit.penetration <= 0) {
        return false;
    }
    const double actual =
        input.roll ? hit.point.x - spawn.position.x : hit.point.z - spawn.position.z;
    const double half = input.roll ? dd2_pair_test_side_arm : dd2_pair_test_front_arm;
    const double expected = dd2_pair_test_band_center(half, input.angle, input.roll);
    if (fabs(actual - expected) > dd2_pair_test_tolerance ||
        fabs(hit.normal.y + 1) > dd2_pair_test_tolerance) {
        printf("Face band roll=%d angle=%.17g point=%.17g expected=%.17g\n", (int)input.roll,
               input.angle, actual, expected);
        return false;
    }
    return true;
}

static bool dd2_pair_test_band_collision(double factor) {
    const double angle = factor * dd2_pair_test_band_width / (2 * dd2_pair_test_front_arm);
    dd2_vehicle previous[DD2_PAIR_TEST_BODIES] = {0};
    if (!dd2_pair_test_reset(&previous[0], 0) || !dd2_pair_test_reset(&previous[1], 0)) {
        return false;
    }
    previous[1].position.y += (2 * dd2_pair_test_vertical_arm) - dd2_pair_test_band_overlap;
    previous[1].rotation = (dd2_vehicle_rotation){.x = sin(angle / 2), .w = cos(angle / 2)};
    previous[0].velocity.y = dd2_pair_test_band_speed;
    dd2_vehicle next[DD2_PAIR_TEST_BODIES] = {0};
    dd2_pair_test_motion(previous, next, DD2_PAIR_TEST_BODIES);
    dd2_vehicle_collision_report report = {0};
    if (!dd2_vehicle_collide_fleet_report(next, previous, DD2_PAIR_TEST_BODIES, NULL, NULL,
                                          &report) ||
        report.count != 1 || report.contacts[0].impulse <= 0 ||
        fabs(next[0].angular_velocity.x) >= dd2_pair_test_band_spin ||
        fabs(next[1].angular_velocity.x) >= dd2_pair_test_band_spin ||
        dd2_pair_test_energy(&next[0]) + dd2_pair_test_energy(&next[1]) >
            dd2_pair_test_energy(&previous[0]) + dd2_pair_test_energy(&previous[1])) {
        printf("Face band collision factor=%.17g contacts=%u impulse=%.17g spin=%.17g,%.17g\n",
               factor, report.count, report.contacts[0].impulse, next[0].angular_velocity.x,
               next[1].angular_velocity.x);
        return false;
    }
    return true;
}

static bool dd2_pair_test_face_band(void) {
    static const double factors[] = {0, 0.25, 0.5, 0.99999, 1.00001, 1.1, 1.49999, 1.50001, 2, 4};
    static const dd2_vehicle_vector origins[] = {{.y = 5000},
                                                 {.x = -1000000, .y = 1000000, .z = 20000000},
                                                 {.x = 1000000, .y = -1000000, .z = -20000000}};
    for (unsigned axis = 0; axis < DD2_PAIR_TEST_BODIES; ++axis) {
        const bool roll = axis != 0;
        const double half = roll ? dd2_pair_test_side_arm : dd2_pair_test_front_arm;
        for (unsigned sign = 0; sign < DD2_PAIR_TEST_BODIES; ++sign) {
            for (size_t sample = 0; sample < sizeof(factors) / sizeof(factors[0]); ++sample) {
                const double angle =
                    (sign == 0 ? 1 : -1) * factors[sample] * dd2_pair_test_band_width / (2 * half);
                for (size_t origin = 0; origin < sizeof(origins) / sizeof(origins[0]); ++origin) {
                    if (!dd2_pair_test_face_case((dd2_pair_band_case){
                            .origin = origins[origin], .angle = angle, .roll = roll})) {
                        return false;
                    }
                }
            }
        }
    }
    /* Crossing the old vertex-selection threshold must not create a distant
     * lever arm. These closing collisions check the resulting physical spin. */
    return dd2_pair_test_band_collision(factors[3]) && dd2_pair_test_band_collision(factors[4]);
}

static dd2_vehicle_vector dd2_pair_patch_local(const dd2_vehicle *start, const dd2_vehicle *end,
                                               dd2_vehicle_vector point, double time) {
    const dd2_vehicle_rotation first = start->rotation;
    const dd2_vehicle_rotation last = end->rotation;
    const double sign =
        ((first.x * last.x) + (first.y * last.y) + (first.z * last.z) + (first.w * last.w)) < 0 ? -1
                                                                                                : 1;
    dd2_vehicle_rotation rotation = {.x = first.x + (((sign * last.x) - first.x) * time),
                                     .y = first.y + (((sign * last.y) - first.y) * time),
                                     .z = first.z + (((sign * last.z) - first.z) * time),
                                     .w = first.w + (((sign * last.w) - first.w) * time)};
    const double length = sqrt((rotation.x * rotation.x) + (rotation.y * rotation.y) +
                               (rotation.z * rotation.z) + (rotation.w * rotation.w));
    rotation.x /= -length;
    rotation.y /= -length;
    rotation.z /= -length;
    rotation.w /= length;
    const dd2_vehicle_vector arm = {
        .x = point.x - start->position.x - ((end->position.x - start->position.x) * time),
        .y = point.y - start->position.y - ((end->position.y - start->position.y) * time),
        .z = point.z - start->position.z - ((end->position.z - start->position.z) * time)};
    return dd2_vehicle_rotate(rotation, arm);
}

static bool dd2_pair_test_front_patch(void) {
    dd2_car_contact hit = {0};
    if (!dd2_car_contact_sweep(&dd2_patch_start[0], &dd2_patch_end[0], &dd2_patch_start[1],
                               &dd2_patch_end[1], &hit) ||
        hit.unresolved) {
        return false;
    }
    for (unsigned body = 0; body < DD2_PAIR_TEST_BODIES; ++body) {
        const dd2_vehicle_vector point =
            dd2_pair_patch_local(&dd2_patch_start[body], &dd2_patch_end[body], hit.point, hit.time);
        if (fabs(point.x) > dd2_pair_test_side_arm + dd2_pair_test_tolerance ||
            fabs(point.y) > dd2_pair_test_vertical_arm + dd2_pair_test_tolerance ||
            fabs(point.z) > dd2_pair_test_front_arm + dd2_pair_test_tolerance) {
            printf("Front patch body=%u point=%.17g,%.17g,%.17g\n", body, point.x, point.y,
                   point.z);
            return false;
        }
    }
    dd2_vehicle next[DD2_PAIR_TEST_BODIES] = {dd2_patch_end[0], dd2_patch_end[1]};
    dd2_vehicle_collision_report report = {0};
    return dd2_vehicle_collide_fleet_report(next, dd2_patch_start, DD2_PAIR_TEST_BODIES, NULL, NULL,
                                            &report) &&
           report.count > 0 && report.contacts[0].impulse > 0;
}

int main(void) {
    if (!dd2_pair_test_front_patch() || !dd2_pair_test_face_band() ||
        !dd2_pair_test_near_parallel_features() || !dd2_pair_test_rotation_precision() ||
        !dd2_pair_test_touching_crossing() || !dd2_pair_test_common_motion() ||
        !dd2_pair_test_small_closing() || !dd2_pair_test_head_on(false) ||
        !dd2_pair_test_head_on(true) || !dd2_pair_test_glancing() ||
        !dd2_pair_test_contact_continuity() || !dd2_pair_test_rotation() ||
        !dd2_pair_test_chain() || !dd2_pair_test_bridge_and_invalid() ||
        !dd2_pair_test_report_bound() || !dd2_pair_test_rotated_report()) {
        puts("car pair contacts: FAIL");
        return EXIT_FAILURE;
    }
    puts("car pair contacts: PASS");
    return EXIT_SUCCESS;
}
