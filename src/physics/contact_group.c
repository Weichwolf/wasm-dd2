#include "physics/contact_group.h"

#include "physics/collision_math.h"
#include "physics/numeric.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

enum {
    DD2_GROUP_PASSES = 4096,
    DD2_GROUP_STALLED_PASSES = 64,
    DD2_GROUP_NEWTON_AXES = 3,
    DD2_GROUP_NEWTON_DIMENSIONS = DD2_GROUP_NEWTON_AXES * DD2_VEHICLE_CONTACT_LIMIT,
    DD2_GROUP_NEWTON_PERIOD = 32,
    DD2_GROUP_NEWTON_DELAY = 512,
    DD2_GROUP_NEWTON_SEARCHES = 16,
    DD2_GROUP_NEWTON_REFINEMENTS = 16,
    DD2_GROUP_NEWTON_BASE_MODELS = 2,
    DD2_GROUP_NEWTON_CONTACT_MODELS = 4,
    DD2_GROUP_NEWTON_MODEL_LIMIT =
        DD2_GROUP_NEWTON_BASE_MODELS + (DD2_GROUP_NEWTON_CONTACT_MODELS * DD2_VEHICLE_CONTACT_LIMIT)
};
_Static_assert(DD2_VEHICLE_CONTACT_LIMIT <= sizeof(uint64_t) * CHAR_BIT,
               "Contact branch selection must fit its mask");
/* Late stick/slip roots need a smaller forward difference than coarse sweep
 * errors. Keep enough separation from cancellation in accumulated body motion;
 * the captured pressure-sensitive supports verify this scale independently. */
static const double dd2_group_newton_difference = 1e-6;
static const double dd2_group_newton_pivot_floor = 1e-12;
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

static double dd2_group_friction_softness(double limit) {
    return limit == 0 ? 0
                      : dd2_group_micro_slip /
                            fmax(limit, dd2_group_micro_slip / dd2_group_max_friction_softness);
}

static dd2_group_friction dd2_group_friction_candidate(const dd2_group_workspace *workspace,
                                                       unsigned index) {
    const dd2_group_contact contact = workspace->query->contacts[index];
    const dd2_group_constraint *constraint = &workspace->constraints[index];
    const double limit = contact.friction * constraint->normal_impulse;
    dd2_group_friction candidate = {.mass = constraint->tangent_mass};
    if (limit == 0) {
        return candidate;
    }
    /* Implicit regularized body/world friction: below 0.1 world units/s, the
     * opposing impulse grows linearly with slip; above it, Coulomb saturation
     * is unchanged. The tiny creep avoids ambiguous stick/slip branches in
     * both coupled bodies and nearly parallel road/wall supports.
     * The softness cap keeps vanishing pressure finite; its maximum affected
     * cone radius is only 1e-13 impulse units. */
    const double softness = dd2_group_friction_softness(limit);
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

typedef struct {
    dd2_vehicle_vector axes[DD2_GROUP_NEWTON_AXES];
} dd2_group_basis;

typedef enum {
    DD2_GROUP_NEWTON_PROJECTED,
    DD2_GROUP_NEWTON_WORLD_LINEAR,
    DD2_GROUP_NEWTON_CONSTITUTIVE,
    DD2_GROUP_NEWTON_SATURATED,
    DD2_GROUP_NEWTON_LOAD,
    DD2_GROUP_NEWTON_RELEASE,
    DD2_GROUP_NEWTON_PATCH
} dd2_group_newton_method;

typedef struct {
    uint64_t released_contacts; /* Fixed zero-impulse rows of a private patch branch. */
    dd2_group_newton_method method;
    unsigned selected_contact; /* Contact limit selects the complete saturated branch. */
    unsigned second_contact;   /* Second retained endpoint of a private world patch. */
} dd2_group_newton_model;

typedef struct {
    dd2_group_iterate base;
    dd2_group_motion motion[DD2_VEHICLE_FLEET_LIMIT];
    dd2_group_basis basis[DD2_VEHICLE_CONTACT_LIMIT];
    double residual[DD2_GROUP_NEWTON_DIMENSIONS];
    size_t dimensions;
    dd2_group_newton_model model;
} dd2_group_newton_state;

typedef struct {
    double entries[DD2_GROUP_NEWTON_DIMENSIONS][DD2_GROUP_NEWTON_DIMENSIONS + 1];
    double direction[DD2_GROUP_NEWTON_DIMENSIONS];
    size_t dimensions;
} dd2_group_linear_system;

static dd2_group_basis dd2_group_contact_basis(dd2_vehicle_vector normal) {
    const dd2_vehicle_vector reference =
        fabs(normal.x) < 0.5 ? (dd2_vehicle_vector){.x = 1} : (dd2_vehicle_vector){.y = 1};
    dd2_vehicle_vector tangent = dd2_collision_cross(normal, reference);
    tangent = dd2_collision_scale(tangent, 1 / sqrt(dd2_collision_dot(tangent, tangent)));
    return (dd2_group_basis){.axes = {normal, tangent, dd2_collision_cross(normal, tangent)}};
}

/* A constitutive root avoids differentiating the projection radius through
 * the pressure-dependent 0.1-unit friction softness. Unit-impulse mobility
 * also avoids cancellation against the already accumulated body velocity. */
typedef struct {
    dd2_vehicle_vector direction;
    double velocity_scale;
    double pressure_scale;
    bool sliding;
} dd2_group_material;

static dd2_group_material dd2_group_material_at(const dd2_group_workspace *workspace,
                                                unsigned index, bool saturated) {
    const dd2_group_contact contact = workspace->query->contacts[index];
    const double limit = contact.friction * workspace->constraints[index].normal_impulse;
    const dd2_vehicle_vector velocity = dd2_group_velocity(workspace, index);
    const dd2_vehicle_vector slip = dd2_collision_add(
        velocity,
        dd2_collision_scale(contact.normal, -dd2_collision_dot(velocity, contact.normal)));
    const double speed = sqrt(dd2_collision_dot(slip, slip));
    const double transition = fmin(dd2_group_micro_slip, dd2_group_max_friction_softness * limit);
    const double denominator = saturated && speed > 0 ? speed : fmax(transition, speed);
    if (denominator == 0) {
        return (dd2_group_material){0};
    }
    return (dd2_group_material){.direction = dd2_collision_scale(slip, 1 / denominator),
                                .velocity_scale = limit / denominator,
                                .pressure_scale = saturated || speed > transition ||
                                                          transition == dd2_group_micro_slip
                                                      ? contact.friction
                                                      : 0,
                                .sliding = (saturated && speed > 0) || speed > transition};
}

/* Saturation is an alternate private direction, not a material change. A
 * positive load below the transition can trap the regularized root on its
 * linear branch; final acceptance still uses the original projected law. */
static bool dd2_group_regularized_load(const dd2_group_workspace *workspace) {
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        if (workspace->constraints[index].normal_impulse > 0 &&
            !dd2_group_material_at(workspace, index, false).sliding) {
            return true;
        }
    }
    return false;
}

