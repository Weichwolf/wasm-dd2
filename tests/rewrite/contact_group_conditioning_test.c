#include "physics/collision_math.h"
#include "physics/contact_group.h"
#include "physics/vehicle.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_CONDITIONING_BODIES = 4,
    DD2_CONDITIONING_CONTACTS = 4,
    DD2_CONDITIONING_WIDTH = 372,
    DD2_CONDITIONING_HEIGHT = 260,
    DD2_CONDITIONING_LENGTH = 900,
    DD2_CONDITIONING_INERTIA_DIVISOR = 12,
    DD2_CONDITIONING_PASS_LIMIT = 4096
};
static const double dd2_conditioning_tolerance = 1e-6;
static const double dd2_conditioning_position_tolerance = 1e-8;
static const double dd2_conditioning_clearance = 0.0002;

/* Certified event pose from the original arena-9 natural run at tick 4427.
 * Source fleet slots 3/4/5/19 form this isolated, poorly conditioned component.
 * The previous 4096-pass iteration fails at 9.83e-6 units/s residual. */
static const dd2_vehicle dd2_conditioning_initial[DD2_CONDITIONING_BODIES] = {
    {.position = {.x = 10905.348275706736, .y = -271.1087289367623, .z = -7488.9358951720333},
     .rotation = {.x = 0.047697055519126098,
                  .y = 0.054459925845900328,
                  .z = 0.073170788689171887,
                  .w = 0.99468846532684252},
     .velocity = {.x = 200.1701373253189, .y = 36.198225717772544, .z = -19.297739965299705},
     .angular_velocity = {.x = 0.016120294663515868,
                          .y = -0.019393095366915108,
                          .z = 0.034296160066168847}},
    {.position = {.x = 10333.898517920668, .y = -407.16917345742223, .z = -6958.957955827248},
     .rotation = {.x = 0.019156682220926254,
                  .y = -0.74954434087672872,
                  .z = -0.082439555015983115,
                  .w = -0.65652115149073709},
     .velocity = {.x = 216.66079590190685, .y = 29.463845007026247, .z = -10.865827587108182},
     .angular_velocity = {.x = -0.024766850629552865,
                          .y = 0.020512188029527988,
                          .z = 0.004202278351569241}},
    {.position = {.x = 10290.267717305562, .y = -375.89168919744975, .z = -7337.4152688518143},
     .rotation = {.x = 0.019513621916155334,
                  .y = -0.7418236875506774,
                  .z = -0.084120187634701096,
                  .w = -0.66501175115988087},
     .velocity = {.x = 215.5103841953385, .y = 28.176244574741048, .z = -9.3280335934242959},
     .angular_velocity = {.x = 0.018703493835947424,
                          .y = 0.011549379927450901,
                          .z = 0.00095820950843783471}},
    {.position = {.x = 10238.398954138249, .y = -343.22667017114316, .z = -7724.2343861377867},
     .rotation = {.x = 0.017608369360429098,
                  .y = -0.72704851144136928,
                  .z = -0.084457009519168791,
                  .w = -0.68114419977155727},
     .velocity = {.x = 220.28826427135377, .y = 26.719441279171932, .z = 18.415720414743085},
     .angular_velocity = {.x = -0.027951855450486549,
                          .y = 0.072987402891137379,
                          .z = 0.03099726123683854}},
};
static const dd2_group_contact dd2_conditioning_contacts[DD2_CONDITIONING_CONTACTS] = {
    {.first = 0,
     .second = 3,
     .point = {.x = 10652.620219348124, .y = -134.26245786445804, .z = -7902.2286329645713},
     .normal = {.x = 0.98747522350519024, .y = 0.14679796791721994, .z = -0.057819024366981504},
     .penetration = 0,
     .friction = 0.25},
    {.first = 0,
     .second = 1,
     .point = {.x = 10756.2458808056, .y = -211.3412537858481, .z = -7011.5424136101046},
     .normal = {.x = 0.98046504210083008, .y = 0.15270821157478578, .z = -0.12396976783009707},
     .penetration = -0.00013878404018896617,
     .friction = 0.25},
    {.first = 0,
     .second = 2,
     .point = {.x = 10735.837909741242, .y = -195.61433468685445, .z = -7186.1403722758105},
     .normal = {.x = 0.9833603043189274, .y = 0.15075943522925295, .z = -0.10136125778197316},
     .penetration = -1.5077765283422195e-05,
     .friction = 0.25},
    {.first = 1,
     .second = 2,
     .point = {.x = 10732.032502101863, .y = -197.32292584608007, .z = -7185.7744894095858},
     .normal = {.x = 0.11475717875796461, .y = -0.082930492639076495, .z = 0.98992591809395136},
     .penetration = -4.4679289629812047e-05,
     .friction = 0.25},
};

