#include "contact_friction_fixture.h"
#include "physics/collision_math.h"
#include "physics/contact_group.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_FRICTION_PASS_LIMIT = 4096,
    DD2_FRICTION_PAIR = 2,
    DD2_FRICTION_COUPLED_CONTACTS = 7,
    DD2_FRICTION_GROUND_CONTACTS = 4,
    DD2_FRICTION_REFINEMENT_CONTACTS = 8,
    DD2_FRICTION_SLIDING_CONTACTS = 5,
    DD2_FRICTION_SELECTIVE_BODIES = 4,
    DD2_FRICTION_SELECTIVE_CONTACTS = 10,
    DD2_FRICTION_LOAD_CONTACTS = 3,
    DD2_FRICTION_LINEAR_CONTACTS = 3,
    DD2_FRICTION_RELEASE_BODIES = 3,
    DD2_FRICTION_RELEASE_CONTACTS = 5,
    DD2_FRICTION_MIXED_RELEASE_BODIES = 6,
    DD2_FRICTION_MIXED_RELEASE_CONTACTS = 11,
    DD2_FRICTION_PATCH_BODIES = 4,
    DD2_FRICTION_PATCH_CONTACTS = 10,
    DD2_FRICTION_ENDPOINT_BODIES = 6,
    DD2_FRICTION_ENDPOINT_CONTACTS = 14,
    DD2_FRICTION_FITTED_CONTACTS = 3,
    DD2_FRICTION_MERIT_CONTACTS = 5,
    DD2_FRICTION_DENSE_CONTACTS = 20,
    DD2_FRICTION_DENSE_BODIES = 7,
    DD2_FRICTION_COLD_CONTACTS = 4,
    DD2_FRICTION_SLIDING_LOAD_CONTACTS = 11,
    DD2_FRICTION_SLIDING_LOAD_BODIES = 5,
    DD2_FRICTION_FIRST_PHASE_CONTACTS = 8,
    DD2_FRICTION_FIRST_PHASE_BODIES = 3,
    DD2_FRICTION_FIXED_ACTIVE_CONTACTS = 14,
    DD2_FRICTION_FIXED_ACTIVE_BODIES = 5
};
static const double dd2_friction_tolerance = 1e-6;
static const double dd2_friction_position_tolerance = 1e-8;
static const double dd2_friction_clearance = 1e-4;
static const double dd2_friction_micro_slip = 0.1;
static const double dd2_friction_max_softness = 1e12;
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
    /* The constitutive oracle checks final velocity directly, rather than
     * the solver's projected-gradient residual or its iteration history. */
    const dd2_vehicle_vector residual = dd2_collision_add(
        slip,
        dd2_collision_scale(
            response.friction_impulse,
            fmax(fmin(dd2_friction_micro_slip, dd2_friction_max_softness * limit), speed) / limit));
    return dd2_friction_length(residual) < dd2_friction_tolerance;
}

static bool dd2_friction_capped_oracle(void) {
    /* Exercise below, at and above the existing 1e-13 cone-radius cap.
     * Both linear and saturated branches must oppose slip; omitting the cap
     * must fail the below-cap case rather than silently relaxing its oracle. */
    const double limits[] = {1e-14, 1e-13, 1e-12};
    for (unsigned index = 0; index < sizeof(limits) / sizeof(limits[0]); ++index) {
        const double limit = limits[index];
        const double transition = fmin(dd2_friction_micro_slip, dd2_friction_max_softness * limit);
        const dd2_group_contact contact = {.normal = {.y = 1},
                                           .friction = dd2_friction_coefficient};
        for (unsigned saturated = 0; saturated < 2; ++saturated) {
            const double speed = transition * (saturated != 0 ? 2 : 0.5);
            const double impulse = limit * (saturated != 0 ? 1 : 0.5);
            for (int direction = -1; direction <= 1; direction += 2) {
                const dd2_vehicle_vector slip = {.x = direction * speed};
                dd2_group_response response = {.normal_impulse = limit / dd2_friction_coefficient,
                                               .friction_impulse = {.x = -direction * impulse}};
                if (!dd2_friction_law(slip, response, contact)) {
                    return false;
                }
                response.friction_impulse.x = -response.friction_impulse.x;
                if (dd2_friction_law(slip, response, contact)) {
                    return false;
                }
            }
        }
    }
    const dd2_group_contact contact = {.normal = {.y = 1}, .friction = dd2_friction_coefficient};
    const dd2_vehicle_vector slip = {.x = 0.005};
    const dd2_group_response uncapped = {
        .normal_impulse = limits[0] / dd2_friction_coefficient,
        .friction_impulse = {.x = -limits[0] * slip.x / dd2_friction_micro_slip}};
    return !dd2_friction_law(slip, uncapped, contact);
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
        result.position_passes > DD2_FRICTION_PASS_LIMIT ||
        result.position_predictions > result.position_passes || result.coordinate_restarts > 1 ||
        (scenario->require_position_prediction && result.position_predictions == 0) ||
        (scenario->require_restart && result.coordinate_restarts != 1) ||
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
    printf("Car-body friction %s: PASS (contacts=%u passes=%u restarts=%u position_passes=%u "
           "position_predictions=%u)\n",
           scenario->name, result.count, result.velocity_passes, result.coordinate_restarts,
           result.position_passes, result.position_predictions);
    return true;
}