static bool dd2_group_newton_saturated(const dd2_group_newton_state *state, unsigned index) {
    return state->model.method == DD2_GROUP_NEWTON_SATURATED &&
           (state->model.selected_contact == DD2_VEHICLE_CONTACT_LIMIT ||
            state->model.selected_contact == index);
}

static bool dd2_group_newton_pressure(dd2_group_newton_method method) {
    return method == DD2_GROUP_NEWTON_LOAD || method == DD2_GROUP_NEWTON_RELEASE;
}

static bool dd2_group_newton_material(dd2_group_newton_method method) {
    return method == DD2_GROUP_NEWTON_CONSTITUTIVE || method == DD2_GROUP_NEWTON_SATURATED ||
           dd2_group_newton_pressure(method) || method == DD2_GROUP_NEWTON_PATCH;
}

static bool dd2_group_newton_retained(const dd2_group_newton_state *state, unsigned index) {
    return state->model.method == DD2_GROUP_NEWTON_PATCH &&
           (state->model.selected_contact == index || state->model.second_contact == index);
}

static bool dd2_group_newton_released(dd2_group_newton_model model, unsigned index) {
    return (model.released_contacts & (UINT64_C(1) << index)) != 0;
}

static void dd2_group_residuals(const dd2_group_workspace *workspace,
                                const dd2_group_newton_state *state, double *residual) {
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        const dd2_group_constraint *constraint = &workspace->constraints[index];
        const dd2_group_friction friction = dd2_group_friction_candidate(workspace, index);
        const double speed = dd2_collision_dot(dd2_group_velocity(workspace, index),
                                               workspace->query->contacts[index].normal);
        const size_t offset = DD2_GROUP_NEWTON_AXES * (size_t)index;
        residual[offset] =
            constraint->normal_mass *
            (constraint->normal_impulse -
             fmax(0, constraint->normal_impulse - (speed / constraint->normal_mass)));
        if (dd2_group_newton_retained(state, index)) {
            residual[offset] = speed;
        }
        const dd2_vehicle_vector difference =
            dd2_collision_add(constraint->friction, dd2_collision_scale(friction.impulse, -1));
        const dd2_group_contact contact = workspace->query->contacts[index];
        if (state->model.method == DD2_GROUP_NEWTON_WORLD_LINEAR && !dd2_group_pair(contact) &&
            contact.friction > 0 && state->base.normal[index] > 0) {
            const dd2_vehicle_vector gradient = dd2_collision_add(
                dd2_group_velocity(workspace, index),
                dd2_collision_scale(
                    constraint->friction,
                    dd2_group_friction_softness(contact.friction * constraint->normal_impulse)));
            residual[offset + 1] = dd2_collision_dot(gradient, state->basis[index].axes[1]);
            residual[offset + 2] = dd2_collision_dot(gradient, state->basis[index].axes[2]);
        } else if (dd2_group_newton_material(state->model.method)) {
            const dd2_group_material material =
                dd2_group_material_at(workspace, index, dd2_group_newton_saturated(state, index));
            const dd2_vehicle_vector root = dd2_collision_add(
                constraint->friction,
                dd2_collision_scale(material.direction,
                                    contact.friction * constraint->normal_impulse));
            residual[offset + 1] =
                constraint->tangent_mass * dd2_collision_dot(root, state->basis[index].axes[1]);
            residual[offset + 2] =
                constraint->tangent_mass * dd2_collision_dot(root, state->basis[index].axes[2]);
        } else {
            residual[offset + 1] =
                friction.mass * dd2_collision_dot(difference, state->basis[index].axes[1]);
            residual[offset + 2] =
                friction.mass * dd2_collision_dot(difference, state->basis[index].axes[2]);
        }
    }
}

static dd2_group_newton_state dd2_group_newton_prepare(const dd2_group_workspace *workspace,
                                                       dd2_group_newton_model model) {
    dd2_group_newton_state state = {.base = dd2_group_values(workspace),
                                    .model = model,
                                    .dimensions = DD2_GROUP_NEWTON_AXES *
                                                  (size_t)workspace->query->contact_count};
    for (unsigned body = 0; body < workspace->query->body_count; ++body) {
        state.motion[body] = (dd2_group_motion){workspace->query->bodies[body].velocity,
                                                workspace->query->bodies[body].angular_velocity};
    }
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        state.basis[index] = dd2_group_contact_basis(workspace->query->contacts[index].normal);
    }
    dd2_group_residuals(workspace, &state, state.residual);
    return state;
}

static void dd2_group_newton_apply(dd2_group_workspace *workspace,
                                   const dd2_group_newton_state *state,
                                   const dd2_group_iterate *candidate) {
    dd2_group_restore(workspace, &state->base, state->motion);
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        const dd2_vehicle_vector impulse = dd2_collision_add(
            dd2_collision_scale(workspace->query->contacts[index].normal,
                                candidate->normal[index] - state->base.normal[index]),
            dd2_collision_add(candidate->friction[index],
                              dd2_collision_scale(state->base.friction[index], -1)));
        dd2_group_apply(workspace, index, impulse);
        workspace->constraints[index].normal_impulse = candidate->normal[index];
        workspace->constraints[index].friction = candidate->friction[index];
    }
}

static double dd2_group_newton_perturb(dd2_group_iterate *candidate,
                                       const dd2_group_newton_state *state, size_t column) {
    const size_t index = column / DD2_GROUP_NEWTON_AXES;
    const size_t axis = column % DD2_GROUP_NEWTON_AXES;
    const double value =
        axis == 0 ? state->base.normal[index]
                  : dd2_collision_dot(state->base.friction[index], state->basis[index].axes[axis]);
    const double change = dd2_group_newton_difference * fmax(1, fabs(value));
    *candidate = state->base;
    if (axis == 0) {
        candidate->normal[index] += change;
    } else {
        candidate->friction[index] =
            dd2_collision_add(candidate->friction[index],
                              dd2_collision_scale(state->basis[index].axes[axis], change));
    }
    return change;
}

typedef struct {
    unsigned slots[2];
    dd2_group_motion motion[2];
} dd2_group_impulse_response;

