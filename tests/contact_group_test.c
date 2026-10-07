#include "physics/collision_math.h"
#include "physics/contact_group.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static const double dd2_group_test_tolerance = 1e-6;
static const double dd2_group_test_clearance = 1e-4;
static const double dd2_group_test_speed = 10;
static const double dd2_group_test_sliding = 100;
static const double dd2_group_test_sticking = 0.1;
static const double dd2_group_test_coefficient = 0.25;
static const double dd2_group_test_micro_slip = 0.1;
static const double dd2_group_test_arm = 186;
static const double dd2_group_test_height = 130;
static const double dd2_group_test_roll = 0.1;
static const double dd2_group_test_roll_inertia = 17165.333333333333;

enum { DD2_GROUP_TEST_PAIR = 2 };

static bool dd2_group_test_cascade(void) {
    dd2_vehicle bodies[DD2_GROUP_TEST_PAIR] = {0};
    if (!dd2_vehicle_reset(&bodies[0], (dd2_vehicle_spawn){0}) ||
        !dd2_vehicle_reset(&bodies[1], (dd2_vehicle_spawn){.position = {.x = 1}})) {
        return false;
    }
    bodies[0].velocity.x = -dd2_group_test_speed;
    bodies[1].velocity.x = -2 * dd2_group_test_speed;
    const dd2_group_contact contacts[] = {
        {.first = 0, .second = DD2_VEHICLE_NO_PARTNER, .normal = {.x = 1}},
        {.first = 0, .second = 1, .normal = {.x = -1}}};
    dd2_group_solution result = {0};
    const dd2_group_query query = {.bodies = bodies,
                                   .body_count = DD2_GROUP_TEST_PAIR,
                                   .contacts = contacts,
                                   .contact_count = DD2_GROUP_TEST_PAIR};
    if (!dd2_contact_group_solve(&query, &result) || result.count != DD2_GROUP_TEST_PAIR) {
        return false;
    }
    /* v0>=0 at the wall and v1>=v0 at the pair. The equal-mass projection is
     * v0=v1=0, with wall impulse 30 and pair impulse 20. Position correction is
     * the same independent inequality problem, requiring x0+=c, x1+=3c. */
    return fabs(bodies[0].velocity.x) < dd2_group_test_tolerance &&
           fabs(bodies[1].velocity.x) < dd2_group_test_tolerance &&
           fabs(result.contacts[0].normal_impulse - (3 * dd2_group_test_speed)) <
               dd2_group_test_tolerance &&
           fabs(result.contacts[1].normal_impulse - (2 * dd2_group_test_speed)) <
               dd2_group_test_tolerance &&
           fabs(bodies[0].position.x - dd2_group_test_clearance) < dd2_group_test_tolerance &&
           fabs(bodies[1].position.x - 1 - (3 * dd2_group_test_clearance)) <
               dd2_group_test_tolerance;
}

static bool dd2_group_test_friction(bool sliding) {
    dd2_vehicle body = {0};
    if (!dd2_vehicle_reset(&body, (dd2_vehicle_spawn){0})) {
        return false;
    }
    body.velocity.x = sliding ? -dd2_group_test_sliding : -dd2_group_test_sticking;
    body.velocity.y = -dd2_group_test_speed;
    const double energy = dd2_collision_dot(body.velocity, body.velocity);
    const dd2_group_contact contact = {.first = 0,
                                       .second = DD2_VEHICLE_NO_PARTNER,
                                       .normal = {.y = 1},
                                       .friction = dd2_group_test_coefficient};
    dd2_group_solution result = {0};
    const dd2_group_query query = {
        .bodies = &body, .body_count = 1, .contacts = &contact, .contact_count = 1};
    if (!dd2_contact_group_solve(&query, &result)) {
        return false;
    }
    const double expected =
        sliding ? dd2_group_test_coefficient * dd2_group_test_speed : dd2_group_test_sticking;
    const dd2_group_response response = result.contacts[0];
    return fabs(response.friction_impulse.x - expected) < dd2_group_test_tolerance &&
           fabs(body.velocity.x + (sliding ? dd2_group_test_sliding : dd2_group_test_sticking) -
                expected) < dd2_group_test_tolerance &&
           fabs(body.velocity.y) < dd2_group_test_tolerance &&
           fabs(response.normal_impulse - dd2_group_test_speed) < dd2_group_test_tolerance &&
           response.friction_impulse.y == 0 && response.friction_impulse.z == 0 &&
           dd2_collision_dot(body.velocity, body.velocity) <= energy;
}

