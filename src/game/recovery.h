#ifndef DD2_GAME_RECOVERY_H
#define DD2_GAME_RECOVERY_H

#include "assets/road.h"
#include "physics/barrier_world.h"
#include "physics/damage.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"

#include <stdbool.h>
#include <stdint.h>

enum { DD2_RECOVERY_REST_STEPS = 400 };

typedef struct {
    uint64_t recoveries;
    unsigned rest_steps;
    bool overturned;
} dd2_recovery_driver;

typedef struct {
    const dd2_road *road;
    const dd2_road_surface *surface;
    const dd2_barrier_world *world;
    const dd2_vehicle_damage *damage;
    unsigned count;
} dd2_recovery_frame;

/* Observe the already resolved physical field once per 5 ms step. A resting,
 * supported roof/side marks a car temporarily unavailable without retiring its
 * engine. Two seconds of continuous rest right the living player; NPCs must
 * additionally be more than 8192 XZ units away. Retry a blocked NPC after the
 * original deadline rather than losing the opportunity permanently.
 * Local road support supplies landing tilt/height; world overlaps are corrected
 * before publication. Pair response remains in the next ordinary physical step.
 * Damage, vehicle step clocks and ownership are unchanged. No allocation.
 * Reset with zeroed driver states; pause/countdown/results call no recovery step.
 * Invalid inputs/overflow preserve all drivers and bodies transactionally. */
bool dd2_recovery_step(dd2_recovery_driver *drivers, dd2_vehicle *vehicles,
                       dd2_recovery_frame frame);

#endif