static dd2_vehicle_vector dd2_conditioning_spin_momentum(const dd2_vehicle *body) {
    const dd2_vehicle_rotation inverse = {.x = -body->rotation.x,
                                          .y = -body->rotation.y,
                                          .z = -body->rotation.z,
                                          .w = body->rotation.w};
    const dd2_vehicle_vector spin = dd2_vehicle_rotate(inverse, body->angular_velocity);
    const double inertia_x = ((DD2_CONDITIONING_LENGTH * DD2_CONDITIONING_LENGTH) +
                              (DD2_CONDITIONING_HEIGHT * DD2_CONDITIONING_HEIGHT)) /
                             (double)DD2_CONDITIONING_INERTIA_DIVISOR;
    const double inertia_y = ((DD2_CONDITIONING_LENGTH * DD2_CONDITIONING_LENGTH) +
                              (DD2_CONDITIONING_WIDTH * DD2_CONDITIONING_WIDTH)) /
                             (double)DD2_CONDITIONING_INERTIA_DIVISOR;
    const double inertia_z = ((DD2_CONDITIONING_WIDTH * DD2_CONDITIONING_WIDTH) +
                              (DD2_CONDITIONING_HEIGHT * DD2_CONDITIONING_HEIGHT)) /
                             (double)DD2_CONDITIONING_INERTIA_DIVISOR;
    return dd2_vehicle_rotate(body->rotation, (dd2_vehicle_vector){.x = spin.x * inertia_x,
                                                                   .y = spin.y * inertia_y,
                                                                   .z = spin.z * inertia_z});
}

static double dd2_conditioning_energy(const dd2_vehicle bodies[DD2_CONDITIONING_BODIES],
                                      dd2_vehicle_vector reference) {
    double energy = 0;
    for (unsigned slot = 0; slot < DD2_CONDITIONING_BODIES; ++slot) {
        const dd2_vehicle *body = &bodies[slot];
        const dd2_vehicle_vector relative =
            dd2_collision_add(body->velocity, dd2_collision_scale(reference, -1));
        energy += dd2_collision_dot(relative, relative) +
                  dd2_collision_dot(body->angular_velocity, dd2_conditioning_spin_momentum(body));
    }
    return energy;
}

static bool dd2_conditioning_conservation(const dd2_vehicle bodies[DD2_CONDITIONING_BODIES]) {
    dd2_vehicle_vector linear = {0};
    dd2_vehicle_vector angular = {0};
    dd2_vehicle_vector offset = {0};
    dd2_vehicle_vector reference = {0};
    const dd2_vehicle_vector origin = dd2_conditioning_initial[0].position;
    for (unsigned slot = 0; slot < DD2_CONDITIONING_BODIES; ++slot) {
        const dd2_vehicle *before = &dd2_conditioning_initial[slot];
        const dd2_vehicle *after = &bodies[slot];
        const dd2_vehicle_vector delta =
            dd2_collision_add(after->velocity, dd2_collision_scale(before->velocity, -1));
        linear = dd2_collision_add(linear, delta);
        /* Position repair is impulse-free and follows response. Measure angular
         * momentum at the certified pre-repair centers, about a nearby origin. */
        const dd2_vehicle_vector arm =
            dd2_collision_add(before->position, dd2_collision_scale(origin, -1));
        angular = dd2_collision_add(angular, dd2_collision_cross(arm, delta));
        angular = dd2_collision_add(angular, dd2_conditioning_spin_momentum(after));
        angular = dd2_collision_add(
            angular, dd2_collision_scale(dd2_conditioning_spin_momentum(before), -1));
        offset = dd2_collision_add(
            offset, dd2_collision_add(after->position, dd2_collision_scale(before->position, -1)));
        reference = dd2_collision_add(reference, before->velocity);
        if (after->steps != before->steps || after->rotation.x != before->rotation.x ||
            after->rotation.y != before->rotation.y || after->rotation.z != before->rotation.z ||
            after->rotation.w != before->rotation.w) {
            return false;
        }
    }
    reference = dd2_collision_scale(reference, 1.0 / (double)DD2_CONDITIONING_BODIES);
    return sqrt(dd2_collision_dot(linear, linear)) < dd2_conditioning_tolerance &&
           sqrt(dd2_collision_dot(angular, angular)) < dd2_conditioning_tolerance &&
           sqrt(dd2_collision_dot(offset, offset)) < dd2_conditioning_position_tolerance &&
           dd2_conditioning_energy(bodies, reference) <=
               dd2_conditioning_energy(dd2_conditioning_initial, reference);
}

