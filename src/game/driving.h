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

/* Owns the surface/barrier indices and twenty vehicle states; borrows road until
 * destruction. Selects all original grid slots without original memory.
 * A frame advances bounded fixed steps transactionally. Pause discards partial
 * elapsed time; reset restores the same settled field, independent of rendering.
 * Slot zero is the player; other bodies hold their brakes until driving AI exists. */
dd2_driving *dd2_driving_create(const dd2_road *road, unsigned level);
void dd2_driving_destroy(dd2_driving *driving);
bool dd2_driving_advance(dd2_driving *driving, dd2_driving_frame frame);
bool dd2_driving_reset(dd2_driving *driving);
void dd2_driving_suspend(dd2_driving *driving);
const dd2_vehicle_spawn *dd2_driving_start(const dd2_driving *driving);
const dd2_vehicle *dd2_driving_vehicle(const dd2_driving *driving);
uint64_t dd2_driving_collisions(const dd2_driving *driving);
double dd2_driving_wheel_roll(const dd2_driving *driving);
unsigned dd2_driving_vehicle_count(const dd2_driving *driving);
/* Borrowed arrays have vehicle_count entries and remain owned by driving. */
const dd2_vehicle *dd2_driving_vehicles(const dd2_driving *driving);
const double *dd2_driving_wheel_rolls(const dd2_driving *driving);
/* Player contact counts include individual solver responses, not unique crashes. */
uint64_t dd2_driving_pair_collisions(const dd2_driving *driving);
const dd2_vehicle_spawn *dd2_driving_grid_start(const dd2_driving *driving, unsigned slot);

#endif
