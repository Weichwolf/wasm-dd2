#ifndef DD2_TESTS_DRIVING_SOUND_FIXTURE_H
#define DD2_TESTS_DRIVING_SOUND_FIXTURE_H

#include "game/sound_events.h"

#include <stdbool.h>

/* The driving test interposes one late consumer, delegating ordinary calls to
 * the unchanged production implementation compiled under this private name. */
bool dd2_drive_fixture_sound_step(dd2_sound_state *state, dd2_sound_batch *batch,
                                  dd2_sound_observation observation);

#endif