static bool dd2_group_test_rocking(bool both) {
    dd2_vehicle body = {0};
    if (!dd2_vehicle_reset(&body, (dd2_vehicle_spawn){.position = {.y = dd2_group_test_height}})) {
        return false;
    }
    body.velocity.y = both ? -dd2_group_test_speed : -dd2_group_test_sticking;
    body.angular_velocity.z = dd2_group_test_roll;
    const double energy =
        (body.velocity.y * body.velocity.y) +
        (dd2_group_test_roll_inertia * body.angular_velocity.z * body.angular_velocity.z);
    const dd2_group_contact contacts[] = {{.first = 0,
                                           .second = DD2_VEHICLE_NO_PARTNER,
                                           .point = {.x = dd2_group_test_arm},
                                           .normal = {.y = 1}},
                                          {.first = 0,
                                           .second = DD2_VEHICLE_NO_PARTNER,
                                           .point = {.x = -dd2_group_test_arm},
                                           .normal = {.y = 1}}};
    dd2_group_solution result = {0};
    const dd2_group_query query = {.bodies = &body,
                                   .body_count = 1,
                                   .contacts = contacts,
                                   .contact_count = DD2_GROUP_TEST_PAIR};
    if (!dd2_contact_group_solve(&query, &result)) {
        return false;
    }
    const double first_speed = body.velocity.y + (dd2_group_test_arm * body.angular_velocity.z);
    const double second_speed = body.velocity.y - (dd2_group_test_arm * body.angular_velocity.z);
    const double after_energy =
        (body.velocity.y * body.velocity.y) +
        (dd2_group_test_roll_inertia * body.angular_velocity.z * body.angular_velocity.z);
    if (after_energy > energy || fabs(second_speed) > dd2_group_test_tolerance ||
        first_speed < -dd2_group_test_tolerance || result.contacts[1].normal_impulse <= 0) {
        return false;
    }
    if (both) {
        return fabs(first_speed) < dd2_group_test_tolerance &&
               fabs(body.velocity.y) < dd2_group_test_tolerance &&
               fabs(body.angular_velocity.z) < dd2_group_test_tolerance &&
               result.contacts[0].normal_impulse > 0;
    }
    return first_speed > 0 && result.contacts[0].normal_impulse == 0;
}

/* Frozen three-car support chain from the native frame-partition regression.
 * Slip-direction effective mass used to oscillate for all 4096 passes here. */