static dd2_group_impulse_response dd2_group_unit_response(const dd2_group_workspace *workspace,
                                                          unsigned index, dd2_vehicle_vector axis) {
    const dd2_group_contact contact = workspace->query->contacts[index];
    dd2_group_impulse_response result = {.slots = {contact.first, contact.second}};
    for (unsigned side = 0; side < 2; ++side) {
        const unsigned slot = result.slots[side];
        if (slot == DD2_VEHICLE_NO_PARTNER) {
            continue;
        }
        dd2_vehicle response = {.rotation = workspace->query->bodies[slot].rotation};
        dd2_collision_impulse(&response, workspace->constraints[index].arms[side],
                              dd2_collision_scale(axis, side == 0 ? 1 : -1));
        result.motion[side] = (dd2_group_motion){response.velocity, response.angular_velocity};
    }
    return result;
}

static dd2_vehicle_vector dd2_group_mobility(const dd2_group_workspace *workspace, unsigned index,
                                             const dd2_group_impulse_response *response) {
    const dd2_group_contact contact = workspace->query->contacts[index];
    const unsigned slots[] = {contact.first, contact.second};
    dd2_vehicle_vector result = {0};
    for (unsigned source = 0; source < 2; ++source) {
        if (response->slots[source] == DD2_VEHICLE_NO_PARTNER) {
            continue;
        }
        for (unsigned target = 0; target < 2; ++target) {
            if (slots[target] == response->slots[source]) {
                const dd2_vehicle_vector velocity = dd2_collision_add(
                    response->motion[source].velocity,
                    dd2_collision_cross(response->motion[source].spin,
                                        workspace->constraints[index].arms[target]));
                result =
                    dd2_collision_add(result, dd2_collision_scale(velocity, target == 0 ? 1 : -1));
            }
        }
    }
    return result;
}

static void dd2_group_material_row(const dd2_group_workspace *workspace,
                                   const dd2_group_newton_state *state,
                                   dd2_group_linear_system *system, unsigned index,
                                   const dd2_group_impulse_response *responses) {
    const dd2_group_contact contact = workspace->query->contacts[index];
    const dd2_group_constraint *constraint = &workspace->constraints[index];
    const dd2_group_material material =
        dd2_group_material_at(workspace, index, dd2_group_newton_saturated(state, index));
    const double speed = dd2_collision_dot(dd2_group_velocity(workspace, index), contact.normal);
    const bool loaded = dd2_group_newton_retained(state, index) ||
                        constraint->normal_impulse - speed / constraint->normal_mass > 0;
    const size_t offset = DD2_GROUP_NEWTON_AXES * (size_t)index;
    for (size_t column = 0; column < state->dimensions; ++column) {
        const unsigned source = (unsigned)(column / DD2_GROUP_NEWTON_AXES);
        const size_t axis = column % DD2_GROUP_NEWTON_AXES;
        const dd2_vehicle_vector velocity =
            dd2_group_mobility(workspace, index, &responses[column]);
        const double normal = dd2_collision_dot(velocity, contact.normal);
        dd2_vehicle_vector tangent =
            dd2_collision_add(velocity, dd2_collision_scale(contact.normal, -normal));
        if (material.sliding) {
            tangent = dd2_collision_add(
                tangent, dd2_collision_scale(material.direction,
                                             -dd2_collision_dot(material.direction, tangent)));
        }
        const double pressure = source == index && axis == 0 ? 1 : 0;
        const dd2_vehicle_vector friction =
            source == index && axis > 0 ? state->basis[source].axes[axis] : (dd2_vehicle_vector){0};
        const dd2_vehicle_vector derivative = dd2_collision_add(
            friction, dd2_collision_add(dd2_collision_scale(material.direction,
                                                            material.pressure_scale * pressure),
                                        dd2_collision_scale(tangent, material.velocity_scale)));
        system->entries[offset][column] = loaded ? normal : constraint->normal_mass * pressure;
        system->entries[offset + 1][column] =
            constraint->tangent_mass * dd2_collision_dot(derivative, state->basis[index].axes[1]);
        system->entries[offset + 2][column] =
            constraint->tangent_mass * dd2_collision_dot(derivative, state->basis[index].axes[2]);
    }
    for (size_t axis = 0; axis < DD2_GROUP_NEWTON_AXES; ++axis) {
        system->entries[offset + axis][state->dimensions] = -state->residual[offset + axis];
    }
}

static void dd2_group_newton_matrix(dd2_group_workspace *workspace,
                                    const dd2_group_newton_state *state,
                                    dd2_group_linear_system *system) {
    if (dd2_group_newton_material(state->model.method)) {
        /* Each impulse column reaches at most two bodies. Cache its unit
         * motion once instead of recomputing inertia for every receiver row. */
        dd2_group_impulse_response responses[DD2_GROUP_NEWTON_DIMENSIONS] = {0};
        for (size_t column = 0; column < state->dimensions; ++column) {
            const unsigned source = (unsigned)(column / DD2_GROUP_NEWTON_AXES);
            const size_t axis = column % DD2_GROUP_NEWTON_AXES;
            responses[column] =
                dd2_group_unit_response(workspace, source, state->basis[source].axes[axis]);
        }
        for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
            dd2_group_material_row(workspace, state, system, index, responses);
        }
        return;
    }
    for (size_t column = 0; column < state->dimensions; ++column) {
        dd2_group_iterate candidate = {0};
        const double change = dd2_group_newton_perturb(&candidate, state, column);
        dd2_group_newton_apply(workspace, state, &candidate);
        double residual[DD2_GROUP_NEWTON_DIMENSIONS] = {0};
        dd2_group_residuals(workspace, state, residual);
        for (size_t row = 0; row < state->dimensions; ++row) {
            system->entries[row][column] = (residual[row] - state->residual[row]) / change;
        }
    }
    dd2_group_restore(workspace, &state->base, state->motion);
    for (size_t row = 0; row < state->dimensions; ++row) {
        system->entries[row][state->dimensions] = -state->residual[row];
    }
}

static size_t dd2_group_linear_pivot(const dd2_group_linear_system *system, size_t column) {
    size_t pivot = column;
    for (size_t row = column + 1; row < system->dimensions; ++row) {
        if (fabs(system->entries[row][column]) > fabs(system->entries[pivot][column])) {
            pivot = row;
        }
    }
    return pivot;
}

static bool dd2_group_linear_eliminate(dd2_group_linear_system *system, size_t column) {
    const size_t pivot = dd2_group_linear_pivot(system, column);
    if (fabs(system->entries[pivot][column]) < dd2_group_newton_pivot_floor) {
        return false;
    }
    if (pivot != column) {
        for (size_t entry = column; entry <= system->dimensions; ++entry) {
            const double value = system->entries[column][entry];
            system->entries[column][entry] = system->entries[pivot][entry];
            system->entries[pivot][entry] = value;
        }
    }
    for (size_t row = column + 1; row < system->dimensions; ++row) {
        const double factor = system->entries[row][column] / system->entries[column][column];
        for (size_t entry = column; entry <= system->dimensions; ++entry) {
            system->entries[row][entry] -= factor * system->entries[column][entry];
        }
    }
    return true;
}

