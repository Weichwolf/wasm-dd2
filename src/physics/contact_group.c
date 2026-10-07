#include "physics/contact_group.h"

#include "physics/collision_math.h"
#include "physics/numeric.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <float.h>
#include <math.h>
#include <stddef.h>

enum {
    DD2_GROUP_PASSES = 4096,
    DD2_GROUP_ACCELERATION_PASSES = DD2_GROUP_PASSES / 2,
    DD2_GROUP_STALLED_PASSES = 64
};
static const double dd2_group_clearance = 1e-4;
static const double dd2_group_progress_fraction = 0.999;
static const double dd2_group_velocity_tolerance = 1e-7;
static const double dd2_group_position_tolerance = 1e-9;
static const double dd2_group_active_tolerance = 1e-10;
static const double dd2_group_position_relaxation = 1.8;
static const double dd2_group_axis_tolerance = 1e-8;
static const double dd2_group_secant_floor = 1e-30;
static const double dd2_group_micro_slip = 0.1;
static const double dd2_group_max_friction_softness = 1e12;

typedef struct {
    dd2_vehicle_vector arms[2];
    dd2_vehicle_vector friction;
    double normal_mass;
    double tangent_mass;
    double normal_impulse;
    double position_impulse;
    double initial_normal_speed;
} dd2_group_constraint;

typedef struct {
    const dd2_group_query *query;
    dd2_group_constraint constraints[DD2_VEHICLE_CONTACT_LIMIT];
} dd2_group_workspace;

static bool dd2_group_vector_finite(const dd2_vehicle_vector *value) {
    return dd2_numeric_finite(&value->x) && dd2_numeric_finite(&value->y) &&
           dd2_numeric_finite(&value->z);
}

static bool dd2_group_pair(dd2_group_contact contact) {
    return contact.second != DD2_VEHICLE_NO_PARTNER;
}

static bool dd2_group_contact_valid(const dd2_group_contact *contact, unsigned count) {
    return contact->first < count &&
           (contact->second == DD2_VEHICLE_NO_PARTNER ||
            (contact->second < count && contact->first != contact->second)) &&
           dd2_group_vector_finite(&contact->point) && dd2_group_vector_finite(&contact->normal) &&
           fabs(dd2_collision_dot(contact->normal, contact->normal) - 1) <
               dd2_group_axis_tolerance &&
           dd2_numeric_finite(&contact->penetration) && dd2_numeric_finite(&contact->friction) &&
           contact->friction >= 0 && contact->friction <= 1;
}

static bool dd2_group_body_valid(const dd2_vehicle *body) {
    if (!dd2_group_vector_finite(&body->velocity) ||
        !dd2_group_vector_finite(&body->angular_velocity)) {
        return false;
    }
    /* A primary corner impulse can temporarily exceed integrator speed/spin
     * bounds before the other supports absorb it. Validate its pose and finite
     * motion here, then enforce full vehicle limits after joint response. */
    dd2_vehicle pose = *body;
    pose.velocity = (dd2_vehicle_vector){0};
    pose.angular_velocity = (dd2_vehicle_vector){0};
    return dd2_vehicle_valid(&pose);
}

static bool dd2_group_validate(const dd2_group_query *query) {
    if (query == NULL || query->bodies == NULL || query->contacts == NULL ||
        query->body_count == 0 || query->body_count > DD2_VEHICLE_FLEET_LIMIT ||
        query->contact_count == 0 || query->contact_count > DD2_VEHICLE_CONTACT_LIMIT) {
        return false;
    }
    for (unsigned body = 0; body < query->body_count; ++body) {
        if (!dd2_group_body_valid(&query->bodies[body])) {
            return false;
        }
    }
    for (unsigned index = 0; index < query->contact_count; ++index) {
        if (!dd2_group_contact_valid(&query->contacts[index], query->body_count)) {
            return false;
        }
    }
    return true;
}

static dd2_vehicle_vector dd2_group_velocity(const dd2_group_workspace *workspace, unsigned index) {
    const dd2_group_contact contact = workspace->query->contacts[index];
    const dd2_group_constraint *constraint = &workspace->constraints[index];
    const dd2_vehicle *bodies = workspace->query->bodies;
    dd2_vehicle_vector velocity =
        dd2_collision_point_velocity(&bodies[contact.first], constraint->arms[0]);
    if (dd2_group_pair(contact)) {
        velocity = dd2_collision_add(
            velocity,
            dd2_collision_scale(
                dd2_collision_point_velocity(&bodies[contact.second], constraint->arms[1]), -1));
    }
    return velocity;
}

