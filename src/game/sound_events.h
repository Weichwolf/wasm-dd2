#ifndef DD2_GAME_SOUND_EVENTS_H
#define DD2_GAME_SOUND_EVENTS_H

#include "game/race.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <stdbool.h>
#include <stdint.h>

enum { DD2_SOUND_EVENT_LIMIT = 64, DD2_SOUND_GAIN_ONE = 256, DD2_SOUND_COOLDOWN_STEPS = 30 };
typedef enum {
    DD2_SOUND_IMPACT,
    DD2_SOUND_THREE,
    DD2_SOUND_TWO,
    DD2_SOUND_ONE,
    DD2_SOUND_GO,
    DD2_SOUND_CUE_COUNT
} dd2_sound_cue;
typedef struct {
    dd2_sound_cue cue;
    uint64_t tick;
    unsigned gain; /* Linear amplitude 0..256. */
    int pan;       /* -256 left, +256 right in the chase view. */
} dd2_sound_event;
typedef struct {
    dd2_sound_event events[DD2_SOUND_EVENT_LIMIT];
    unsigned count;
} dd2_sound_batch;
typedef struct {
    uint64_t ticks;
    uint8_t cooldown[DD2_VEHICLE_FLEET_LIMIT][DD2_VEHICLE_FLEET_LIMIT + 1];
    unsigned countdown;
    bool waiting_for_go;
} dd2_sound_state;
typedef struct {
    const dd2_vehicle *listener;
    const dd2_vehicle_collision_report *contacts;
    const dd2_race *race; /* NULL for free driving. */
    unsigned count;
} dd2_sound_observation;

/* One successful 5 ms simulation tick. Appends countdown transitions and the
 * strongest nearby impact, ignoring repair-only/soft support contacts. A pair
 * or body's world contact cannot retrigger for 150 ms. Clear the output batch
 * at each rendered frame, but retain state between frames. Nothing runs during
 * pause/results. Reset uses {.waiting_for_go = racing}. No allocation/device
 * access. Invalid input/full output/clock overflow preserves state and batch. */
bool dd2_sound_events_step(dd2_sound_state *state, dd2_sound_batch *batch,
                           dd2_sound_observation observation);

#endif
