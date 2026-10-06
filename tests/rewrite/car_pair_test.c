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
    dd2_vehicle_impact impacts[DD2_PAIR_TEST_BODIES] = {0};
    unsigned pairs = 0;
    valid = valid &&
            dd2_vehicle_collide_fleet(next, previous, DD2_PAIR_TEST_BODIES, NULL, NULL, impacts,
                                      &pairs) &&
            pairs == 1 && impacts[0].pair_contacts == 1 && impacts[1].pair_contacts == 1;
    const double first_velocity = side ? next[0].velocity.x : next[0].velocity.z;
    const double second_velocity = side ? next[1].velocity.x : next[1].velocity.z;
    valid = valid && fabs(first_velocity + dd2_pair_test_rebound) < dd2_pair_test_tolerance &&
            fabs(second_velocity - dd2_pair_test_rebound) < dd2_pair_test_tolerance &&
            dd2_pair_test_energy(&next[0]) + dd2_pair_test_energy(&next[1]) <=
                dd2_pair_test_energy(&previous[0]) + dd2_pair_test_energy(&previous[1]);
    valid = valid && !dd2_car_contact_sweep(&next[0], &next[0], &next[1], &next[1], &contact);
    if (!valid) {
        printf("Head-on side=%d pairs=%u v=%.17g,%.17g\n", (int)side, pairs, first_velocity,
               second_velocity);
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
    unsigned pairs = 0;
    valid =
        valid &&
        dd2_vehicle_collide_fleet(next, previous, DD2_PAIR_TEST_CHAIN, NULL, NULL, NULL, &pairs) &&
        pairs >= 2 && next[2].velocity.z > 0;
    const double momentum = next[0].velocity.z + next[1].velocity.z + next[2].velocity.z;
    valid = valid && fabs(momentum - previous[0].velocity.z) < dd2_pair_test_tolerance;
    double first_energy = 0;
    double last_energy = 0;
    for (unsigned body = 0; body < DD2_PAIR_TEST_CHAIN; ++body) {
        first_energy += dd2_pair_test_energy(&previous[body]);
        last_energy += dd2_pair_test_energy(&next[body]);
    }
    if (!valid) {
        printf("Chain pairs=%u momentum=%.17g third=%.17g\n", pairs, momentum, next[2].velocity.z);
    }
    return valid && last_energy <= first_energy;
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
        !dd2_pair_test_bridge_and_invalid()) {
        puts("car pair contacts: FAIL");
        return EXIT_FAILURE;
    }
    puts("car pair contacts: PASS");
    return EXIT_SUCCESS;
}
