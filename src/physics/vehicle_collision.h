#ifndef DD2_PHYSICS_VEHICLE_COLLISION_H
#define DD2_PHYSICS_VEHICLE_COLLISION_H

#include "physics/barrier_world.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"

#include <stdbool.h>

typedef struct {
    unsigned contacts;
    double normal_speed;
    double impulse; /* impulse / mass */
    dd2_vehicle_vector point;
} dd2_vehicle_impact;

enum { DD2_VEHICLE_BODY_CORNERS = 8 };
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

#endif
