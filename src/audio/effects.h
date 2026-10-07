#ifndef DD2_AUDIO_EFFECTS_H
#define DD2_AUDIO_EFFECTS_H

#include "assets/audio.h"
#include "audio/mixer.h"
#include "game/sound_events.h"

#include <stdbool.h>
#include <stdint.h>

enum { DD2_EFFECT_NO_SAMPLE = DD2_SOUND_BANK_LIMIT };
typedef struct dd2_effects dd2_effects;
typedef struct {
    bool running;
    double speed;    /* Signed speed along the player's forward axis, world units/s. */
    double throttle; /* Effective -1..1 control; coasting/retirement use zero. */
} dd2_engine_sound;
typedef struct {
    dd2_voice_state voice;
    unsigned sample;
} dd2_effect_voice;

/* Borrows immutable bank metadata/PCM until destruction. One owner controls the
 * mixer under its device lock. Channel 0 is a looping motor, 1 is countdown;
 * unlocked channels 2/3 rotate through spatial impacts. Cue indices follow the
 * source bank; motor pitch/levels and impact attenuation are rewrite tuning.
 * Reset stops effects and resets session counters, preserving music/master gain.
 * Invalid updates preserve effects and mixer. Consume each event batch once. */
dd2_effects *dd2_effects_create(const dd2_sound_bank *bank);
void dd2_effects_destroy(dd2_effects *effects);
bool dd2_effects_reset(dd2_effects *effects, dd2_mixer *mixer);
bool dd2_effects_update(dd2_effects *effects, dd2_mixer *mixer, dd2_engine_sound engine,
                        const dd2_sound_batch *events);
bool dd2_effects_gain(dd2_effects *effects, dd2_mixer *mixer, unsigned gain);
dd2_effect_voice dd2_effects_voice(const dd2_effects *effects, const dd2_mixer *mixer,
                                   unsigned channel);
uint64_t dd2_effects_played(const dd2_effects *effects, dd2_sound_cue cue);

#endif
