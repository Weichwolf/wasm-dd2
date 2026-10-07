#include "contact_friction_fixture.h"
#include "physics/collision_math.h"
#include "physics/contact_group.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

enum { DD2_FRICTION_PASS_LIMIT = 4096, DD2_FRICTION_PAIR = 2 };
static const double dd2_friction_tolerance = 1e-6;
static const double dd2_friction_position_tolerance = 1e-8;
static const double dd2_friction_clearance = 1e-4;
static const double dd2_friction_micro_slip = 0.1;
static const double dd2_friction_load = 10;
static const double dd2_friction_coefficient = 0.25;
static const dd2_vehicle_vector dd2_friction_inertia = {
    /* Box lengths 900 x 372 x 260, inertia divided by mass. */
    .x = 73133.333333333333,
    .y = 79032,
    .z = 17165.333333333333};

typedef struct {
    dd2_vehicle_vector point;
    dd2_vehicle_vector impulse;
} dd2_friction_applied;

static double dd2_friction_length(dd2_vehicle_vector vector) {
    return sqrt(dd2_collision_dot(vector, vector));
}

static dd2_vehicle_vector dd2_friction_local(dd2_vehicle_rotation rotation,
                                             dd2_vehicle_vector vector) {
    return dd2_vehicle_rotate(
        (dd2_vehicle_rotation){
            .x = -rotation.x, .y = -rotation.y, .z = -rotation.z, .w = rotation.w},
        vector);
}

static dd2_vehicle_vector dd2_friction_spin(dd2_vehicle_rotation rotation,
                                            dd2_vehicle_vector torque) {
    const dd2_vehicle_vector local = dd2_friction_local(rotation, torque);
    return dd2_vehicle_rotate(rotation,
                              (dd2_vehicle_vector){.x = local.x / dd2_friction_inertia.x,
                                                   .y = local.y / dd2_friction_inertia.y,
                                                   .z = local.z / dd2_friction_inertia.z});
}

static double dd2_friction_energy(const dd2_vehicle *bodies, unsigned count) {
    double energy = 0;
    for (unsigned slot = 0; slot < count; ++slot) {
        const dd2_vehicle *body = &bodies[slot];
        const dd2_vehicle_vector spin = dd2_friction_local(body->rotation, body->angular_velocity);
        energy += dd2_collision_dot(body->velocity, body->velocity) +
                  (spin.x * spin.x * dd2_friction_inertia.x) +
                  (spin.y * spin.y * dd2_friction_inertia.y) +
                  (spin.z * spin.z * dd2_friction_inertia.z);
    }
    return energy;
}

static void dd2_friction_reaction(dd2_vehicle *expected, const dd2_vehicle *initial,
                                  dd2_friction_applied reaction) {
    expected->velocity = dd2_collision_add(expected->velocity, reaction.impulse);
    const dd2_vehicle_vector arm =
        dd2_collision_add(reaction.point, dd2_collision_scale(initial->position, -1));
    expected->angular_velocity = dd2_collision_add(
        expected->angular_velocity,
        dd2_friction_spin(initial->rotation, dd2_collision_cross(arm, reaction.impulse)));
}

static bool dd2_friction_law(dd2_vehicle_vector slip, dd2_group_response response,
                             dd2_group_contact contact) {
    const double magnitude = dd2_friction_length(response.friction_impulse);
    const double limit = contact.friction * response.normal_impulse;
    if (magnitude > limit + dd2_friction_tolerance ||
        fabs(dd2_collision_dot(response.friction_impulse, contact.normal)) >
            dd2_friction_tolerance) {
        return false;
    }
    if (limit == 0) {
        return magnitude == 0;
    }
    const double speed = dd2_friction_length(slip);
    if (contact.second == DD2_VEHICLE_NO_PARTNER) {
        return speed < dd2_friction_tolerance ||
               (fabs(magnitude - limit) < dd2_friction_tolerance &&
                dd2_friction_length(dd2_collision_add(
                    slip, dd2_collision_scale(response.friction_impulse, speed / limit))) <
                    dd2_friction_tolerance);
    }
    /* The constitutive oracle checks final velocity directly, rather than
     * the solver's projected-gradient residual or its iteration history. */
    const dd2_vehicle_vector residual =
        dd2_collision_add(slip, dd2_collision_scale(response.friction_impulse,
                                                    fmax(dd2_friction_micro_slip, speed) / limit));
    return dd2_friction_length(residual) < dd2_friction_tolerance;
}