static bool dd2_group_linear_solve(dd2_group_linear_system *system, size_t dimensions) {
    for (size_t column = 0; column < dimensions; ++column) {
        if (!dd2_group_linear_eliminate(system, column)) {
            return false;
        }
    }
    for (size_t remaining = dimensions; remaining > 0; --remaining) {
        const size_t row = remaining - 1;
        double value = system->entries[row][dimensions];
        for (size_t column = row + 1; column < dimensions; ++column) {
            value -= system->entries[row][column] * system->direction[column];
        }
        system->direction[row] = value / system->entries[row][row];
        if (!dd2_numeric_finite(&system->direction[row])) {
            return false;
        }
    }
    return true;
}

/* An unconstrained Newton direction can require negative normal pressure.
 * Refit its released contact with zero normal/tangent impulse instead of
 * clipping that direction while leaving every coupled equation unchanged. */
static void dd2_group_linear_release(dd2_group_linear_system *system,
                                     const dd2_group_newton_state *state, unsigned index) {
    const size_t offset = DD2_GROUP_NEWTON_AXES * (size_t)index;
    for (size_t axis = 0; axis < DD2_GROUP_NEWTON_AXES; ++axis) {
        const size_t row = offset + axis;
        for (size_t column = 0; column < state->dimensions; ++column) {
            system->entries[row][column] = column == row ? 1 : 0;
        }
        const double impulse = axis == 0 ? state->base.normal[index]
                                         : dd2_collision_dot(state->base.friction[index],
                                                             state->basis[index].axes[axis]);
        system->entries[row][state->dimensions] = -impulse;
    }
}

/* Releases are monotone within one direction search: each refit adds at
 * least one contact, so there are at most contact_count + 1 linear solves.
 * All original contacts remain in the physical residual and final checks. */
static bool dd2_group_newton_direction(dd2_group_workspace *workspace,
                                       const dd2_group_newton_state *state,
                                       dd2_group_linear_system *system) {
    bool released[DD2_VEHICLE_CONTACT_LIMIT] = {false};
    /* Keep the selected support unloaded throughout this private branch.
     * Its normal inequality still participates in physical acceptance. */
    if (state->model.method == DD2_GROUP_NEWTON_RELEASE) {
        released[state->model.selected_contact] = true;
    }
    const unsigned count = workspace->query->contact_count;
    for (unsigned index = 0; index < count; ++index) {
        released[index] = released[index] || dd2_group_newton_released(state->model, index);
    }
    for (unsigned attempt = 0; attempt <= count; ++attempt) {
        dd2_group_newton_matrix(workspace, state, system);
        for (unsigned index = 0; index < count; ++index) {
            if (released[index]) {
                dd2_group_linear_release(system, state, index);
            }
        }
        if (!dd2_group_linear_solve(system, state->dimensions)) {
            return false;
        }
        bool changed = false;
        for (unsigned index = 0; index < count; ++index) {
            if (!released[index] &&
                state->base.normal[index] +
                        system->direction[DD2_GROUP_NEWTON_AXES * (size_t)index] <
                    0) {
                released[index] = true;
                changed = true;
            }
        }
        if (!changed) {
            return true;
        }
    }
    return false;
}

typedef struct {
    const dd2_group_newton_state *state;
    const double *direction;
    double factor;
} dd2_group_newton_prediction;

static dd2_group_iterate dd2_group_newton_candidate(const dd2_group_workspace *workspace,
                                                    dd2_group_newton_prediction prediction) {
    dd2_group_iterate candidate = prediction.state->base;
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        const size_t offset = DD2_GROUP_NEWTON_AXES * (size_t)index;
        /* The fitted target is exactly zero. Avoid reconstructing tiny
         * positive loads whose pressure-dependent softness amplifies roundoff. */
        if (dd2_group_newton_released(prediction.state->model, index)) {
            candidate.normal[index] *= 1 - prediction.factor;
            candidate.friction[index] =
                dd2_collision_scale(candidate.friction[index], 1 - prediction.factor);
            continue;
        }
        candidate.normal[index] =
            fmax(0, candidate.normal[index] + (prediction.factor * prediction.direction[offset]));
        candidate.friction[index] = dd2_collision_add(
            candidate.friction[index],
            dd2_collision_add(
                dd2_collision_scale(prediction.state->basis[index].axes[1],
                                    prediction.factor * prediction.direction[offset + 1]),
                dd2_collision_scale(prediction.state->basis[index].axes[2],
                                    prediction.factor * prediction.direction[offset + 2])));
        const double magnitude =
            sqrt(dd2_collision_dot(candidate.friction[index], candidate.friction[index]));
        const double limit = workspace->query->contacts[index].friction * candidate.normal[index];
        if (magnitude > limit) {
            candidate.friction[index] =
                dd2_collision_scale(candidate.friction[index], limit / magnitude);
        }
    }
    return candidate;
}

/* Cold Newton corrections preserve the same physical contact equations. The bounded dense matrix
 * uses automatic storage, never allocation. Singular directions are skipped. Every
 * rejected/backtracked trial restores exact motion, and only a smaller physical residual is
 * accepted. */
static bool dd2_group_newton_step(dd2_group_workspace *workspace, double *error,
                                  dd2_group_newton_model model, dd2_group_linear_system *system) {
    const dd2_group_newton_state state = dd2_group_newton_prepare(workspace, model);
    system->dimensions = state.dimensions;
    if (!dd2_group_newton_direction(workspace, &state, system)) {
        return false;
    }
    dd2_group_newton_prediction prediction = {&state, system->direction, 1};
    for (unsigned attempt = 0; attempt < DD2_GROUP_NEWTON_SEARCHES; ++attempt) {
        const dd2_group_iterate candidate = dd2_group_newton_candidate(workspace, prediction);
        dd2_group_newton_apply(workspace, &state, &candidate);
        const double predicted_error = dd2_group_velocity_error(workspace);
        if (dd2_group_motion_finite(workspace) && dd2_numeric_finite(&predicted_error) &&
            predicted_error < *error) {
            *error = predicted_error;
            return true;
        }
        prediction.factor /= 2;
    }
    dd2_group_restore(workspace, &state.base, state.motion);
    return false;
}

static bool dd2_group_newton_trial(dd2_group_workspace *workspace, double *error,
                                   dd2_group_newton_method method) {
    dd2_group_linear_system system = {0};
    return dd2_group_newton_step(
        workspace, error,
        (dd2_group_newton_model){.method = method, .selected_contact = DD2_VEHICLE_CONTACT_LIMIT},
        &system);
}

static void dd2_group_copy_motion(const dd2_group_workspace *workspace, dd2_group_motion *motion) {
    for (unsigned body = 0; body < workspace->query->body_count; ++body) {
        const dd2_vehicle *vehicle = &workspace->query->bodies[body];
        motion[body] = (dd2_group_motion){vehicle->velocity, vehicle->angular_velocity};
    }
}