static bool dd2_group_test_chain(void) {
    enum { DD2_GROUP_TEST_CHAIN = 3 };
    dd2_vehicle bodies[DD2_GROUP_TEST_CHAIN] = {0};
    const dd2_vehicle initial[DD2_GROUP_TEST_CHAIN] = {
        {.position = {.x = -4325.820407810862, .y = 183.06541124228636, .z = 4838.3805537619155},
         .rotation = {.x = -8.7079600790143557e-05,
                      .y = -0.99222336540368106,
                      .z = 5.4447703062190493e-05,
                      .w = -0.12447000682728399},
         .velocity = {.x = -8.6755781870551889,
                      .y = -0.055491695519232824,
                      .z = 148.67328779364578},
         .angular_velocity = {.x = 0.00023598360330373357,
                              .y = 0.082274106779537295,
                              .z = 0.0068802995398760261}},
        {.position = {.x = -4972.3559295650966, .y = 183.22828125507655, .z = 4933.0613751146393},
         .rotation = {.x = -0.00011183655332190231,
                      .y = -0.90339673573028267,
                      .z = -0.00018471139922838782,
                      .w = -0.42880565673292431},
         .velocity = {.x = 28.860183671810489, .y = -0.36998823365221306, .z = 29.376098823587498},
         .angular_velocity = {.x = 0.023072792079002865,
                              .y = 0.010598859280879721,
                              .z = -0.023065260709068339}},
        {.position = {.x = -5218.5854458141239, .y = 182.88753717063224, .z = 4313.0935895783668},
         .rotation = {.x = -0.00031550212098456265,
                      .y = -0.33556977283893497,
                      .z = -0.00035165165716884573,
                      .w = -0.94201523573472501},
         .velocity = {.x = 15.728887614477991, .y = 0.43803959432761796, .z = 37.097545570598669},
         .angular_velocity = {.x = -0.010768041174723186,
                              .y = 0.029803953568345101,
                              .z = 0.002358126125516198}},
    };
    dd2_vehicle_vector momentum = {0};
    for (unsigned slot = 0; slot < DD2_GROUP_TEST_CHAIN; ++slot) {
        if (!dd2_vehicle_reset(&bodies[slot], (dd2_vehicle_spawn){0})) {
            return false;
        }
        bodies[slot].position = initial[slot].position;
        bodies[slot].rotation = initial[slot].rotation;
        bodies[slot].velocity = initial[slot].velocity;
        bodies[slot].angular_velocity = initial[slot].angular_velocity;
        momentum = dd2_collision_add(momentum, bodies[slot].velocity);
    }
    const dd2_group_contact contacts[] = {
        {.first = 0,
         .second = 1,
         .normal = {.x = 0.96901441963511803,
                    .y = -0.00015925061716424898,
                    .z = 0.24700410761454217},
         .point = {.x = -4506.1193588794395, .y = 53.27863006169737, .z = 4792.5983773130938},
         .friction = dd2_group_test_coefficient},
        {.first = 1,
         .second = 2,
         .normal = {.x = 0.63225139258309593,
                    .y = -0.00035837725212945609,
                    .z = 0.77476322069551096},
         .point = {.x = -4789.9161293664456, .y = 53.387444236234323, .z = 4544.0461076956044},
         .penetration = -0.0001977793580749676,
         .friction = dd2_group_test_coefficient}};
    const dd2_group_query query = {.bodies = bodies,
                                   .body_count = DD2_GROUP_TEST_CHAIN,
                                   .contacts = contacts,
                                   .contact_count = DD2_GROUP_TEST_PAIR};
    dd2_group_solution result = {0};
    if (!dd2_contact_group_solve(&query, &result) || result.count != DD2_GROUP_TEST_PAIR) {
        return false;
    }
    for (unsigned index = 0; index < DD2_GROUP_TEST_PAIR; ++index) {
        const dd2_group_contact contact = contacts[index];
        const dd2_group_response response = result.contacts[index];
        dd2_vehicle_vector relative = {0};
        for (unsigned side = 0; side < DD2_GROUP_TEST_PAIR; ++side) {
            const unsigned slot = side == 0 ? contact.first : contact.second;
            const dd2_vehicle_vector arm =
                dd2_collision_add(contact.point, dd2_collision_scale(initial[slot].position, -1));
            const dd2_vehicle_vector velocity = dd2_collision_point_velocity(&bodies[slot], arm);
            relative =
                dd2_collision_add(relative, dd2_collision_scale(velocity, side == 0 ? 1 : -1));
        }
        const double normal_speed = dd2_collision_dot(relative, contact.normal);
        const dd2_vehicle_vector slip =
            dd2_collision_add(relative, dd2_collision_scale(contact.normal, -normal_speed));
        const double slip_length = sqrt(dd2_collision_dot(slip, slip));
        const double tangent_length =
            sqrt(dd2_collision_dot(response.friction_impulse, response.friction_impulse));
        const double limit = contact.friction * response.normal_impulse;
        if (response.normal_impulse <= 0 || fabs(normal_speed) > dd2_group_test_tolerance ||
            tangent_length > limit + dd2_group_test_tolerance ||
            fabs(dd2_collision_dot(response.friction_impulse, contact.normal)) >
                dd2_group_test_tolerance) {
            return false;
        }
        /* Independent constitutive check: low-speed friction is linear in
         * final slip, with Coulomb saturation above the documented threshold. */
        const dd2_vehicle_vector residual = dd2_collision_add(
            slip, dd2_collision_scale(response.friction_impulse,
                                      fmax(dd2_group_test_micro_slip, slip_length) / limit));
        if (sqrt(dd2_collision_dot(residual, residual)) > dd2_group_test_tolerance) {
            return false;
        }
    }
    for (unsigned slot = 0; slot < DD2_GROUP_TEST_CHAIN; ++slot) {
        momentum = dd2_collision_add(momentum, dd2_collision_scale(bodies[slot].velocity, -1));
    }
    return sqrt(dd2_collision_dot(momentum, momentum)) < dd2_group_test_tolerance;
}

