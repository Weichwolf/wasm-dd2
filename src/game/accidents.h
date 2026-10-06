#ifndef DD2_GAME_ACCIDENTS_H
#define DD2_GAME_ACCIDENTS_H

#include "physics/vehicle_collision.h"

#include <stdbool.h>
#include <stdint.h>

enum { DD2_ACCIDENT_SCORE_LIMIT = 999, DD2_ACCIDENT_WINDOW_STEPS = 300 };

typedef struct {
    double heading; /* Horizontal forward direction, radians in [-pi, pi]. */
    bool retired;
} dd2_accident_observation;

typedef struct {
    double heading;
    double rotation;
    uint64_t steps;
    unsigned points;
    unsigned destructions;
    unsigned partner; /* Instigator slot, or DD2_VEHICLE_NO_PARTNER. */
    unsigned remaining;
    unsigned quarters; /* Largest signed net spin threshold reached: 0..2. */
    bool retired;
} dd2_accident_driver;

typedef struct {
    const dd2_vehicle_collision_report *contacts;
    const dd2_accident_observation *vehicles;
    unsigned count;
} dd2_accident_frame;

/* Source accident points: 90/180/360 degrees give 10/25/50, destruction
 * gives 25 plus one destruction, points cap at 999. Each responding pair
 * arms mutual attribution only for living, currently untracked victims.
 * Later contacts do not steal/refresh the 1.5 s window (75 original 20 ms
 * ticks, represented by 300 rewrite 5 ms steps). A full spin scores immediately;
 * other spin thresholds score on expiration. A retired instigator cancels the
 * credit. Wrecks cannot score or generate new attribution. No allocations or
 * retained pointers. Validation/counter overflow preserves all driver states.
 * Closing speed/impulse thresholds are rewrite tuning, excluding repairs and
 * resting contacts. Lap/race/championship points belong to separate rules. */
bool dd2_accidents_reset(dd2_accident_driver *drivers, const dd2_accident_observation *vehicles,
                         unsigned count);
bool dd2_accidents_step(dd2_accident_driver *drivers, dd2_accident_frame frame);

#endif