static double dd2_group_mass(const dd2_group_workspace *workspace, unsigned index,
                             dd2_vehicle_vector axis) {
    const dd2_group_contact contact = workspace->query->contacts[index];
    const dd2_group_constraint *constraint = &workspace->constraints[index];
    const dd2_vehicle *bodies = workspace->query->bodies;
    double mass = dd2_collision_effective_mass(&bodies[contact.first], constraint->arms[0], axis);
    if (dd2_group_pair(contact)) {
        mass += dd2_collision_effective_mass(&bodies[contact.second], constraint->arms[1], axis);
    }
    return mass;
}

static double dd2_group_tangent_mass(const dd2_group_workspace *workspace, unsigned index) {
    const dd2_vehicle_vector normal = workspace->query->contacts[index].normal;
    const dd2_vehicle_vector reference =
        fabs(normal.x) < 0.5 ? (dd2_vehicle_vector){.x = 1} : (dd2_vehicle_vector){.y = 1};
    dd2_vehicle_vector tangent = dd2_collision_cross(normal, reference);
    tangent = dd2_collision_scale(tangent, 1 / sqrt(dd2_collision_dot(tangent, tangent)));
    const dd2_vehicle_vector bitangent = dd2_collision_cross(normal, tangent);
    /* The trace of the positive tangential mobility matrix bounds its largest
     * eigenvalue. A fixed inverse-trace gradient step cannot overshoot a stiff
     * tangent axis when the current slip points along a softer axis. */
    return dd2_group_mass(workspace, index, tangent) + dd2_group_mass(workspace, index, bitangent);
}

static bool dd2_group_prepare(dd2_group_workspace *workspace) {
    const dd2_group_query *query = workspace->query;
    for (unsigned index = 0; index < query->contact_count; ++index) {
        const dd2_group_contact contact = query->contacts[index];
        dd2_group_constraint *constraint = &workspace->constraints[index];
        constraint->arms[0] = dd2_collision_add(
            contact.point, dd2_collision_scale(query->bodies[contact.first].position, -1));
        if (dd2_group_pair(contact)) {
            constraint->arms[1] = dd2_collision_add(
                contact.point, dd2_collision_scale(query->bodies[contact.second].position, -1));
        }
        constraint->normal_mass = dd2_group_mass(workspace, index, contact.normal);
        constraint->tangent_mass = dd2_group_tangent_mass(workspace, index);
        constraint->initial_normal_speed =
            dd2_collision_dot(dd2_group_velocity(workspace, index), contact.normal);
        if (!dd2_numeric_finite(&constraint->normal_mass) || constraint->normal_mass <= 0 ||
            !dd2_numeric_finite(&constraint->tangent_mass) || constraint->tangent_mass <= 0 ||
            !dd2_numeric_finite(&constraint->initial_normal_speed)) {
            return false;
        }
    }
    return true;
}

static void dd2_group_apply(dd2_group_workspace *workspace, unsigned index,
                            dd2_vehicle_vector impulse) {
    const dd2_group_contact contact = workspace->query->contacts[index];
    const dd2_group_constraint *constraint = &workspace->constraints[index];
    dd2_vehicle *bodies = workspace->query->bodies;
    dd2_collision_impulse(&bodies[contact.first], constraint->arms[0], impulse);
    if (dd2_group_pair(contact)) {
        dd2_collision_impulse(&bodies[contact.second], constraint->arms[1],
                              dd2_collision_scale(impulse, -1));
    }
}

typedef struct {
    dd2_vehicle_vector impulse;
    double mass;
} dd2_group_friction;