/* Compare the projected-root and analytic constitutive directions from the
 * same exact input motion. The latter includes pressure-dependent friction
 * and the saturated/linear transition for every coupled body/world contact.
 * Only the lower physical residual is retained; cone projection, active-set
 * refits, finite checks and exact rollback apply to both trials. World-only
 * groups retain their zero-friction-gradient alternate; the analytic material
 * direction handles pressure coupling with dynamic contact partners. */
static bool dd2_group_newton_refitted(dd2_group_workspace *workspace, double *error) {
    bool supported = false;
    bool paired = false;
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        const dd2_group_contact contact = workspace->query->contacts[index];
        paired = paired || dd2_group_pair(contact);
        supported = supported || (!dd2_group_pair(contact) && contact.friction > 0 &&
                                  workspace->constraints[index].normal_impulse > 0);
    }
    if (!supported) {
        return dd2_group_newton_trial(workspace, error, DD2_GROUP_NEWTON_PROJECTED);
    }
    const dd2_group_iterate base = dd2_group_values(workspace);
    dd2_group_motion motion[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_group_copy_motion(workspace, motion);
    double projected_error = *error;
    const bool projected =
        dd2_group_newton_trial(workspace, &projected_error, DD2_GROUP_NEWTON_PROJECTED);
    const dd2_group_iterate projected_values = dd2_group_values(workspace);
    dd2_group_motion projected_motion[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_group_copy_motion(workspace, projected_motion);
    dd2_group_restore(workspace, &base, motion);
    double alternate_error = *error;
    const dd2_group_newton_method alternate =
        paired ? DD2_GROUP_NEWTON_CONSTITUTIVE : DD2_GROUP_NEWTON_WORLD_LINEAR;
    const bool alternate_accepted = dd2_group_newton_trial(workspace, &alternate_error, alternate);
    if (alternate_accepted && (!projected || alternate_error < projected_error)) {
        *error = alternate_error;
        return true;
    }
    if (projected) {
        dd2_group_restore(workspace, &projected_values, projected_motion);
        *error = projected_error;
        return true;
    }
    return false;
}

static bool dd2_group_normal_patch(const dd2_group_workspace *workspace) {
    for (unsigned first = 0; first < workspace->query->contact_count; ++first) {
        const dd2_group_contact source = workspace->query->contacts[first];
        if (dd2_group_pair(source)) {
            continue;
        }
        for (unsigned second = first + 1; second < workspace->query->contact_count; ++second) {
            const dd2_group_contact target = workspace->query->contacts[second];
            const dd2_vehicle_vector difference =
                dd2_collision_add(source.normal, dd2_collision_scale(target.normal, -1));
            if (!dd2_group_pair(target) && source.first == target.first &&
                dd2_collision_dot(difference, difference) <
                    dd2_group_axis_tolerance * dd2_group_axis_tolerance) {
                return true;
            }
        }
    }
    return false;
}

/* Co-oriented world supports can leave dependent active normal rows. Use the
 * existing projected Jacobian only when the analytic seed is singular and
 * such a patch exists; all inequalities remain in every physical check. */
static bool dd2_group_refinement_seed(dd2_group_workspace *workspace, dd2_group_newton_state *state,
                                      dd2_group_linear_system *system) {
    if (dd2_group_newton_direction(workspace, state, system)) {
        return true;
    }
    if (!dd2_group_normal_patch(workspace)) {
        return false;
    }
    *state = dd2_group_newton_prepare(
        workspace, (dd2_group_newton_model){.method = DD2_GROUP_NEWTON_PROJECTED});
    return dd2_group_newton_direction(workspace, state, system);
}

/* Explore higher world pressure while retaining every constraint.
 * Equilibrate friction with all normal loads held fixed before the constitutive
 * refinement. Cone projection and the final physical law still apply. */
static bool dd2_group_pressure_seed(dd2_group_workspace *workspace, unsigned index,
                                    dd2_group_linear_system *system) {
    const double pressure = workspace->constraints[index].normal_impulse;
    workspace->constraints[index].normal_impulse += pressure;
    dd2_group_apply(workspace, index,
                    dd2_collision_scale(workspace->query->contacts[index].normal, pressure));
    if (!dd2_group_motion_finite(workspace)) {
        return false;
    }
    const dd2_group_newton_state state = dd2_group_newton_prepare(
        workspace, (dd2_group_newton_model){.method = DD2_GROUP_NEWTON_WORLD_LINEAR});
    dd2_group_newton_matrix(workspace, &state, system);
    for (unsigned contact = 0; contact < workspace->query->contact_count; ++contact) {
        const size_t row = DD2_GROUP_NEWTON_AXES * (size_t)contact;
        for (size_t column = 0; column < state.dimensions; ++column) {
            system->entries[row][column] = column == row ? 1 : 0;
        }
        system->entries[row][state.dimensions] = 0;
    }
    if (!dd2_group_linear_solve(system, state.dimensions)) {
        return false;
    }
    const dd2_group_iterate candidate = dd2_group_newton_candidate(
        workspace, (dd2_group_newton_prediction){&state, system->direction, 1});
    dd2_group_newton_apply(workspace, &state, &candidate);
    return dd2_group_motion_finite(workspace);
}

/* Refine a speculative branch after its full fitted step. Intermediate states
 * stay private: the outer iterate changes only for a finite smaller residual.
 * Re-evaluating loaded normals allows previously separating supports to load
 * while friction directions settle. The original regularized physical residual
 * still controls acceptance of the alternate saturated branch. Reuse one
 * bounded matrix for all steps. */
static bool dd2_group_newton_refine(dd2_group_workspace *workspace, double *error,
                                    dd2_group_newton_model model) {
    const dd2_group_newton_state original = dd2_group_newton_prepare(workspace, model);
    dd2_group_linear_system system = {.dimensions = original.dimensions};
    const bool seeded = model.method == DD2_GROUP_NEWTON_LOAD;
    if (seeded && !dd2_group_pressure_seed(workspace, model.selected_contact, &system)) {
        dd2_group_restore(workspace, &original.base, original.motion);
        return false;
    }
    dd2_group_newton_state prepared =
        seeded ? dd2_group_newton_prepare(workspace, model) : original;
    if (!dd2_group_refinement_seed(workspace, &prepared, &system)) {
        dd2_group_restore(workspace, &original.base, original.motion);
        return false;
    }
    const dd2_group_iterate seed = dd2_group_newton_candidate(
        workspace, (dd2_group_newton_prediction){&prepared, system.direction, 1});
    dd2_group_newton_apply(workspace, &prepared, &seed);
    double current = dd2_group_velocity_error(workspace);
    if (dd2_group_motion_finite(workspace) && dd2_numeric_finite(&current)) {
        for (unsigned refinement = 0; refinement < DD2_GROUP_NEWTON_REFINEMENTS; ++refinement) {
            if (current < dd2_group_velocity_tolerance) {
                break;
            }
            if (dd2_group_newton_step(workspace, &current, model, &system)) {
                continue;
            }
            if (!dd2_group_normal_patch(workspace) ||
                !dd2_group_newton_step(
                    workspace, &current,
                    (dd2_group_newton_model){.method = DD2_GROUP_NEWTON_PROJECTED}, &system)) {
                break;
            }
        }
        if (current < *error) {
            *error = current;
            return true;
        }
    }
    dd2_group_restore(workspace, &original.base, original.motion);
    return false;
}

/* Compare selective car-pair saturation and world-pressure branches in the
 * same connected field. World-only fields also refine their linear friction
 * direction through a sliding-to-linear transition. All models stay local to
 * one solve and preserve the final material law. */
/* Dependent co-oriented rows can require several simultaneous releases.
 * Compare one retained support with two separated endpoints. The second
 * endpoint permits torque support when matching normals have distinct arms.
 * Both branches preserve every final normal inequality and material check. */
static dd2_group_newton_model dd2_group_patch_model(const dd2_group_workspace *workspace,
                                                    unsigned retained, bool endpoints) {
    const dd2_group_contact target = workspace->query->contacts[retained];
    dd2_group_newton_model model = {.method = DD2_GROUP_NEWTON_PATCH,
                                    .selected_contact = retained,
                                    .second_contact = DD2_VEHICLE_CONTACT_LIMIT};
    if (dd2_group_pair(target)) {
        return model;
    }
    double farthest = -1;
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        const dd2_group_contact contact = workspace->query->contacts[index];
        const dd2_vehicle_vector difference =
            dd2_collision_add(contact.normal, dd2_collision_scale(target.normal, -1));
        if (index != retained && !dd2_group_pair(contact) && contact.first == target.first &&
            dd2_collision_dot(difference, difference) <
                dd2_group_axis_tolerance * dd2_group_axis_tolerance) {
            model.released_contacts |= UINT64_C(1) << index;
            const dd2_vehicle_vector arm =
                dd2_collision_add(contact.point, dd2_collision_scale(target.point, -1));
            const double distance = dd2_collision_dot(arm, arm);
            if (endpoints && dd2_numeric_finite(&distance) && distance > farthest) {
                farthest = distance;
                model.second_contact = index;
            }
        }
    }
    if (model.second_contact != DD2_VEHICLE_CONTACT_LIMIT) {
        model.released_contacts &= ~(UINT64_C(1) << model.second_contact);
    }
    return model;
}