static bool dd2_conditioning_contact(const dd2_vehicle *bodies, dd2_group_contact contact,
                                     dd2_group_response response) {
    dd2_vehicle_vector relative = {0};
    dd2_vehicle_vector displacement = {0};
    const unsigned slots[] = {contact.first, contact.second};
    for (unsigned side = 0; side < 2; ++side) {
        const unsigned slot = slots[side];
        const dd2_vehicle_vector arm = dd2_collision_add(
            contact.point, dd2_collision_scale(dd2_conditioning_initial[slot].position, -1));
        const dd2_vehicle_vector velocity = dd2_collision_point_velocity(&bodies[slot], arm);
        const dd2_vehicle_vector offset =
            dd2_collision_add(bodies[slot].position,
                              dd2_collision_scale(dd2_conditioning_initial[slot].position, -1));
        relative = dd2_collision_add(relative, dd2_collision_scale(velocity, side == 0 ? 1 : -1));
        displacement =
            dd2_collision_add(displacement, dd2_collision_scale(offset, side == 0 ? 1 : -1));
    }
    const double normal = dd2_collision_dot(relative, contact.normal);
    const dd2_vehicle_vector tangent =
        dd2_collision_add(relative, dd2_collision_scale(contact.normal, -normal));
    const double slip = sqrt(dd2_collision_dot(tangent, tangent));
    const double friction =
        sqrt(dd2_collision_dot(response.friction_impulse, response.friction_impulse));
    const double limit = contact.friction * response.normal_impulse;
    if (response.normal_impulse < 0 || normal < -dd2_conditioning_tolerance ||
        (response.normal_impulse > 0 && fabs(normal) > dd2_conditioning_tolerance) ||
        friction > limit + dd2_conditioning_tolerance ||
        fabs(dd2_collision_dot(response.friction_impulse, contact.normal)) >
            dd2_conditioning_tolerance ||
        dd2_collision_dot(displacement, contact.normal) < contact.penetration +
                                                              dd2_conditioning_clearance -
                                                              dd2_conditioning_position_tolerance) {
        return false;
    }
    return slip <= dd2_conditioning_tolerance ||
           (fabs(friction - limit) < dd2_conditioning_tolerance &&
            fabs(dd2_collision_dot(response.friction_impulse, tangent) + (friction * slip)) <
                dd2_conditioning_tolerance);
}

int main(void) {
    dd2_vehicle bodies[DD2_CONDITIONING_BODIES] = {0};
    for (unsigned slot = 0; slot < DD2_CONDITIONING_BODIES; ++slot) {
        if (!dd2_vehicle_reset(&bodies[slot], (dd2_vehicle_spawn){0})) {
            return EXIT_FAILURE;
        }
        bodies[slot].position = dd2_conditioning_initial[slot].position;
        bodies[slot].rotation = dd2_conditioning_initial[slot].rotation;
        bodies[slot].velocity = dd2_conditioning_initial[slot].velocity;
        bodies[slot].angular_velocity = dd2_conditioning_initial[slot].angular_velocity;
    }
    const dd2_group_query query = {.bodies = bodies,
                                   .body_count = DD2_CONDITIONING_BODIES,
                                   .contacts = dd2_conditioning_contacts,
                                   .contact_count = DD2_CONDITIONING_CONTACTS};
    dd2_group_solution result = {0};
    bool valid =
        dd2_contact_group_solve(&query, &result) && result.count == DD2_CONDITIONING_CONTACTS &&
        result.velocity_passes < DD2_CONDITIONING_PASS_LIMIT && result.accelerated_passes > 0 &&
        result.rejected_extrapolations > 0 && dd2_conditioning_conservation(bodies);
    for (unsigned index = 0; valid && index < DD2_CONDITIONING_CONTACTS; ++index) {
        valid = dd2_conditioning_contact(bodies, dd2_conditioning_contacts[index],
                                         result.contacts[index]);
    }
    printf("Conditioned contact group: %s (passes=%u accepted=%u rejected=%u)\n",
           valid ? "PASS" : "FAIL", result.velocity_passes, result.accelerated_passes,
           result.rejected_extrapolations);
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
