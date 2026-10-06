#ifndef DD2_GAME_DRIVING_H
#define DD2_GAME_DRIVING_H

#include "assets/road.h"
#include "physics/vehicle.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct dd2_driving dd2_driving;
typedef struct {
    double seconds;
    dd2_vehicle_control control;
} dd2_driving_frame;

/* Owns the surface index; borrows road until destruction. Selects the original
 * first grid position (including alternate branches), without original memory.
 * A frame advances bounded fixed steps transactionally. Pause discards partial
 * elapsed time; reset restores the same settled start, independent of rendering. */
dd2_driving *dd2_driving_create(const dd2_road *road, unsigned level);
void dd2_driving_destroy(dd2_driving *driving);
bool dd2_driving_advance(dd2_driving *driving, dd2_driving_frame frame);
bool dd2_driving_reset(dd2_driving *driving);
void dd2_driving_suspend(dd2_driving *driving);
const dd2_vehicle_spawn *dd2_driving_start(const dd2_driving *driving);
const dd2_vehicle *dd2_driving_vehicle(const dd2_driving *driving);
uint64_t dd2_driving_collisions(const dd2_driving *driving);
double dd2_driving_wheel_roll(const dd2_driving *driving);

#endif