static bool dd2_friction_contact(const dd2_vehicle *bodies, const dd2_friction_case *scenario,
                                 unsigned index, dd2_group_response response) {
    const dd2_group_contact contact = scenario->contacts[index];
    dd2_vehicle_vector relative = {0};
    dd2_vehicle_vector offset = {0};
    const unsigned slots[] = {contact.first, contact.second};
    for (unsigned side = 0; side < DD2_FRICTION_PAIR; ++side) {
        const unsigned slot = slots[side];
        if (slot == DD2_VEHICLE_NO_PARTNER) {
            continue;
        }
        const dd2_vehicle_vector arm = dd2_collision_add(
            contact.point, dd2_collision_scale(scenario->initial[slot].position, -1));
        const dd2_vehicle_vector velocity = dd2_collision_point_velocity(&bodies[slot], arm);
        const dd2_vehicle_vector displacement = dd2_collision_add(
            bodies[slot].position, dd2_collision_scale(scenario->initial[slot].position, -1));
        const double sign = side == 0 ? 1 : -1;
        relative = dd2_collision_add(relative, dd2_collision_scale(velocity, sign));
        offset = dd2_collision_add(offset, dd2_collision_scale(displacement, sign));
    }
    const double normal = dd2_collision_dot(relative, contact.normal);
    const double clearance =
        dd2_friction_clearance * (contact.second == DD2_VEHICLE_NO_PARTNER ? 1 : 2);
    if (response.normal_impulse < 0 || normal < -dd2_friction_tolerance ||
        (response.normal_impulse > dd2_friction_tolerance &&
         fabs(normal) > dd2_friction_tolerance) ||
        dd2_collision_dot(offset, contact.normal) <
            contact.penetration + clearance - dd2_friction_position_tolerance) {
        return false;
    }
    return dd2_friction_law(
        dd2_collision_add(relative, dd2_collision_scale(contact.normal, -normal)), response,
        contact);
}