static bool dd2_friction_analytic(double speed, bool pair) {
    dd2_vehicle bodies[DD2_FRICTION_PAIR] = {0};
    for (unsigned slot = 0; slot < DD2_FRICTION_PAIR; ++slot) {
        if (!dd2_vehicle_reset(&bodies[slot], (dd2_vehicle_spawn){0})) {
            return false;
        }
    }
    bodies[0].velocity = (dd2_vehicle_vector){.x = speed, .y = -dd2_friction_load};
    const dd2_group_contact contact = {.first = 0,
                                       .second = pair ? 1 : DD2_VEHICLE_NO_PARTNER,
                                       .normal = {.y = 1},
                                       .friction = dd2_friction_coefficient};
    dd2_group_solution result = {0};
    const dd2_group_query query = {.bodies = bodies,
                                   .body_count = pair ? DD2_FRICTION_PAIR : 1,
                                   .contacts = &contact,
                                   .contact_count = 1};
    if (!dd2_contact_group_solve(&query, &result)) {
        return false;
    }
    const double count = pair ? (double)DD2_FRICTION_PAIR : 1;
    const double pressure = dd2_friction_load / count;
    const double limit = dd2_friction_coefficient * pressure;
    const double impulse = copysign(
        fmin(limit, fabs(speed) * limit / ((count * limit) + dd2_friction_micro_slip)), -speed);
    return fabs(result.contacts[0].normal_impulse - pressure) < dd2_friction_tolerance &&
           fabs(result.contacts[0].friction_impulse.x - impulse) < dd2_friction_tolerance &&
           fabs(bodies[0].velocity.x - speed - impulse) < dd2_friction_tolerance &&
           fabs(bodies[1].velocity.x + (pair ? impulse : 0)) < dd2_friction_tolerance &&
           fabs(bodies[0].velocity.y + dd2_friction_load - pressure) < dd2_friction_tolerance &&
           fabs(bodies[1].velocity.y + (pair ? pressure : 0)) < dd2_friction_tolerance;
}

/* Redundant supports must not require the captured row ordering to converge.
 * Reorder the same equations without changing geometry or physical acceptance. */