static unsigned dd2_group_newton_models(const dd2_group_workspace *workspace,
                                        dd2_group_newton_model *models, bool paired) {
    unsigned count = 0;
    if (paired) {
        models[count++] = (dd2_group_newton_model){.method = DD2_GROUP_NEWTON_CONSTITUTIVE,
                                                   .selected_contact = DD2_VEHICLE_CONTACT_LIMIT};
        models[count++] = (dd2_group_newton_model){.method = DD2_GROUP_NEWTON_SATURATED,
                                                   .selected_contact = DD2_VEHICLE_CONTACT_LIMIT};
    } else {
        models[count++] = (dd2_group_newton_model){.method = DD2_GROUP_NEWTON_WORLD_LINEAR,
                                                   .selected_contact = DD2_VEHICLE_CONTACT_LIMIT};
    }
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        const dd2_group_contact contact = workspace->query->contacts[index];
        const dd2_group_newton_model patch = dd2_group_patch_model(workspace, index, false);
        if (patch.released_contacts != 0) {
            models[count++] = patch;
        }
        const dd2_group_newton_model endpoints = dd2_group_patch_model(workspace, index, true);
        if (endpoints.released_contacts != 0 &&
            endpoints.released_contacts != patch.released_contacts) {
            models[count++] = endpoints;
        }
        if (contact.friction <= 0 || workspace->constraints[index].normal_impulse <= 0) {
            continue;
        }
        if (!dd2_group_material_at(workspace, index, false).sliding) {
            models[count++] = (dd2_group_newton_model){.method = dd2_group_pair(contact)
                                                                     ? DD2_GROUP_NEWTON_SATURATED
                                                                     : DD2_GROUP_NEWTON_LOAD,
                                                       .selected_contact = index};
        }
        if (!dd2_group_pair(contact)) {
            models[count++] = (dd2_group_newton_model){.method = DD2_GROUP_NEWTON_RELEASE,
                                                       .selected_contact = index};
        }
    }
    return count;
}

static bool dd2_group_newton(dd2_group_workspace *workspace, double *error, bool coordinate) {
    const dd2_group_iterate base = dd2_group_values(workspace);
    dd2_group_motion motion[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_group_copy_motion(workspace, motion);
    const double original_error = *error;
    bool corrected = dd2_group_newton_refitted(workspace, error);
    bool paired = false;
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        paired = paired || dd2_group_pair(workspace->query->contacts[index]);
    }
    if (!coordinate) {
        return corrected;
    }
    dd2_group_iterate best = dd2_group_values(workspace);
    dd2_group_motion best_motion[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_group_copy_motion(workspace, best_motion);
    double best_error = *error;
    dd2_group_restore(workspace, &base, motion);
    dd2_group_newton_model models[DD2_GROUP_NEWTON_MODEL_LIMIT] = {0};
    const unsigned count = dd2_group_newton_models(workspace, models, paired);
    for (unsigned index = 0; index < count; ++index) {
        dd2_group_restore(workspace, &base, motion);
        if (models[index].method == DD2_GROUP_NEWTON_SATURATED &&
            models[index].selected_contact == DD2_VEHICLE_CONTACT_LIMIT &&
            !dd2_group_regularized_load(workspace)) {
            continue;
        }
        double refined_error = original_error;
        const bool refined = dd2_group_newton_refine(workspace, &refined_error, models[index]);
        if (refined && (!corrected || refined_error < best_error)) {
            best = dd2_group_values(workspace);
            dd2_group_copy_motion(workspace, best_motion);
            best_error = refined_error;
            corrected = true;
        }
    }
    dd2_group_restore(workspace, &best, best_motion);
    *error = best_error;
    return corrected;
}

static bool dd2_group_newton_advance(dd2_group_workspace *workspace, unsigned pass,
                                     dd2_group_solution *result) {
    if (pass + 1 < DD2_GROUP_NEWTON_DELAY || (pass + 1) % DD2_GROUP_NEWTON_PERIOD != 0) {
        return false;
    }
    const bool corrected =
        dd2_group_newton(workspace, &result->velocity_error, result->coordinate_restarts > 0);
    result->accelerated_passes += (unsigned)corrected;
    return corrected;
}

static void dd2_group_sweep(dd2_group_workspace *workspace) {
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        const dd2_group_contact contact = workspace->query->contacts[index];
        dd2_group_constraint *constraint = &workspace->constraints[index];
        const double speed =
            dd2_collision_dot(dd2_group_velocity(workspace, index), contact.normal);
        /* Exact unilateral coordinate response avoids amplifying coupled
         * friction modes; guarded corrections handle slow convergence. */
        const double next = fmax(0, constraint->normal_impulse - (speed / constraint->normal_mass));
        dd2_group_apply(workspace, index,
                        dd2_collision_scale(contact.normal, next - constraint->normal_impulse));
        constraint->normal_impulse = next;
        const dd2_group_friction candidate = dd2_group_friction_candidate(workspace, index);
        dd2_group_apply(
            workspace, index,
            dd2_collision_add(candidate.impulse, dd2_collision_scale(constraint->friction, -1)));
        constraint->friction = candidate.impulse;
    }
}