static dd2_group_friction dd2_group_friction_candidate(const dd2_group_workspace *workspace,
                                                       unsigned index) {
    const dd2_group_contact contact = workspace->query->contacts[index];
    const dd2_group_constraint *constraint = &workspace->constraints[index];
    const double limit = contact.friction * constraint->normal_impulse;
    dd2_group_friction candidate = {.mass = constraint->tangent_mass};
    if (limit == 0) {
        return candidate;
    }
    /* Implicit regularized car-body friction: below 0.1 world units/s, the
     * opposing impulse grows linearly with slip; above it, Coulomb saturation
     * is unchanged. The tiny creep avoids ambiguous stick/slip branches in
     * tightly coupled bodies. Static world supports retain exact sticking.
     * The softness cap keeps vanishing pressure finite; its maximum affected
     * cone radius is only 1e-13 impulse units. */
    const double softness =
        dd2_group_pair(contact)
            ? dd2_group_micro_slip /
                  fmax(limit, dd2_group_micro_slip / dd2_group_max_friction_softness)
            : 0;
    candidate.mass += softness;
    const dd2_vehicle_vector velocity = dd2_group_velocity(workspace, index);
    const dd2_vehicle_vector slip = dd2_collision_add(
        velocity,
        dd2_collision_scale(contact.normal, -dd2_collision_dot(velocity, contact.normal)));
    const dd2_vehicle_vector gradient =
        dd2_collision_add(slip, dd2_collision_scale(constraint->friction, softness));
    candidate.impulse =
        dd2_collision_add(constraint->friction, dd2_collision_scale(gradient, -1 / candidate.mass));
    /* Project even without current slip: a released normal constraint cannot
     * retain friction from an earlier iteration with a larger normal impulse. */
    const double magnitude = sqrt(dd2_collision_dot(candidate.impulse, candidate.impulse));
    if (magnitude > limit) {
        candidate.impulse = dd2_collision_scale(candidate.impulse, limit / magnitude);
    }
    return candidate;
}

static double dd2_group_velocity_error(const dd2_group_workspace *workspace) {
    double error = 0;
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        const dd2_group_contact contact = workspace->query->contacts[index];
        const dd2_group_constraint *constraint = &workspace->constraints[index];
        const double speed =
            dd2_collision_dot(dd2_group_velocity(workspace, index), contact.normal);
        const double normal =
            constraint->normal_impulse > dd2_group_active_tolerance ? fabs(speed) : fmax(0, -speed);
        const dd2_group_friction candidate = dd2_group_friction_candidate(workspace, index);
        const dd2_vehicle_vector change =
            dd2_collision_add(candidate.impulse, dd2_collision_scale(constraint->friction, -1));
        error = fmax(error, fmax(normal, sqrt(dd2_collision_dot(change, change)) * candidate.mass));
    }
    return error;
}

typedef struct {
    double normal[DD2_VEHICLE_CONTACT_LIMIT];
    dd2_vehicle_vector friction[DD2_VEHICLE_CONTACT_LIMIT];
} dd2_group_iterate;

typedef struct {
    dd2_group_iterate image;
    dd2_group_iterate step;
    bool ready;
} dd2_group_history;

typedef struct {
    dd2_vehicle_vector velocity;
    dd2_vehicle_vector spin;
} dd2_group_motion;

typedef enum {
    DD2_GROUP_EXTRAPOLATION_SKIPPED,
    DD2_GROUP_EXTRAPOLATION_REJECTED,
    DD2_GROUP_EXTRAPOLATION_ACCEPTED
} dd2_group_extrapolation;

static dd2_group_iterate dd2_group_values(const dd2_group_workspace *workspace) {
    dd2_group_iterate values = {0};
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        values.normal[index] = workspace->constraints[index].normal_impulse;
        values.friction[index] = workspace->constraints[index].friction;
    }
    return values;
}

static dd2_group_iterate dd2_group_subtract(const dd2_group_iterate *left,
                                            const dd2_group_iterate *right, unsigned count) {
    dd2_group_iterate difference = {0};
    for (unsigned index = 0; index < count; ++index) {
        difference.normal[index] = left->normal[index] - right->normal[index];
        difference.friction[index] = dd2_collision_add(
            left->friction[index], dd2_collision_scale(right->friction[index], -1));
    }
    return difference;
}

static double dd2_group_iterate_dot(const dd2_group_iterate *left, const dd2_group_iterate *right,
                                    unsigned count) {
    double product = 0;
    for (unsigned index = 0; index < count; ++index) {
        product += (left->normal[index] * right->normal[index]) +
                   dd2_collision_dot(left->friction[index], right->friction[index]);
    }
    return product;
}