static bool dd2_friction_permutations(bool position) {
    for (unsigned reversed = 0; reversed < DD2_FRICTION_PAIR; ++reversed) {
        for (unsigned start = 0; start < DD2_FRICTION_COUPLED_CONTACTS; ++start) {
            dd2_group_contact contacts[DD2_FRICTION_COUPLED_CONTACTS] = {0};
            for (unsigned index = 0; index < DD2_FRICTION_COUPLED_CONTACTS; ++index) {
                const unsigned offset =
                    reversed != 0 ? DD2_FRICTION_COUPLED_CONTACTS - index : index;
                contacts[index] = (position ? dd2_friction_championship_position_contacts
                                            : dd2_friction_championship_coupled_contacts)
                    [(start + offset) % DD2_FRICTION_COUPLED_CONTACTS];
            }
            const dd2_friction_case scenario = {
                .initial = position ? dd2_friction_championship_position_bodies
                                    : dd2_friction_championship_coupled_bodies,
                .contacts = contacts,
                .body_count = DD2_FRICTION_PAIR,
                .contact_count = DD2_FRICTION_COUPLED_CONTACTS,
                .name = position ? "championship position support ordering"
                                 : "championship coupled support ordering"};
            printf("Coupled support ordering: position=%u start=%u reversed=%u\n",
                   (unsigned)position, start, reversed);
            if (!dd2_friction_run(&scenario)) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_friction_order_next(unsigned *order, unsigned count) {
    unsigned pivot = count - 1;
    while (pivot > 0 && order[pivot - 1] >= order[pivot]) {
        --pivot;
    }
    if (pivot == 0) {
        return false;
    }
    unsigned successor = count - 1;
    while (order[successor] <= order[pivot - 1]) {
        --successor;
    }
    const unsigned saved = order[pivot - 1];
    order[pivot - 1] = order[successor];
    order[successor] = saved;
    for (unsigned left = pivot, right = count - 1; left < right; ++left, --right) {
        const unsigned value = order[left];
        order[left] = order[right];
        order[right] = value;
    }
    return true;
}

static bool dd2_friction_ground_orderings(void) {
    unsigned order[DD2_FRICTION_GROUND_CONTACTS] = {0};
    for (unsigned index = 0; index < DD2_FRICTION_GROUND_CONTACTS; ++index) {
        order[index] = index;
    }
    do {
        dd2_group_contact contacts[DD2_FRICTION_GROUND_CONTACTS] = {0};
        for (unsigned index = 0; index < DD2_FRICTION_GROUND_CONTACTS; ++index) {
            contacts[index] = dd2_friction_championship_ground_contacts[order[index]];
        }
        const dd2_friction_case scenario = {.initial = dd2_friction_championship_ground_bodies,
                                            .contacts = contacts,
                                            .body_count = DD2_FRICTION_PAIR,
                                            .contact_count = DD2_FRICTION_GROUND_CONTACTS,
                                            .name = "championship ground support ordering"};
        if (!dd2_friction_run(&scenario)) {
            return false;
        }
    } while (dd2_friction_order_next(order, DD2_FRICTION_GROUND_CONTACTS));
    return true;
}

static bool dd2_friction_refinement_orderings(void) {
    for (unsigned reversed = 0; reversed < DD2_FRICTION_PAIR; ++reversed) {
        for (unsigned start = 0; start < DD2_FRICTION_REFINEMENT_CONTACTS; ++start) {
            dd2_group_contact contacts[DD2_FRICTION_REFINEMENT_CONTACTS] = {0};
            for (unsigned index = 0; index < DD2_FRICTION_REFINEMENT_CONTACTS; ++index) {
                const unsigned offset =
                    reversed != 0 ? DD2_FRICTION_REFINEMENT_CONTACTS - index : index;
                contacts[index] =
                    dd2_friction_championship_refinement_contacts[(start + offset) %
                                                                  DD2_FRICTION_REFINEMENT_CONTACTS];
            }
            const dd2_friction_case scenario = {.initial =
                                                    dd2_friction_championship_refinement_bodies,
                                                .contacts = contacts,
                                                .body_count = DD2_FRICTION_PAIR,
                                                .contact_count = DD2_FRICTION_REFINEMENT_CONTACTS,
                                                .name = "championship refinement support ordering"};
            printf("Refinement support ordering: start=%u reversed=%u\n", start, reversed);
            if (!dd2_friction_run(&scenario)) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_friction_sliding_orderings(void) {
    unsigned order[DD2_FRICTION_SLIDING_CONTACTS] = {0};
    for (unsigned index = 0; index < DD2_FRICTION_SLIDING_CONTACTS; ++index) {
        order[index] = index;
    }
    do {
        dd2_group_contact contacts[DD2_FRICTION_SLIDING_CONTACTS] = {0};
        for (unsigned index = 0; index < DD2_FRICTION_SLIDING_CONTACTS; ++index) {
            contacts[index] = dd2_friction_championship_sliding_contacts[order[index]];
        }
        const dd2_friction_case scenario = {.initial = dd2_friction_championship_sliding_bodies,
                                            .contacts = contacts,
                                            .body_count = DD2_FRICTION_PAIR,
                                            .contact_count = DD2_FRICTION_SLIDING_CONTACTS,
                                            .name = "championship sliding branch support ordering"};
        if (!dd2_friction_run(&scenario)) {
            return false;
        }
    } while (dd2_friction_order_next(order, DD2_FRICTION_SLIDING_CONTACTS));
    return true;
}

static bool dd2_friction_fitted_orderings(void) {
    unsigned order[DD2_FRICTION_FITTED_CONTACTS] = {0};
    for (unsigned index = 0; index < DD2_FRICTION_FITTED_CONTACTS; ++index) {
        order[index] = index;
    }
    do {
        dd2_group_contact contacts[DD2_FRICTION_FITTED_CONTACTS] = {0};
        for (unsigned index = 0; index < DD2_FRICTION_FITTED_CONTACTS; ++index) {
            contacts[index] = dd2_friction_championship_fitted_contacts[order[index]];
        }
        const dd2_friction_case scenario = {.initial = dd2_friction_championship_fitted_bodies,
                                            .contacts = contacts,
                                            .body_count = 1,
                                            .contact_count = DD2_FRICTION_FITTED_CONTACTS,
                                            .name = "championship fitted release ordering"};
        if (!dd2_friction_run(&scenario)) {
            return false;
        }
    } while (dd2_friction_order_next(order, DD2_FRICTION_FITTED_CONTACTS));
    return true;
}

static bool dd2_friction_release_orderings(void) {
    unsigned order[DD2_FRICTION_RELEASE_CONTACTS] = {0};
    for (unsigned index = 0; index < DD2_FRICTION_RELEASE_CONTACTS; ++index) {
        order[index] = index;
    }
    do {
        dd2_group_contact contacts[DD2_FRICTION_RELEASE_CONTACTS] = {0};
        for (unsigned index = 0; index < DD2_FRICTION_RELEASE_CONTACTS; ++index) {
            contacts[index] = dd2_friction_championship_release_contacts[order[index]];
        }
        const dd2_friction_case scenario = {.initial = dd2_friction_championship_release_bodies,
                                            .contacts = contacts,
                                            .body_count = DD2_FRICTION_RELEASE_BODIES,
                                            .contact_count = DD2_FRICTION_RELEASE_CONTACTS,
                                            .name = "championship released world support ordering"};
        if (!dd2_friction_run(&scenario)) {
            return false;
        }
    } while (dd2_friction_order_next(order, DD2_FRICTION_RELEASE_CONTACTS));
    return true;
}

static bool dd2_friction_selective_orderings(void) {
    for (unsigned reversed = 0; reversed < DD2_FRICTION_PAIR; ++reversed) {
        for (unsigned start = 0; start < DD2_FRICTION_SELECTIVE_CONTACTS; ++start) {
            dd2_group_contact contacts[DD2_FRICTION_SELECTIVE_CONTACTS] = {0};
            for (unsigned index = 0; index < DD2_FRICTION_SELECTIVE_CONTACTS; ++index) {
                const unsigned offset =
                    reversed != 0 ? DD2_FRICTION_SELECTIVE_CONTACTS - index : index;
                contacts[index] =
                    dd2_friction_championship_selective_contacts[(start + offset) %
                                                                 DD2_FRICTION_SELECTIVE_CONTACTS];
            }
            const dd2_friction_case scenario = {
                .initial = dd2_friction_championship_selective_bodies,
                .contacts = contacts,
                .body_count = DD2_FRICTION_SELECTIVE_BODIES,
                .contact_count = DD2_FRICTION_SELECTIVE_CONTACTS,
                .name = "championship selective branch support ordering"};
            if (!dd2_friction_run(&scenario)) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_friction_load_orderings(void) {
    for (unsigned reversed = 0; reversed < DD2_FRICTION_PAIR; ++reversed) {
        for (unsigned start = 0; start < DD2_FRICTION_LOAD_CONTACTS; ++start) {
            dd2_group_contact contacts[DD2_FRICTION_LOAD_CONTACTS] = {0};
            for (unsigned index = 0; index < DD2_FRICTION_LOAD_CONTACTS; ++index) {
                const unsigned offset = reversed != 0 ? DD2_FRICTION_LOAD_CONTACTS - index : index;
                contacts[index] =
                    dd2_friction_championship_load_contacts[(start + offset) %
                                                            DD2_FRICTION_LOAD_CONTACTS];
            }
            const dd2_friction_case scenario = {.initial = dd2_friction_championship_load_body,
                                                .contacts = contacts,
                                                .body_count = 1,
                                                .contact_count = DD2_FRICTION_LOAD_CONTACTS,
                                                .name =
                                                    "championship positive load support ordering"};
            if (!dd2_friction_run(&scenario)) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_friction_linear_orderings(void) {
    for (unsigned reversed = 0; reversed < DD2_FRICTION_PAIR; ++reversed) {
        for (unsigned start = 0; start < DD2_FRICTION_LINEAR_CONTACTS; ++start) {
            dd2_group_contact contacts[DD2_FRICTION_LINEAR_CONTACTS] = {0};
            for (unsigned index = 0; index < DD2_FRICTION_LINEAR_CONTACTS; ++index) {
                const unsigned offset =
                    reversed != 0 ? DD2_FRICTION_LINEAR_CONTACTS - index : index;
                contacts[index] =
                    dd2_friction_championship_linear_contacts[(start + offset) %
                                                              DD2_FRICTION_LINEAR_CONTACTS];
            }
            const dd2_friction_case scenario = {
                .initial = dd2_friction_championship_linear_body,
                .contacts = contacts,
                .body_count = 1,
                .contact_count = DD2_FRICTION_LINEAR_CONTACTS,
                .name = "championship sliding-to-linear world support ordering"};
            if (!dd2_friction_run(&scenario)) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_friction_mixed_release_orderings(void) {
    for (unsigned reversed = 0; reversed < DD2_FRICTION_PAIR; ++reversed) {
        for (unsigned start = 0; start < DD2_FRICTION_MIXED_RELEASE_CONTACTS; ++start) {
            dd2_group_contact contacts[DD2_FRICTION_MIXED_RELEASE_CONTACTS] = {0};
            for (unsigned index = 0; index < DD2_FRICTION_MIXED_RELEASE_CONTACTS; ++index) {
                const unsigned offset =
                    reversed != 0 ? DD2_FRICTION_MIXED_RELEASE_CONTACTS - index : index;
                contacts[index] = dd2_friction_championship_mixed_release_contacts
                    [(start + offset) % DD2_FRICTION_MIXED_RELEASE_CONTACTS];
            }
            const dd2_friction_case scenario = {
                .initial = dd2_friction_championship_mixed_release_bodies,
                .contacts = contacts,
                .body_count = DD2_FRICTION_MIXED_RELEASE_BODIES,
                .contact_count = DD2_FRICTION_MIXED_RELEASE_CONTACTS,
                .name = "championship mixed released support ordering"};
            if (!dd2_friction_run(&scenario)) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_friction_patch_orderings(void) {
    for (unsigned reversed = 0; reversed < DD2_FRICTION_PAIR; ++reversed) {
        for (unsigned start = 0; start < DD2_FRICTION_PATCH_CONTACTS; ++start) {
            dd2_group_contact contacts[DD2_FRICTION_PATCH_CONTACTS] = {0};
            for (unsigned index = 0; index < DD2_FRICTION_PATCH_CONTACTS; ++index) {
                const unsigned offset = reversed != 0 ? DD2_FRICTION_PATCH_CONTACTS - index : index;
                contacts[index] =
                    dd2_friction_championship_patch_contacts[(start + offset) %
                                                             DD2_FRICTION_PATCH_CONTACTS];
            }
            const dd2_friction_case scenario = {.initial = dd2_friction_championship_patch_bodies,
                                                .contacts = contacts,
                                                .body_count = DD2_FRICTION_PATCH_BODIES,
                                                .contact_count = DD2_FRICTION_PATCH_CONTACTS,
                                                .name =
                                                    "championship co-oriented wall patch ordering"};
            if (!dd2_friction_run(&scenario)) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_friction_endpoint_orderings(void) {
    for (unsigned reversed = 0; reversed < DD2_FRICTION_PAIR; ++reversed) {
        for (unsigned start = 0; start < DD2_FRICTION_ENDPOINT_CONTACTS; ++start) {
            dd2_group_contact contacts[DD2_FRICTION_ENDPOINT_CONTACTS] = {0};
            for (unsigned index = 0; index < DD2_FRICTION_ENDPOINT_CONTACTS; ++index) {
                const unsigned offset =
                    reversed != 0 ? DD2_FRICTION_ENDPOINT_CONTACTS - index : index;
                contacts[index] =
                    dd2_friction_championship_endpoint_contacts[(start + offset) %
                                                                DD2_FRICTION_ENDPOINT_CONTACTS];
            }
            const dd2_friction_case scenario = {
                .initial = dd2_friction_championship_endpoint_bodies,
                .contacts = contacts,
                .body_count = DD2_FRICTION_ENDPOINT_BODIES,
                .contact_count = DD2_FRICTION_ENDPOINT_CONTACTS,
                .name = "championship two retained wall supports ordering"};
            if (!dd2_friction_run(&scenario)) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_friction_merit_orderings(void) {
    unsigned order[DD2_FRICTION_MERIT_CONTACTS] = {0};
    for (unsigned index = 0; index < DD2_FRICTION_MERIT_CONTACTS; ++index) {
        order[index] = index;
    }
    do {
        dd2_group_contact contacts[DD2_FRICTION_MERIT_CONTACTS] = {0};
        for (unsigned index = 0; index < DD2_FRICTION_MERIT_CONTACTS; ++index) {
            contacts[index] = dd2_friction_championship_merit_contacts[order[index]];
        }
        const dd2_friction_case scenario = {
            .initial = dd2_friction_championship_merit_bodies,
            .contacts = contacts,
            .body_count = DD2_FRICTION_PAIR,
            .contact_count = DD2_FRICTION_MERIT_CONTACTS,
            .name = "championship private constitutive release merit ordering"};
        if (!dd2_friction_run(&scenario)) {
            return false;
        }
    } while (dd2_friction_order_next(order, DD2_FRICTION_MERIT_CONTACTS));
    return true;
}

static bool dd2_friction_cold_orderings(void) {
    unsigned order[DD2_FRICTION_COLD_CONTACTS] = {0};
    for (unsigned index = 0; index < DD2_FRICTION_COLD_CONTACTS; ++index) {
        order[index] = index;
    }
    do {
        dd2_group_contact contacts[DD2_FRICTION_COLD_CONTACTS] = {0};
        for (unsigned index = 0; index < DD2_FRICTION_COLD_CONTACTS; ++index) {
            contacts[index] = dd2_friction_cold_contacts[order[index]];
        }
        const dd2_friction_case scenario = {.initial = dd2_friction_cold_bodies,
                                            .contacts = contacts,
                                            .body_count = DD2_FRICTION_PAIR,
                                            .contact_count = DD2_FRICTION_COLD_CONTACTS,
                                            .name = "championship cold constitutive ordering"};
        if (!dd2_friction_run(&scenario)) {
            return false;
        }
    } while (dd2_friction_order_next(order, DD2_FRICTION_COLD_CONTACTS));
    return true;
}

static bool dd2_friction_dense_orderings(void) {
    for (unsigned reversed = 0; reversed < 2; ++reversed) {
        for (unsigned start = 0; start < DD2_FRICTION_DENSE_CONTACTS; ++start) {
            dd2_group_contact contacts[DD2_FRICTION_DENSE_CONTACTS] = {0};
            for (unsigned index = 0; index < DD2_FRICTION_DENSE_CONTACTS; ++index) {
                contacts[index] = dd2_friction_dense_contacts
                    [(start + (reversed != 0 ? DD2_FRICTION_DENSE_CONTACTS - index : index)) %
                     DD2_FRICTION_DENSE_CONTACTS];
            }
            const dd2_friction_case scenario = {.initial = dd2_friction_dense_bodies,
                                                .contacts = contacts,
                                                .body_count = DD2_FRICTION_DENSE_BODIES,
                                                .contact_count = DD2_FRICTION_DENSE_CONTACTS,
                                                .name = "natural dense support ordering"};
            if (!dd2_friction_run(&scenario)) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_friction_sliding_load_orderings(void) {
    for (unsigned reversed = 0; reversed < DD2_FRICTION_PAIR; ++reversed) {
        for (unsigned start = 0; start < DD2_FRICTION_SLIDING_LOAD_CONTACTS; ++start) {
            dd2_group_contact contacts[DD2_FRICTION_SLIDING_LOAD_CONTACTS] = {0};
            for (unsigned index = 0; index < DD2_FRICTION_SLIDING_LOAD_CONTACTS; ++index) {
                const unsigned offset =
                    reversed != 0 ? DD2_FRICTION_SLIDING_LOAD_CONTACTS - index : index;
                contacts[index] =
                    dd2_friction_sliding_load_contacts[(start + offset) %
                                                       DD2_FRICTION_SLIDING_LOAD_CONTACTS];
            }
            const dd2_friction_case scenario = {.initial = dd2_friction_sliding_load_bodies,
                                                .contacts = contacts,
                                                .body_count = DD2_FRICTION_SLIDING_LOAD_BODIES,
                                                .contact_count = DD2_FRICTION_SLIDING_LOAD_CONTACTS,
                                                .name =
                                                    "championship sliding world pressure ordering"};
            if (!dd2_friction_run(&scenario)) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_friction_first_phase_orderings(void) {
    for (unsigned reversed = 0; reversed < DD2_FRICTION_PAIR; ++reversed) {
        for (unsigned start = 0; start < DD2_FRICTION_FIRST_PHASE_CONTACTS; ++start) {
            dd2_group_contact contacts[DD2_FRICTION_FIRST_PHASE_CONTACTS] = {0};
            for (unsigned index = 0; index < DD2_FRICTION_FIRST_PHASE_CONTACTS; ++index) {
                const unsigned offset =
                    reversed != 0 ? DD2_FRICTION_FIRST_PHASE_CONTACTS - index : index;
                contacts[index] =
                    dd2_friction_first_phase_contacts[(start + offset) %
                                                      DD2_FRICTION_FIRST_PHASE_CONTACTS];
            }
            const dd2_friction_case scenario = {.initial = dd2_friction_first_phase_bodies,
                                                .contacts = contacts,
                                                .body_count = DD2_FRICTION_FIRST_PHASE_BODIES,
                                                .contact_count = DD2_FRICTION_FIRST_PHASE_CONTACTS,
                                                .name =
                                                    "championship initial contact phase ordering"};
            if (!dd2_friction_run(&scenario)) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_friction_fixed_active_orderings(void) {
    for (unsigned reversed = 0; reversed < DD2_FRICTION_PAIR; ++reversed) {
        for (unsigned start = 0; start < DD2_FRICTION_FIXED_ACTIVE_CONTACTS; ++start) {
            dd2_group_contact contacts[DD2_FRICTION_FIXED_ACTIVE_CONTACTS] = {0};
            for (unsigned index = 0; index < DD2_FRICTION_FIXED_ACTIVE_CONTACTS; ++index) {
                const unsigned offset =
                    reversed != 0 ? DD2_FRICTION_FIXED_ACTIVE_CONTACTS - index : index;
                contacts[index] =
                    dd2_friction_fixed_active_contacts[(start + offset) %
                                                       DD2_FRICTION_FIXED_ACTIVE_CONTACTS];
            }
            const dd2_friction_case scenario = {
                .initial = dd2_friction_fixed_active_bodies,
                .contacts = contacts,
                .body_count = DD2_FRICTION_FIXED_ACTIVE_BODIES,
                .contact_count = DD2_FRICTION_FIXED_ACTIVE_CONTACTS,
                .name = "championship fixed normal active set ordering"};
            if (!dd2_friction_run(&scenario)) {
                return false;
            }
        }
    }
    return true;
}

int main(void) {
    if (!dd2_friction_capped_oracle()) {
        puts("Capped material oracle: FAIL");
        return EXIT_FAILURE;
    }
    puts("Capped material oracle: PASS");
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
        if (!dd2_friction_analytic(speeds[index], false) ||
            !dd2_friction_analytic(speeds[index], true)) {
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
    if (!dd2_friction_permutations(false) || !dd2_friction_permutations(true) ||
        !dd2_friction_ground_orderings() || !dd2_friction_refinement_orderings() ||
        !dd2_friction_sliding_orderings() || !dd2_friction_selective_orderings() ||
        !dd2_friction_load_orderings() || !dd2_friction_release_orderings() ||
        !dd2_friction_linear_orderings() || !dd2_friction_mixed_release_orderings() ||
        !dd2_friction_patch_orderings() || !dd2_friction_endpoint_orderings() ||
        !dd2_friction_fitted_orderings() || !dd2_friction_merit_orderings() ||
        !dd2_friction_dense_orderings() || !dd2_friction_cold_orderings() ||
        !dd2_friction_sliding_load_orderings() || !dd2_friction_first_phase_orderings() ||
        !dd2_friction_fixed_active_orderings()) {
        puts("Coupled support ordering: FAIL");
        return EXIT_FAILURE;
    }
    puts("Regularized car-body friction: PASS");
    return EXIT_SUCCESS;
}
