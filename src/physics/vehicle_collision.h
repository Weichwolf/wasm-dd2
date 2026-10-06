#ifndef DD2_PHYSICS_VEHICLE_COLLISION_H
#define DD2_PHYSICS_VEHICLE_COLLISION_H

#include "physics/barrier_world.h"
#include "physics/vehicle.h"

#include <stdbool.h>

typedef struct {
    unsigned contacts;
    double normal_speed;
    double impulse; /* impulse / mass */
    dd2_vehicle_vector point;
} dd2_vehicle_impact;

/* Resolves one already integrated fixed step against source barriers. Rounded
 * body proxies follow the full pose; angular travel is conservatively padded.
 * Swept contacts split the remaining motion, apply restitution/friction and
 * world-inertia angular impulses, and correct overlaps. Bounded iterations
 * stop residual travel instead of tunneling. Failure preserves vehicle state.
 * This is barrier response; car pairs, body/ground contact and damage are separate. */
bool dd2_vehicle_collide_barriers(dd2_vehicle *vehicle, const dd2_vehicle *previous,
                                  const dd2_barrier_world *world, dd2_vehicle_impact *impact);

#endif