static bool dd2_group_predict(const dd2_group_workspace *workspace, const dd2_group_iterate *image,
                              const dd2_group_iterate *previous, double factor,
                              dd2_group_iterate *candidate) {
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        const double predicted =
            image->normal[index] - (factor * (image->normal[index] - previous->normal[index]));
        dd2_vehicle_vector tangent = dd2_collision_add(
            image->friction[index],
            dd2_collision_scale(
                dd2_collision_add(image->friction[index],
                                  dd2_collision_scale(previous->friction[index], -1)),
                -factor));
        const double squared = dd2_collision_dot(tangent, tangent);
        if (!dd2_numeric_finite(&predicted) || !dd2_group_vector_finite(&tangent) ||
            !dd2_numeric_finite(&squared)) {
            return false;
        }
        const double normal = fmax(0, predicted);
        const double limit = workspace->query->contacts[index].friction * normal;
        const double magnitude = sqrt(squared);
        if (magnitude > limit) {
            tangent = dd2_collision_scale(tangent, limit / magnitude);
        }
        candidate->normal[index] = normal;
        candidate->friction[index] = tangent;
    }
    return true;
}

static bool dd2_group_motion_finite(const dd2_group_workspace *workspace) {
    for (unsigned body = 0; body < workspace->query->body_count; ++body) {
        const dd2_vehicle *vehicle = &workspace->query->bodies[body];
        if (!dd2_group_vector_finite(&vehicle->velocity) ||
            !dd2_group_vector_finite(&vehicle->angular_velocity)) {
            return false;
        }
    }
    return true;
}

static void dd2_group_restore(dd2_group_workspace *workspace, const dd2_group_iterate *image,
                              const dd2_group_motion motion[DD2_VEHICLE_FLEET_LIMIT]) {
    for (unsigned body = 0; body < workspace->query->body_count; ++body) {
        workspace->query->bodies[body].velocity = motion[body].velocity;
        workspace->query->bodies[body].angular_velocity = motion[body].spin;
    }
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        workspace->constraints[index].normal_impulse = image->normal[index];
        workspace->constraints[index].friction = image->friction[index];
    }
}

typedef struct {
    const dd2_group_history *history;
    const dd2_group_iterate *image;
    const dd2_group_iterate *step;
} dd2_group_prediction;

static dd2_group_extrapolation dd2_group_extrapolate(dd2_group_workspace *workspace,
                                                     const dd2_group_prediction *prediction,
                                                     double *error) {
    const dd2_group_history *history = prediction->history;
    const dd2_group_iterate *image = prediction->image;
    const dd2_group_iterate *step = prediction->step;
    const unsigned count = workspace->query->contact_count;
    const dd2_group_iterate difference = dd2_group_subtract(step, &history->step, count);
    const double denominator = dd2_group_iterate_dot(&difference, &difference, count);
    if (!dd2_numeric_finite(&denominator) || denominator <= dd2_group_secant_floor) {
        return DD2_GROUP_EXTRAPOLATION_SKIPPED;
    }
    const double factor = dd2_group_iterate_dot(&difference, step, count) / denominator;
    dd2_group_iterate candidate = {0};
    if (!dd2_numeric_finite(&factor) ||
        !dd2_group_predict(workspace, image, &history->image, factor, &candidate)) {
        return DD2_GROUP_EXTRAPOLATION_REJECTED;
    }
    dd2_vehicle_vector impulses[DD2_VEHICLE_CONTACT_LIMIT] = {0};
    for (unsigned index = 0; index < count; ++index) {
        impulses[index] =
            dd2_collision_add(dd2_collision_scale(workspace->query->contacts[index].normal,
                                                  candidate.normal[index] - image->normal[index]),
                              dd2_collision_add(candidate.friction[index],
                                                dd2_collision_scale(image->friction[index], -1)));
        if (!dd2_group_vector_finite(&impulses[index])) {
            return DD2_GROUP_EXTRAPOLATION_REJECTED;
        }
    }
    dd2_group_motion motion[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned body = 0; body < workspace->query->body_count; ++body) {
        const dd2_vehicle *vehicle = &workspace->query->bodies[body];
        motion[body] = (dd2_group_motion){vehicle->velocity, vehicle->angular_velocity};
    }
    for (unsigned index = 0; index < count; ++index) {
        dd2_group_apply(workspace, index, impulses[index]);
        workspace->constraints[index].normal_impulse = candidate.normal[index];
        workspace->constraints[index].friction = candidate.friction[index];
    }
    if (dd2_group_motion_finite(workspace)) {
        const double predicted_error = dd2_group_velocity_error(workspace);
        if (dd2_numeric_finite(&predicted_error) && predicted_error < *error) {
            *error = predicted_error;
            return DD2_GROUP_EXTRAPOLATION_ACCEPTED;
        }
    }
    /* Restore exact saved motion; reversing rejected impulses would accumulate
     * rounding drift through the same poorly conditioned contact chain. */
    dd2_group_restore(workspace, image, motion);
    return DD2_GROUP_EXTRAPOLATION_REJECTED;
}

