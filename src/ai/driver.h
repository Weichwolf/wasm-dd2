#ifndef DD2_AI_DRIVER_H
#define DD2_AI_DRIVER_H

#include "assets/road.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const dd2_road *road;
    uint32_t cell;
    unsigned slot;
    unsigned count;
} dd2_ai_start;
typedef struct {
    double lane;
    double base_lane;
    uint64_t steps;
    /* Accepted horizontal displacement since the current forward attempt. */
    dd2_vehicle_vector progress_origin;
    uint32_t cell;
    unsigned target;
    unsigned stuck_steps;
    unsigned reverse_steps;
    unsigned progress_steps;
} dd2_ai_driver;
typedef struct {
    const dd2_road *road;
    const dd2_road_surface *surface;
    const dd2_vehicle *vehicles;
    unsigned count;
    unsigned slot;
    /* Arena opponents only. Keep player slot zero as the target even across
     * normal retarget intervals and temporary inverted/braking decisions. */
    bool pursue_player;
} dd2_ai_observation;

/* One fixed-step decision from a simultaneous immutable field observation.
 * Racing follows source road links and brakes for curves/traffic; arenas pursue
 * another car. Timed reverse maneuvers release stalled cars. These are rewrite
 * tuning, not the original byte-command interpreter. No allocations/randomness.
 * Invalid input clears control and preserves all driver state. */
bool dd2_ai_driver_reset(dd2_ai_driver *driver, dd2_ai_start start);
bool dd2_ai_driver_step(dd2_ai_driver *driver, const dd2_ai_observation *observation,
                        dd2_vehicle_control *control);

#endif
