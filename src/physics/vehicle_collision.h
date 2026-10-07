#ifndef DD2_PHYSICS_VEHICLE_COLLISION_H
#define DD2_PHYSICS_VEHICLE_COLLISION_H

#include "physics/barrier_world.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    unsigned contacts;
    unsigned pair_contacts;
    double normal_speed;
    double impulse; /* impulse / mass */
    dd2_vehicle_vector point;
} dd2_vehicle_impact;

enum {
    DD2_VEHICLE_BODY_CORNERS = 8,
    DD2_VEHICLE_FLEET_LIMIT = 20,
    DD2_VEHICLE_CONTACT_LIMIT = 64,
    DD2_VEHICLE_NO_PARTNER = DD2_VEHICLE_FLEET_LIMIT
};

typedef enum {
    DD2_VEHICLE_CONTACT_GROUND,
    DD2_VEHICLE_CONTACT_BARRIER,
    DD2_VEHICLE_CONTACT_PAIR
} dd2_vehicle_contact_kind;

typedef struct {
    dd2_vehicle_vector point;           /* World contact before overlap correction. */
    dd2_vehicle_vector normal;          /* World impulse direction on first body. */
    dd2_vehicle_vector local_points[2]; /* First/second body contact-space arms. */
    double time;                        /* Fraction of the complete fixed step, in solver order. */
    double normal_speed;                /* Pre-response closing speed, zero when separating. */
    double impulse;                     /* Normal impulse / body mass; excludes tangent friction. */
    unsigned first;
    unsigned second;   /* NO_PARTNER for world contacts. */
    uint32_t obstacle; /* Road cell/barrier index, UINT32_MAX for pairs. */
    dd2_vehicle_contact_kind kind;
} dd2_vehicle_contact;

typedef struct {
    dd2_vehicle_impact impacts[DD2_VEHICLE_FLEET_LIMIT];
    dd2_vehicle_contact contacts[DD2_VEHICLE_CONTACT_LIMIT];
    unsigned count;
    unsigned pair_contacts;
    unsigned unresolved_sweeps; /* Checked-pose stop without a physical response. */
} dd2_vehicle_collision_report;
/* Source contact box in local body coordinates. Out-of-range indices return
 * zero. The collision proxy is separate from visual mesh bounds. */
dd2_vehicle_vector dd2_vehicle_body_corner(unsigned corner);

/* Resolves one already integrated fixed step against source barriers. Rounded
 * body proxies follow the full pose; angular travel is conservatively padded.
 * Swept contacts split the remaining motion, apply restitution/friction and
 * world-inertia angular impulses, and correct overlaps. Bounded iterations
 * stop residual travel instead of tunneling. Failure preserves vehicle state.
 * This is barrier response; car pairs, body/ground contact and damage are separate. */
bool dd2_vehicle_collide_barriers(dd2_vehicle *vehicle, const dd2_vehicle *previous,
                                  const dd2_barrier_world *world, dd2_vehicle_impact *impact);

/* Combined earliest-contact solver: source body corners against one-sided road
 * triangles, plus optional barriers. Ground and barriers share the remaining
 * step, so a rebound cannot bypass either system. Ground friction and inelastic
 * impacts support inverted bodies. Rotation is approximated by short chords.
 * Initial ground overlap recovery is bounded to preserve stacked road levels.
 * Failure preserves the proposed state and clears impact. */
bool dd2_vehicle_collide_world(dd2_vehicle *vehicle, const dd2_vehicle *previous,
                               const dd2_road_surface *surface, const dd2_barrier_world *world,
                               dd2_vehicle_impact *impact);

/* All bodies share one earliest-event clock for this already integrated step.
 * Pair impulses use equal mass and opposite linear/angular impulses. Ground,
 * barriers and pair contacts are rechecked after every event. Arrays must hold
 * count bodies (1..20); proposed/previous may alias for overlap repair. Impact
 * output, when supplied, holds count entries. Validation/solver failure preserves
 * every proposed body and clears outputs; an invalid count leaves the impact
 * array untouched because its extent is unknown. Queries allocate nothing. */
bool dd2_vehicle_collide_fleet(dd2_vehicle *vehicles, const dd2_vehicle *previous, unsigned count,
                               const dd2_road_surface *surface, const dd2_barrier_world *world,
                               dd2_vehicle_impact *impacts, unsigned *pair_contacts);

/* Same shared solver, with every selected contact (including overlap-only
 * repairs). All entries refer to the pose at contact, before correction and
 * rebound travel. Local point zero belongs to first; point one belongs to
 * second for a pair and is zero for a world contact. At most 64 contacts are
 * selected, matching the solver budget: no successful response is dropped.
 * Damage/scoring consumers must ignore zero-impulse repair/support contacts as
 * appropriate. An unresolved pair sweep stops at its conservative time bound,
 * increments unresolved_sweeps and produces no contact record or impulse.
 * Output must not alias bodies; it is cleared on entry and
 * published only after every resulting body passes validation. NULL output is
 * allowed. Failure preserves all proposed bodies. No allocations or callbacks. */
bool dd2_vehicle_collide_fleet_report(dd2_vehicle *vehicles, const dd2_vehicle *previous,
                                      unsigned count, const dd2_road_surface *surface,
                                      const dd2_barrier_world *world,
                                      dd2_vehicle_collision_report *report);

#endif