static bool dd2_group_velocities(dd2_group_workspace *workspace, dd2_group_solution *result) {
    dd2_group_history history = {0};
    dd2_group_motion initial[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned body = 0; body < workspace->query->body_count; ++body) {
        const dd2_vehicle *vehicle = &workspace->query->bodies[body];
        initial[body] = (dd2_group_motion){vehicle->velocity, vehicle->angular_velocity};
    }
    double progress_error = DBL_MAX;
    unsigned stalled = 0;
    bool accelerated = true;
    for (unsigned pass = 0; pass < DD2_GROUP_PASSES; ++pass) {
        const dd2_group_iterate before = dd2_group_values(workspace);
        for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
            const dd2_group_contact contact = workspace->query->contacts[index];
            dd2_group_constraint *constraint = &workspace->constraints[index];
            const double speed =
                dd2_collision_dot(dd2_group_velocity(workspace, index), contact.normal);
            /* Exact unilateral coordinate response avoids amplifying coupled
             * friction modes; the guarded secant handles slow convergence. */
            const double next =
                fmax(0, constraint->normal_impulse - (speed / constraint->normal_mass));
            dd2_group_apply(workspace, index,
                            dd2_collision_scale(contact.normal, next - constraint->normal_impulse));
            constraint->normal_impulse = next;
            const dd2_group_friction candidate = dd2_group_friction_candidate(workspace, index);
            dd2_group_apply(workspace, index,
                            dd2_collision_add(candidate.impulse,
                                              dd2_collision_scale(constraint->friction, -1)));
            constraint->friction = candidate.impulse;
        }
        result->velocity_passes = pass + 1;
        result->velocity_error = dd2_group_velocity_error(workspace);
        if (result->velocity_error < dd2_group_velocity_tolerance) {
            return true;
        }
        if (!accelerated) {
            continue;
        }
        const dd2_group_iterate image = dd2_group_values(workspace);
        const dd2_group_iterate step =
            dd2_group_subtract(&image, &before, workspace->query->contact_count);
        if (history.ready) {
            /* A secant through consecutive impulse fixed-point steps predicts
             * slow modes. Project its normal/friction cones, then accept only
             * a finite candidate with strictly smaller physical residual. */
            const dd2_group_prediction prediction = {&history, &image, &step};
            const dd2_group_extrapolation outcome =
                dd2_group_extrapolate(workspace, &prediction, &result->velocity_error);
            result->accelerated_passes += (unsigned)(outcome == DD2_GROUP_EXTRAPOLATION_ACCEPTED);
            result->rejected_extrapolations +=
                (unsigned)(outcome == DD2_GROUP_EXTRAPOLATION_REJECTED);
            if (result->velocity_error < dd2_group_velocity_tolerance) {
                return true;
            }
        }
        if (result->velocity_error < progress_error * dd2_group_progress_fraction) {
            progress_error = result->velocity_error;
            stalled = 0;
        } else {
            ++stalled;
        }
        if (stalled >= DD2_GROUP_STALLED_PASSES || pass + 1 == DD2_GROUP_ACCELERATION_PASSES) {
            /* Locally improving secants can keep revisiting a worse cycle.
             * Restart once from the exact input motion, with zero accumulated
             * impulses and ordinary coordinate steps. Retain every contact and
             * reserve half the existing total budget for unaccelerated response. */
            const dd2_group_iterate empty = {0};
            dd2_group_restore(workspace, &empty, initial);
            accelerated = false;
            ++result->coordinate_restarts;
            history.ready = false;
            continue;
        }
        history = (dd2_group_history){.image = image, .step = step, .ready = true};
    }
    return false;
}

