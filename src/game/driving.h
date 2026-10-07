#ifndef DD2_GAME_DRIVING_H
#define DD2_GAME_DRIVING_H

#include "ai/driver.h"
#include "assets/road.h"
#include "game/accidents.h"
#include "game/course.h"
#include "game/laps.h"
#include "game/race.h"
#include "game/recovery.h"
#include "game/sound_events.h"
#include "physics/damage.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct dd2_driving dd2_driving;
typedef struct {
    double seconds;
    dd2_vehicle_control control;
} dd2_driving_frame;

/* Owns the surface/barrier indices and up to twenty vehicle states; borrows road until
 * destruction. Selects all original grid slots without original memory.
 * A frame advances bounded fixed steps transactionally. Pause discards partial
 * elapsed time; reset restores the same settled field, independent of rendering.
 * Slot zero is the player; other bodies use driving AI by default. */
dd2_driving *dd2_driving_create(const dd2_road *road, unsigned level);
/* Driver IDs remain stable with human zero; supplied physical slots form a
 * permutation of twenty entries. NULL uses the original identity order.
 * Starts are copied; caller storage is not retained. Invalid maps fail before
 * allocation. Reset and mode changes preserve each driver's physical start. */
dd2_driving *dd2_driving_create_grid(const dd2_road *road, unsigned level,
                                     const unsigned *slot_for_driver);
void dd2_driving_destroy(dd2_driving *driving);
/* Validates bounded elapsed time and finite normalized analog controls. */
bool dd2_driving_frame_valid(dd2_driving_frame frame);
bool dd2_driving_advance(dd2_driving *driving, dd2_driving_frame frame);
bool dd2_driving_reset(dd2_driving *driving);
void dd2_driving_suspend(dd2_driving *driving);
/* Disable driving decisions for stationary-field collision comparisons. */
void dd2_driving_set_opponents(dd2_driving *driving, bool enabled);
bool dd2_driving_opponents(const dd2_driving *driving);
const dd2_ai_driver *dd2_driving_drivers(const dd2_driving *driving);
/* Borrowed contacts from the last fixed step of the last successful frame.
 * Reset and a frame without fixed steps publish an empty report. This snapshot
 * is diagnostic; gameplay consumers process each report inside the fixed step. */
const dd2_vehicle_collision_report *dd2_driving_contact_report(const dd2_driving *driving);
/* Frame-wide fixed-step sound events, published with the successful simulation
 * transaction. Reset/withdraw/no-step frames clear the batch; consume it once. */
const dd2_sound_batch *dd2_driving_sound_events(const dd2_driving *driving);
/* Damage is enabled by default. Disable only for isolated kinematic probes;
 * disabled damage state freezes. Reset restores every car's intact state. */
void dd2_driving_set_damage(dd2_driving *driving, bool enabled);
bool dd2_driving_damage_enabled(const dd2_driving *driving);
const dd2_vehicle_damage *dd2_driving_damage(const dd2_driving *driving);
/* Temporary supported overturns and source-timed righting; independent of
 * engine retirement. Reset clears counters; pause/countdown/results freeze them. */
const dd2_recovery_driver *dd2_driving_recovery(const dd2_driving *driving);
/* Borrow vehicle_count accident scores/attribution windows. Reset clears every
 * score; pause freezes the windows. Race/championship standings are separate. */
const dd2_accident_driver *dd2_driving_accidents(const dd2_driving *driving);
/* Racing levels expose source-equivalent courses and vehicle_count lap states.
 * Arenas return NULL. Progress/timing advances inside each fixed step; pause
 * freezes it, reset restores the grid approach. Finished flags are individual
 * lap completion; mode-specific race endings/results are separate. */
const dd2_course *dd2_driving_course(const dd2_driving *driving);
const dd2_lap_driver *dd2_driving_laps(const dd2_driving *driving);
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

/* Start/reset Wrecking, Stockcar, Time Trial or Total Destruction, or return to the free-driving
 * grid. Countdown/results hold all physics and clocks; active race observation is transactional
 * with the complete field. Stockcar/Time Trial arenas are rejected. Time Trial resets to one
 * physical car and a continuous course. Total Destruction requires an arena; its opponents always
 * pursue the player. Leaving Time Trial restores the original lap limit and twenty-car field.
 * Course pointers are invalidated by a successful mode change. */
bool dd2_driving_set_race(dd2_driving *driving, bool enabled, dd2_race_mode mode);
const dd2_race *dd2_driving_race(const dd2_driving *driving);
bool dd2_driving_withdraw(dd2_driving *driving);

#endif