static bool dd2_group_test_invalid(void) {
    dd2_vehicle body = {0};
    if (!dd2_vehicle_reset(&body, (dd2_vehicle_spawn){0})) {
        return false;
    }
    dd2_group_contact contact = {.first = 0, .second = DD2_VEHICLE_NO_PARTNER, .normal = {.y = 1}};
    dd2_group_query query = {
        .bodies = &body, .body_count = 1, .contacts = &contact, .contact_count = 1};
    dd2_group_solution result = {.count = 1};
    if (dd2_contact_group_solve(NULL, &result) || result.count != 0 ||
        dd2_contact_group_solve(&query, NULL)) {
        return false;
    }
    const union {
        uint64_t bits;
        double number;
    } not_a_number = {.bits = UINT64_C(0x7ff8000000000001)};
    contact.friction = not_a_number.number;
    if (dd2_contact_group_solve(&query, &result) || result.count != 0) {
        return false;
    }
    contact.friction = 0;
    contact.second = DD2_VEHICLE_NO_PARTNER + 1;
    if (dd2_contact_group_solve(&query, &result) || result.count != 0) {
        return false;
    }
    contact.second = DD2_VEHICLE_NO_PARTNER;
    query.contact_count = DD2_VEHICLE_CONTACT_LIMIT + 1;
    if (dd2_contact_group_solve(&query, &result) || result.count != 0) {
        return false;
    }
    return body.position.x == 0 && body.position.y == 0 && body.position.z == 0 &&
           body.velocity.x == 0 && body.velocity.y == 0 && body.velocity.z == 0;
}

static bool dd2_group_test_infeasible(void) {
    dd2_vehicle body = {0};
    if (!dd2_vehicle_reset(&body, (dd2_vehicle_spawn){0})) {
        return false;
    }
    const dd2_group_contact contacts[] = {
        {.first = 0, .second = DD2_VEHICLE_NO_PARTNER, .normal = {.x = 1}, .penetration = 1},
        {.first = 0, .second = DD2_VEHICLE_NO_PARTNER, .normal = {.x = -1}, .penetration = 1}};
    const dd2_group_query query = {.bodies = &body,
                                   .body_count = 1,
                                   .contacts = contacts,
                                   .contact_count = DD2_GROUP_TEST_PAIR};
    dd2_group_solution result = {.count = 1};
    return !dd2_contact_group_solve(&query, &result) && result.count == 0 && body.position.x == 0 &&
           body.velocity.x == 0;
}

int main(void) {
    if (!dd2_group_test_cascade() || !dd2_group_test_friction(false) ||
        !dd2_group_test_friction(true) || !dd2_group_test_rocking(false) ||
        !dd2_group_test_rocking(true) || !dd2_group_test_chain() || !dd2_group_test_invalid() ||
        !dd2_group_test_infeasible()) {
        puts("joint support projection: FAIL");
        return EXIT_FAILURE;
    }
    puts("joint support projection: PASS");
    return EXIT_SUCCESS;
}
