#ifndef DD2_GAME_RACE_H
#define DD2_GAME_RACE_H

#include "game/accidents.h"
#include "game/laps.h"
#include "game/recovery.h"
#include "physics/damage.h"
#include "physics/vehicle_collision.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum { DD2_RACE_WRECKING, DD2_RACE_STOCKCAR, DD2_RACE_TIME_TRIAL } dd2_race_mode;
typedef enum {
    DD2_RACE_COUNTDOWN,
    DD2_RACE_RUNNING,
    DD2_RACE_COASTING,
    DD2_RACE_RESULTS
} dd2_race_phase;
typedef enum {
    DD2_RACE_NO_END,
    DD2_RACE_PLAYER_FINISHED,
    DD2_RACE_PLAYER_RETIRED,
    DD2_RACE_LAST_SURVIVOR,
    DD2_RACE_WITHDRAWN
} dd2_race_end;
enum { DD2_RACE_START_STEPS = 400, DD2_RACE_COAST_STEPS = 600 };

typedef struct {
    dd2_race_mode mode;
    unsigned count;
    /* Zero length/laps selects a Wrecking arena. Time Trial requires a
     * circuit, zero lap limit and exactly one vehicle. */
    uint32_t length;
    unsigned laps;
} dd2_race_rules;
typedef struct {
    uint64_t finish_step;
    uint64_t retired_step;
    /* Lap clocks in 5 ms ticks, latched while alive; coasting freezes a
     * retired driver's current time. Last/best exclude the grid approach. */
    uint64_t current_lap_time;
    uint64_t last_lap_time;
    uint64_t best_lap_time;
    unsigned started_laps;
    unsigned credited_laps;
    uint32_t relative;
    unsigned place;
    unsigned finish_place;
    unsigned accident_points;
    unsigned finish_points;
    unsigned total_points;
    bool retired;
} dd2_race_driver;
typedef struct {
    dd2_race_rules rules;
    dd2_race_phase phase;
    dd2_race_end end;
    uint64_t steps;
    uint64_t elapsed;
    unsigned coasting;
    unsigned finishers;
    unsigned alive;
    dd2_race_driver drivers[DD2_VEHICLE_FLEET_LIMIT];
    /* Slot IDs in road/survival order and, at RESULTS, total-score order.
     * Drivers always remain indexed by stable slot; slot zero is the player. */
    unsigned order[DD2_VEHICLE_FLEET_LIMIT];
    unsigned results[DD2_VEHICLE_FLEET_LIMIT];
} dd2_race;
typedef struct {
    const dd2_lap_driver *laps;
    const dd2_vehicle_damage *damage;
    const dd2_accident_driver *accidents;
    /* Optional for rule-only callers: NULL means no temporary overturns.
     * An overturned car is unavailable for arena survival, not engine-retired. */
    const dd2_recovery_driver *recovery;
    unsigned count;
} dd2_race_observation;

/* No allocation or borrowed state retained. Each step is 5 ms. Hold the settled
 * field during the 2 s countdown; run physics before observing active/coasting
 * steps. Pause calls neither physics nor this module. Results are immutable.
 * Finish order is latched by crossing tick, then previous place for simultaneous
 * crossings. Arenas end with fewer than two survivors or player retirement.
 * Stockcar awards source placement points; Wrecking adds actual accident points.
 * Time Trial has one car and continuous laps, no finish bonuses or automatic
 * lap-limit ending. Withdrawal/retirement results retain lap times.
 * Original post-race randomized NPC scores are replaced by simulated scores.
 * Invalid rules/observations and counter overflow preserve the complete state. */
bool dd2_race_reset(dd2_race *race, dd2_race_rules rules, dd2_race_observation grid);
bool dd2_race_step(dd2_race *race, dd2_race_observation observation);
/* Voluntary exit publishes DNF results without manufacturing a finish flag. */
bool dd2_race_withdraw(dd2_race *race);
/* Three source countdown cues (91/61/31 of 100 original ticks), then GO.
 * Returns 0 outside countdown; presentation may show GO during elapsed < 100. */
unsigned dd2_race_countdown(const dd2_race *race);
unsigned dd2_race_finish_points(dd2_race_mode mode, unsigned place);

#endif
