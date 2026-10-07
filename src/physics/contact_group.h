#ifndef DD2_PHYSICS_CONTACT_GROUP_H
#define DD2_PHYSICS_CONTACT_GROUP_H

#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <stdbool.h>

/* Internal constraints at one already certified fleet event pose. The owner
 * collects the connected neighborhood and applies primary restitution first. */
typedef struct {
    dd2_vehicle_vector point;
    dd2_vehicle_vector normal;
    double penetration; /* Signed depth; nearby separating supports are negative. */
    double friction;
    unsigned first;
    unsigned second; /* DD2_VEHICLE_NO_PARTNER for the static world. */
} dd2_group_contact;

typedef struct {
    dd2_vehicle *bodies;
    const dd2_group_contact *contacts;
    unsigned body_count;
    unsigned contact_count;
} dd2_group_query;

typedef struct {
    dd2_vehicle_vector friction_impulse;
    double normal_impulse;
    double normal_mass;
    double initial_normal_speed;
    double final_normal_speed;
} dd2_group_response;

typedef struct {
    dd2_group_response contacts[DD2_VEHICLE_CONTACT_LIMIT];
    unsigned count;
    unsigned velocity_passes;
    unsigned position_passes;
    unsigned accelerated_passes;      /* Accepted finite predictions reducing physical residual. */
    unsigned coordinate_restarts;     /* At most one; velocity_passes includes both phases. */
    unsigned rejected_extrapolations; /* Invalid or non-improving candidates, without drift. */
    double velocity_error;            /* Normal complementarity and projected friction, units/s. */
    double position_error;            /* Normal position complementarity, world units. */
} dd2_group_solution;

/* Inelastic joint support response and equal-mass, least-norm position repair.
 * Car-pair friction is regularized below 0.1 world units/s: linear opposing impulse
 * within the cone, full Coulomb saturation above that microscopic slip speed.
 * Static-world supports preserve Coulomb sticking. This is rewrite tuning.
 * This operates on the fleet owner's private working copies; convergence or
 * final-state failure can leave those copies changed. The owner must discard
 * them transactionally on failure. Invalid input is rejected before mutation.
 * Input motion must be finite; primary response may temporarily exceed vehicle
 * integration speed/spin limits. Full vehicle limits apply to the final result.
 * Output is cleared on entry, published only on success and must not alias the
 * query or bodies. Contacts are borrowed only for this call, at most 64; the
 * owner preserves the existing report budget before invoking this routine.
 * No allocations, restitution, damage decisions, time advancement or queries. */
bool dd2_contact_group_solve(const dd2_group_query *query, dd2_group_solution *solution);

#endif