static void dd2_group_accelerate(dd2_group_workspace *workspace, const dd2_group_iterate *before,
                                 dd2_group_history *history, dd2_group_solution *result) {
    const dd2_group_iterate image = dd2_group_values(workspace);
    const dd2_group_iterate step =
        dd2_group_subtract(&image, before, workspace->query->contact_count);
    if (history->ready) {
        const dd2_group_prediction prediction = {history, &image, &step};
        const dd2_group_extrapolation outcome =
            dd2_group_extrapolate(workspace, &prediction, &result->velocity_error);
        result->accelerated_passes += (unsigned)(outcome == DD2_GROUP_EXTRAPOLATION_ACCEPTED);
        result->rejected_extrapolations += (unsigned)(outcome == DD2_GROUP_EXTRAPOLATION_REJECTED);
    }
    *history = (dd2_group_history){.image = image, .step = step, .ready = true};
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
        dd2_group_sweep(workspace);
        result->velocity_passes = pass + 1;
        result->velocity_error = dd2_group_velocity_error(workspace);
        if (result->velocity_error < dd2_group_velocity_tolerance) {
            return true;
        }
        const bool corrected = dd2_group_newton_advance(workspace, pass, result);
        if (corrected) {
            /* A Newton correction changes the fixed-point image. Do not mix
             * its displacement into an ordinary-sweep secant or retain stale
             * history across that correction. */
            history.ready = false;
        } else if (accelerated) {
            dd2_group_accelerate(workspace, &before, &history, result);
        }
        /* Predictions can leave a tiny positive impulse at a separating
         * contact. Require the next ordinary sweep to resolve the unilateral
         * active set before reporting convergence, within the same budget. */
        if (!accelerated) {
            continue;
        }
        if (result->velocity_error < progress_error * dd2_group_progress_fraction) {
            progress_error = result->velocity_error;
            stalled = 0;
        } else {
            ++stalled;
        }
        if (stalled >= DD2_GROUP_STALLED_PASSES) {
            /* Restart once from exact input motion with zero impulses and
             * secants disabled. Delayed Newton remains available after the
             * active constraints settle, within the same total pass budget. */
            const dd2_group_iterate empty = {0};
            dd2_group_restore(workspace, &empty, initial);
            accelerated = false;
            ++result->coordinate_restarts;
            history.ready = false;
        }
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

typedef struct {
    double impulses[DD2_VEHICLE_CONTACT_LIMIT];
    dd2_vehicle_vector offsets[DD2_VEHICLE_FLEET_LIMIT];
} dd2_group_position_state;

static dd2_group_position_state
dd2_group_position_save(const dd2_group_workspace *workspace,
                        const dd2_vehicle_vector offsets[DD2_VEHICLE_FLEET_LIMIT]) {
    dd2_group_position_state state = {0};
    for (unsigned body = 0; body < workspace->query->body_count; ++body) {
        state.offsets[body] = offsets[body];
    }
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        state.impulses[index] = workspace->constraints[index].position_impulse;
    }
    return state;
}

static void dd2_group_position_restore(dd2_group_workspace *workspace,
                                       dd2_vehicle_vector offsets[DD2_VEHICLE_FLEET_LIMIT],
                                       const dd2_group_position_state *state) {
    for (unsigned body = 0; body < workspace->query->body_count; ++body) {
        offsets[body] = state->offsets[body];
    }
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        workspace->constraints[index].position_impulse = state->impulses[index];
    }
}

/* Nearly coincident response columns can retain positive multipliers on
 * weaker inequalities for thousands of sweeps. Predict their release toward
 * the strongest matching row, retaining every inequality in physical checks. */
static unsigned dd2_group_position_target(const dd2_group_workspace *workspace, unsigned index) {
    const dd2_group_contact source = workspace->query->contacts[index];
    unsigned target = index;
    for (unsigned candidate = 0; candidate < workspace->query->contact_count; ++candidate) {
        const dd2_group_contact contact = workspace->query->contacts[candidate];
        const dd2_vehicle_vector difference =
            dd2_collision_add(contact.normal, dd2_collision_scale(source.normal, -1));
        if (contact.first == source.first && contact.second == source.second &&
            dd2_collision_dot(difference, difference) <
                dd2_group_axis_tolerance * dd2_group_axis_tolerance &&
            dd2_group_required_offset(contact) >
                dd2_group_required_offset(workspace->query->contacts[target])) {
            target = candidate;
        }
    }
    return target;
}

typedef struct {
    unsigned index;
    double impulse;
} dd2_group_position_shift;

static void dd2_group_position_delta(const dd2_group_workspace *workspace,
                                     dd2_vehicle_vector offsets[DD2_VEHICLE_FLEET_LIMIT],
                                     dd2_group_position_shift shift) {
    const dd2_group_contact contact = workspace->query->contacts[shift.index];
    const dd2_vehicle_vector delta = dd2_collision_scale(contact.normal, shift.impulse);
    offsets[contact.first] = dd2_collision_add(offsets[contact.first], delta);
    if (dd2_group_pair(contact)) {
        offsets[contact.second] =
            dd2_collision_add(offsets[contact.second], dd2_collision_scale(delta, -1));
    }
}

static bool dd2_group_position_transfer(dd2_group_workspace *workspace, bool *selected) {
    bool changed = false;
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        const unsigned target = dd2_group_position_target(workspace, index);
        const double impulse = workspace->constraints[index].position_impulse;
        if (target != index && impulse > 0) {
            workspace->constraints[target].position_impulse += impulse;
            workspace->constraints[index].position_impulse = 0;
            selected[target] = true;
            changed = true;
        }
    }
    return changed;
}

