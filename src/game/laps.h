#ifndef DD2_GAME_LAPS_H
#define DD2_GAME_LAPS_H

#include "game/course.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { DD2_LAP_TRACE_LIMIT = 64 };
typedef struct {
    uint64_t steps;
    uint64_t lap_start;
    uint64_t last_lap;
    uint64_t best_lap;
    uint64_t finish_step;
    uint32_t cell;
    uint32_t relative;
    uint32_t checkpoint;
    unsigned started_laps;
    unsigned credited_laps;
    bool retired;
    bool finished;
} dd2_lap_driver;
typedef struct {
    /* Ordered road contacts along this 5 ms motion. NO_STRIP means unsupported
     * (off-road/airborne) and cannot grant progress. Caller must sample actual
     * geometry; this rules layer never substitutes a nearest road or AI target. */
    const uint32_t *cells;
    size_t count;
    bool retired;
} dd2_lap_observation;

/* Seeds sequential progress at the source grid cell. Crossing the finish for
 * the first time starts lap 1, without crediting the partial grid approach.
 * Every finish-relative unit must then be visited in order. Reverse finish
 * crossings temporarily remove credit; recrossing restores it without earning
 * an extra lap. Lap timing excludes the grid approach, in fixed 5 ms steps.
 * Retirement freezes progress, finish freezes progress/timing; steps still count.
 * A zero-lap course continues timing without finish flags, up to the unsigned
 * counter limit. Invalid observations/state or counter overflow (including
 * multiple crossings within one trace) preserve the complete state. */
bool dd2_laps_reset(dd2_lap_driver *driver, const dd2_course *course, uint32_t cell);
bool dd2_laps_step(dd2_lap_driver *driver, const dd2_course *course,
                   dd2_lap_observation observation);
unsigned dd2_laps_completed(const dd2_lap_driver *driver);

#endif