static bool dd2_friction_run(const dd2_friction_case *scenario) {
    dd2_vehicle bodies[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_vehicle expected[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned slot = 0; slot < scenario->body_count; ++slot) {
        if (!dd2_vehicle_reset(&bodies[slot], (dd2_vehicle_spawn){0})) {
            return false;
        }
        bodies[slot].position = scenario->initial[slot].position;
        bodies[slot].rotation = scenario->initial[slot].rotation;
        bodies[slot].velocity = scenario->initial[slot].velocity;
        bodies[slot].angular_velocity = scenario->initial[slot].angular_velocity;
        expected[slot] = bodies[slot];
    }
    const double energy = dd2_friction_energy(bodies, scenario->body_count);
    dd2_group_solution result = {0};
    const dd2_group_query query = {.bodies = bodies,
                                   .contacts = scenario->contacts,
                                   .body_count = scenario->body_count,
                                   .contact_count = scenario->contact_count};
    if (!dd2_contact_group_solve(&query, &result) || result.count != scenario->contact_count ||
        result.velocity_passes > DD2_FRICTION_PASS_LIMIT ||
        dd2_friction_energy(bodies, scenario->body_count) > energy + dd2_friction_tolerance) {
        return false;
    }
    for (unsigned index = 0; index < scenario->contact_count; ++index) {
        const dd2_group_contact contact = scenario->contacts[index];
        const dd2_group_response response = result.contacts[index];
        if (!dd2_friction_contact(bodies, scenario, index, response)) {
            printf("Friction contact failed: %s contact %u\n", scenario->name, index);
            return false;
        }
        const dd2_vehicle_vector impulse =
            dd2_collision_add(dd2_collision_scale(contact.normal, response.normal_impulse),
                              response.friction_impulse);
        dd2_friction_reaction(&expected[contact.first], &scenario->initial[contact.first],
                              (dd2_friction_applied){.point = contact.point, .impulse = impulse});
        if (contact.second != DD2_VEHICLE_NO_PARTNER) {
            dd2_friction_reaction(
                &expected[contact.second], &scenario->initial[contact.second],
                (dd2_friction_applied){.point = contact.point,
                                       .impulse = dd2_collision_scale(impulse, -1)});
        }
    }
    for (unsigned slot = 0; slot < scenario->body_count; ++slot) {
        const dd2_vehicle *body = &bodies[slot];
        const dd2_vehicle *reference = &expected[slot];
        if (!dd2_vehicle_valid(body) || body->steps != reference->steps ||
            body->rotation.x != reference->rotation.x ||
            body->rotation.y != reference->rotation.y ||
            body->rotation.z != reference->rotation.z ||
            body->rotation.w != reference->rotation.w ||
            dd2_friction_length(
                dd2_collision_add(body->velocity, dd2_collision_scale(reference->velocity, -1))) >
                dd2_friction_tolerance ||
            dd2_friction_length(dd2_collision_add(
                body->angular_velocity, dd2_collision_scale(reference->angular_velocity, -1))) >
                dd2_friction_tolerance) {
            return false;
        }
    }
    printf("Car-body friction %s: PASS (contacts=%u passes=%u)\n", scenario->name, result.count,
           result.velocity_passes);
    return true;
}

static bool dd2_friction_analytic(double speed) {
    dd2_vehicle bodies[DD2_FRICTION_PAIR] = {0};
    for (unsigned slot = 0; slot < DD2_FRICTION_PAIR; ++slot) {
        if (!dd2_vehicle_reset(&bodies[slot], (dd2_vehicle_spawn){0})) {
            return false;
        }
    }
    bodies[0].velocity = (dd2_vehicle_vector){.x = speed, .y = -dd2_friction_load};
    const dd2_group_contact contact = {
        .first = 0, .second = 1, .normal = {.y = 1}, .friction = dd2_friction_coefficient};
    dd2_group_solution result = {0};
    const dd2_group_query query = {.bodies = bodies,
                                   .body_count = DD2_FRICTION_PAIR,
                                   .contacts = &contact,
                                   .contact_count = 1};
    if (!dd2_contact_group_solve(&query, &result)) {
        return false;
    }
    const double pressure = dd2_friction_load / (double)DD2_FRICTION_PAIR;
    const double limit = dd2_friction_coefficient * pressure;
    const double impulse =
        copysign(fmin(limit, fabs(speed) * limit /
                                 (((double)DD2_FRICTION_PAIR * limit) + dd2_friction_micro_slip)),
                 -speed);
    return fabs(result.contacts[0].normal_impulse - pressure) < dd2_friction_tolerance &&
           fabs(result.contacts[0].friction_impulse.x - impulse) < dd2_friction_tolerance &&
           fabs(bodies[0].velocity.x - speed - impulse) < dd2_friction_tolerance &&
           fabs(bodies[1].velocity.x + impulse) < dd2_friction_tolerance &&
           fabs(bodies[0].velocity.y + pressure) < dd2_friction_tolerance &&
           fabs(bodies[1].velocity.y + pressure) < dd2_friction_tolerance;
}

int main(void) {
    const double transition =
        (dd2_friction_coefficient * dd2_friction_load) + dd2_friction_micro_slip;
    const double speeds[] = {0,
                             dd2_friction_micro_slip,
                             -dd2_friction_micro_slip,
                             dd2_friction_micro_slip / 2,
                             transition,
                             transition - dd2_friction_micro_slip,
                             transition + dd2_friction_micro_slip,
                             -transition,
                             -transition - dd2_friction_micro_slip,
                             dd2_friction_load};
    for (unsigned index = 0; index < sizeof(speeds) / sizeof(speeds[0]); ++index) {
        if (!dd2_friction_analytic(speeds[index])) {
            puts("Analytic regularized friction: FAIL");
            return EXIT_FAILURE;
        }
    }
    for (unsigned index = 0; index < sizeof(dd2_friction_cases) / sizeof(dd2_friction_cases[0]);
         ++index) {
        if (!dd2_friction_run(&dd2_friction_cases[index])) {
            puts("Captured car-body friction: FAIL");
            return EXIT_FAILURE;
        }
    }
    puts("Regularized car-body friction: PASS");
    return EXIT_SUCCESS;
}