static void dd2_group_position_rebuild(const dd2_group_workspace *workspace,
                                       dd2_vehicle_vector offsets[DD2_VEHICLE_FLEET_LIMIT]) {
    for (unsigned body = 0; body < workspace->query->body_count; ++body) {
        offsets[body] = (dd2_vehicle_vector){0};
    }
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        dd2_group_position_delta(
            workspace, offsets,
            (dd2_group_position_shift){.index = index,
                                       .impulse = workspace->constraints[index].position_impulse});
    }
}

static void dd2_group_position_project(dd2_group_workspace *workspace,
                                       dd2_vehicle_vector offsets[DD2_VEHICLE_FLEET_LIMIT],
                                       unsigned index) {
    const dd2_group_contact contact = workspace->query->contacts[index];
    dd2_group_constraint *constraint = &workspace->constraints[index];
    const double gap =
        dd2_collision_dot(dd2_group_relative_offset(contact, offsets), contact.normal);
    const double next =
        fmax(0, constraint->position_impulse + ((dd2_group_required_offset(contact) - gap) /
                                                (dd2_group_pair(contact) ? 2 : 1)));
    dd2_group_position_delta(
        workspace, offsets,
        (dd2_group_position_shift){.index = index, .impulse = next - constraint->position_impulse});
    constraint->position_impulse = next;
}

static bool dd2_group_position_finite(const dd2_group_workspace *workspace,
                                      const dd2_vehicle_vector offsets[DD2_VEHICLE_FLEET_LIMIT]) {
    for (unsigned body = 0; body < workspace->query->body_count; ++body) {
        if (!dd2_group_vector_finite(&offsets[body])) {
            return false;
        }
    }
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        const double *impulse = &workspace->constraints[index].position_impulse;
        if (!dd2_numeric_finite(impulse) || *impulse < 0) {
            return false;
        }
    }
    return true;
}

/* This bounded correction changes multipliers, never the query's contacts.
 * Rebuild offsets from their response columns to preserve least-norm
 * stationarity and equal/opposite pair shifts. Project only recipients, then
 * accept a smaller full finite residual or restore exact saved state. */
static bool dd2_group_position_prediction(dd2_group_workspace *workspace,
                                          dd2_vehicle_vector offsets[DD2_VEHICLE_FLEET_LIMIT],
                                          double *error) {
    const dd2_group_position_state original = dd2_group_position_save(workspace, offsets);
    bool selected[DD2_VEHICLE_CONTACT_LIMIT] = {false};
    if (!dd2_group_position_transfer(workspace, selected)) {
        return false;
    }
    dd2_group_position_rebuild(workspace, offsets);
    for (unsigned index = 0; index < workspace->query->contact_count; ++index) {
        if (selected[index]) {
            dd2_group_position_project(workspace, offsets, index);
        }
    }
    if (dd2_group_position_finite(workspace, offsets)) {
        const double predicted = dd2_group_position_error(workspace, offsets);
        if (dd2_numeric_finite(&predicted) && predicted < *error) {
            *error = predicted;
            return true;
        }
    }
    dd2_group_position_restore(workspace, offsets, &original);
    return false;
}

static void dd2_group_position_rows(const dd2_group_workspace *workspace,
                                    const bool released[DD2_VEHICLE_CONTACT_LIMIT],
                                    dd2_group_linear_system *system) {
    for (unsigned column = 0; column < workspace->query->contact_count; column++) {
        dd2_vehicle_vector response[DD2_VEHICLE_FLEET_LIMIT] = {0};
        dd2_group_position_delta(workspace, response,
                                 (dd2_group_position_shift){.index = column, .impulse = 1});
        for (unsigned row = 0; row < workspace->query->contact_count; row++) {
            const dd2_group_contact contact = workspace->query->contacts[row];
            const double identity = row == column ? 1 : 0;
            system->entries[row][column] =
                released[row] ? identity
                              : dd2_collision_dot(dd2_group_relative_offset(contact, response),
                                                  contact.normal);
        }
    }
    for (unsigned row = 0; row < workspace->query->contact_count; row++) {
        system->entries[row][system->dimensions] =
            released[row] ? 0 : dd2_group_required_offset(workspace->query->contacts[row]);
    }
}

/* Solve the active least-norm translation equations from exact unit responses.
 * Each refit releases at least one negative multiplier, so contact_count + 1
 * solves suffice. Singular systems leave the iterate unchanged. All contacts
 * remain in the finite residual; an unhelpful fit restores exact state. */
static bool dd2_group_position_fit(dd2_group_workspace *workspace,
                                   dd2_vehicle_vector offsets[DD2_VEHICLE_FLEET_LIMIT],
                                   double *error) {
    const dd2_group_position_state original = dd2_group_position_save(workspace, offsets);
    bool released[DD2_VEHICLE_CONTACT_LIMIT] = {false};
    for (unsigned i = 0; i < workspace->query->contact_count; i++) {
        const dd2_group_contact contact = workspace->query->contacts[i];
        const double remaining =
            dd2_group_required_offset(contact) -
            dd2_collision_dot(dd2_group_relative_offset(contact, offsets), contact.normal);
        released[i] = workspace->constraints[i].position_impulse <= dd2_group_active_tolerance &&
                      remaining <= 0;
    }
    dd2_group_linear_system system = {.dimensions = workspace->query->contact_count};
    for (unsigned attempt = 0; attempt <= workspace->query->contact_count; attempt++) {
        dd2_group_position_rows(workspace, released, &system);
        if (!dd2_group_linear_solve(&system, system.dimensions)) {
            return false;
        }
        bool negative = false;
        for (unsigned i = 0; i < workspace->query->contact_count; i++) {
            if (!released[i] && system.direction[i] < 0) {
                released[i] = true;
                negative = true;
            }
        }
        if (negative) {
            continue;
        }
        for (unsigned i = 0; i < workspace->query->contact_count; i++) {
            workspace->constraints[i].position_impulse = system.direction[i];
        }
        dd2_group_position_rebuild(workspace, offsets);
        if (dd2_group_position_finite(workspace, offsets)) {
            const double predicted = dd2_group_position_error(workspace, offsets);
            if (dd2_numeric_finite(&predicted) && predicted < *error) {
                *error = predicted;
                return true;
            }
        }
        dd2_group_position_restore(workspace, offsets, &original);
        return false;
    }
    return false;
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
        if (pass + 1 >= DD2_GROUP_NEWTON_DELAY && (pass + 1) % DD2_GROUP_NEWTON_PERIOD == 0) {
            result->position_predictions += (unsigned)dd2_group_position_prediction(
                workspace, offsets, &result->position_error);
            if (result->position_error >= dd2_group_position_tolerance) {
                result->position_predictions +=
                    (unsigned)dd2_group_position_fit(workspace, offsets, &result->position_error);
            }
        }

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