static dd2_vehicle_vector
dd2_group_relative_offset(dd2_group_contact contact,
                          const dd2_vehicle_vector offsets[DD2_VEHICLE_FLEET_LIMIT]) {
    dd2_vehicle_vector relative = offsets[contact.first];
    if (dd2_group_pair(contact)) {
        relative = dd2_collision_add(relative, dd2_collision_scale(offsets[contact.second], -1));
    }
    return relative;
}

static double dd2_group_required_offset(dd2_group_contact contact) {
    return contact.penetration + (dd2_group_clearance * (dd2_group_pair(contact) ? 2 : 1));
}

static double dd2_group_position_error(const dd2_group_workspace *workspace,
                                       const dd2_vehicle_vector offsets[DD2_VEHICLE_FLEET_LIMIT]) {
    double error = 0;
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        const dd2_group_contact contact = workspace->query->contacts[index];
        const double gap =
            dd2_collision_dot(dd2_group_relative_offset(contact, offsets), contact.normal);
        const double remaining = dd2_group_required_offset(contact) - gap;
        const bool active =
            workspace->constraints[index].position_impulse > dd2_group_active_tolerance;
        error = fmax(error, active ? fabs(remaining) : fmax(0, remaining));
    }
    return error;
}

static bool dd2_group_positions(dd2_group_workspace *workspace, dd2_group_solution *result) {
    dd2_vehicle_vector offsets[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned pass = 0; pass < DD2_GROUP_PASSES; ++pass) {
        for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
            const dd2_group_contact contact = workspace->query->contacts[index];
            dd2_group_constraint *constraint = &workspace->constraints[index];
            const double gap =
                dd2_collision_dot(dd2_group_relative_offset(contact, offsets), contact.normal);
            const double next =
                fmax(0, constraint->position_impulse + (dd2_group_position_relaxation *
                                                        (dd2_group_required_offset(contact) - gap) /
                                                        (dd2_group_pair(contact) ? 2 : 1)));
            const dd2_vehicle_vector delta =
                dd2_collision_scale(contact.normal, next - constraint->position_impulse);
            offsets[contact.first] = dd2_collision_add(offsets[contact.first], delta);
            if (dd2_group_pair(contact)) {
                offsets[contact.second] =
                    dd2_collision_add(offsets[contact.second], dd2_collision_scale(delta, -1));
            }
            constraint->position_impulse = next;
        }
        result->position_passes = pass + 1;
        result->position_error = dd2_group_position_error(workspace, offsets);
        if (result->position_error < dd2_group_position_tolerance) {
            for (unsigned body = 0; body < workspace->query->body_count; ++body) {
                dd2_vehicle *vehicle = &workspace->query->bodies[body];
                vehicle->position = dd2_collision_add(vehicle->position, offsets[body]);
            }
            return true;
        }
    }
    return false;
}

bool dd2_contact_group_solve(const dd2_group_query *query, dd2_group_solution *solution) {
    if (solution == NULL) {
        return false;
    }
    *solution = (dd2_group_solution){0};
    if (!dd2_group_validate(query)) {
        return false;
    }
    dd2_group_workspace workspace = {.query = query};
    dd2_group_solution result = {0};
    if (!dd2_group_prepare(&workspace) || !dd2_group_velocities(&workspace, &result) ||
        !dd2_group_positions(&workspace, &result)) {
        return false;
    }
    for (unsigned body = 0; body < query->body_count; ++body) {
        if (!dd2_vehicle_valid(&query->bodies[body])) {
            return false;
        }
    }
    result.count = query->contact_count;
    for (unsigned index = 0; index < query->contact_count; ++index) {
        const dd2_group_constraint *constraint = &workspace.constraints[index];
        result.contacts[index] = (dd2_group_response){
            .friction_impulse = constraint->friction,
            .normal_impulse = constraint->normal_impulse,
            .normal_mass = constraint->normal_mass,
            .initial_normal_speed = constraint->initial_normal_speed,
            .final_normal_speed = dd2_collision_dot(dd2_group_velocity(&workspace, index),
                                                    query->contacts[index].normal)};
    }
    *solution = result;
    return true;
}
