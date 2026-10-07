#include "audio/effects.h"

#include "assets/audio.h"
#include "audio/mixer.h"
#include "game/sound_events.h"
#include "physics/numeric.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    DD2_EFFECT_ENGINE = 0,
    DD2_EFFECT_IMPACT = 3,
    DD2_EFFECT_THREE = 11,
    DD2_EFFECT_TWO = 10,
    DD2_EFFECT_ONE = 9,
    DD2_EFFECT_GO = 8,
    DD2_EFFECT_ENGINE_GAIN = 80,
    DD2_EFFECT_ENGINE_IDLE = 6192,
    DD2_EFFECT_ENGINE_MAX = 28000,
    DD2_EFFECT_ENGINE_REV = 1200
};
static const double dd2_effect_engine_speed_pitch = 2.5;

struct dd2_effects {
    const dd2_sound_bank *bank;
    unsigned samples[DD2_MIXER_CHANNELS];
    unsigned levels[DD2_MIXER_CHANNELS];
    uint64_t played[DD2_SOUND_CUE_COUNT];
    unsigned gain;
};

static unsigned dd2_effect_sample(dd2_sound_cue cue) {
    static const unsigned samples[DD2_SOUND_CUE_COUNT] = {
        DD2_EFFECT_IMPACT, DD2_EFFECT_THREE, DD2_EFFECT_TWO, DD2_EFFECT_ONE, DD2_EFFECT_GO};
    return cue < DD2_SOUND_CUE_COUNT ? samples[cue] : DD2_EFFECT_NO_SAMPLE;
}

dd2_effects *dd2_effects_create(const dd2_sound_bank *bank) {
    const dd2_sound *engine = dd2_sound_bank_get(bank, DD2_EFFECT_ENGINE);
    if (engine == NULL || !engine->loop) {
        return NULL;
    }
    for (unsigned cue = 0; cue < DD2_SOUND_CUE_COUNT; ++cue) {
        if (dd2_sound_bank_get(bank, dd2_effect_sample((dd2_sound_cue)cue)) == NULL) {
            return NULL;
        }
    }
    dd2_effects *effects = calloc(1, sizeof(*effects));
    if (effects != NULL) {
        effects->bank = bank;
        effects->gain = DD2_MIXER_GAIN_ONE;
        for (unsigned channel = 0; channel < DD2_MIXER_CHANNELS; ++channel) {
            effects->samples[channel] = DD2_EFFECT_NO_SAMPLE;
        }
    }
    return effects;
}

void dd2_effects_destroy(dd2_effects *effects) {
    free(effects);
}

bool dd2_effects_reset(dd2_effects *effects, dd2_mixer *mixer) {
    if (effects == NULL || mixer == NULL) {
        return false;
    }
    for (unsigned channel = 0; channel < DD2_MIXER_CHANNELS; ++channel) {
        dd2_mixer_stop(mixer, channel);
        dd2_mixer_lock(mixer, channel, channel < 2);
        effects->samples[channel] = DD2_EFFECT_NO_SAMPLE;
        effects->levels[channel] = 0;
    }
    for (unsigned cue = 0; cue < DD2_SOUND_CUE_COUNT; ++cue) {
        effects->played[cue] = 0;
    }
    return true;
}

static unsigned dd2_effect_level(const dd2_effects *effects, unsigned level) {
    return ((level * effects->gain) + (DD2_MIXER_GAIN_ONE / 2)) / DD2_MIXER_GAIN_ONE;
}

static bool dd2_effect_update_valid(const dd2_effects *effects, const dd2_mixer *mixer,
                                    dd2_engine_sound engine, const dd2_sound_batch *events) {
    if (effects == NULL || mixer == NULL || events == NULL ||
        events->count > DD2_SOUND_EVENT_LIMIT || !dd2_numeric_finite(&engine.speed) ||
        !dd2_numeric_finite(&engine.throttle) || fabs(engine.throttle) > 1) {
        return false;
    }
    unsigned counts[DD2_SOUND_CUE_COUNT] = {0};
    for (unsigned index = 0; index < events->count; ++index) {
        const dd2_sound_event *event = &events->events[index];
        if (event->cue < DD2_SOUND_IMPACT || event->cue >= DD2_SOUND_CUE_COUNT ||
            event->gain > DD2_MIXER_GAIN_ONE || event->pan < DD2_MIXER_PAN_LEFT ||
            event->pan > DD2_MIXER_PAN_RIGHT) {
            return false;
        }
        ++counts[event->cue];
    }
    for (unsigned cue = 0; cue < DD2_SOUND_CUE_COUNT; ++cue) {
        if (UINT64_MAX - effects->played[cue] < counts[cue]) {
            return false;
        }
    }
    return true;
}

static void dd2_effect_engine(dd2_effects *effects, dd2_mixer *mixer, dd2_engine_sound engine) {
    if (!engine.running) {
        dd2_mixer_stop(mixer, 0);
        effects->samples[0] = DD2_EFFECT_NO_SAMPLE;
        return;
    }
    const double pitch =
        fmin(DD2_EFFECT_ENGINE_MAX,
             (double)DD2_EFFECT_ENGINE_IDLE +
                 (fmin(fabs(engine.speed), DD2_EFFECT_ENGINE_MAX) * dd2_effect_engine_speed_pitch) +
                 (fabs(engine.throttle) * (double)DD2_EFFECT_ENGINE_REV));
    const dd2_voice_config config = {.frequency = (uint32_t)lround(pitch),
                                     .gain = dd2_effect_level(effects, DD2_EFFECT_ENGINE_GAIN),
                                     .loop = true};
    dd2_voice_state voice = {0};
    dd2_mixer_voice(mixer, 0, &voice);
    if (voice.playing) {
        dd2_mixer_modify(mixer, 0, config);
    } else {
        const dd2_sound *sound = dd2_sound_bank_get(effects->bank, DD2_EFFECT_ENGINE);
        dd2_mixer_play(mixer, 0, &sound->pcm, config);
    }
    effects->samples[0] = DD2_EFFECT_ENGINE;
    effects->levels[0] = DD2_EFFECT_ENGINE_GAIN;
}

bool dd2_effects_update(dd2_effects *effects, dd2_mixer *mixer, dd2_engine_sound engine,
                        const dd2_sound_batch *events) {
    if (!dd2_effect_update_valid(effects, mixer, engine, events)) {
        return false;
    }
    dd2_effect_engine(effects, mixer, engine);
    for (unsigned index = 0; index < events->count; ++index) {
        const dd2_sound_event *event = &events->events[index];
        const unsigned sample = dd2_effect_sample(event->cue);
        const dd2_sound *sound = dd2_sound_bank_get(effects->bank, sample);
        const dd2_voice_config config = {.frequency = sound->pcm.sample_rate,
                                         .gain = dd2_effect_level(effects, event->gain),
                                         .pan = event->pan};
        const unsigned requested = event->cue == DD2_SOUND_IMPACT ? DD2_MIXER_AUTO_CHANNEL : 1;
        const int channel = dd2_mixer_play(mixer, requested, &sound->pcm, config);
        if (channel >= 0) {
            effects->samples[channel] = sample;
            effects->levels[channel] = event->gain;
            ++effects->played[event->cue];
        }
    }
    return true;
}

bool dd2_effects_gain(dd2_effects *effects, dd2_mixer *mixer, unsigned gain) {
    if (effects == NULL || mixer == NULL || gain > DD2_MIXER_GAIN_ONE) {
        return false;
    }
    effects->gain = gain;
    for (unsigned channel = 0; channel < DD2_MIXER_CHANNELS; ++channel) {
        dd2_voice_state voice = {0};
        dd2_mixer_voice(mixer, channel, &voice);
        if (voice.playing) {
            voice.config.gain = dd2_effect_level(effects, effects->levels[channel]);
            dd2_mixer_modify(mixer, channel, voice.config);
        }
    }
    return true;
}

dd2_effect_voice dd2_effects_voice(const dd2_effects *effects, const dd2_mixer *mixer,
                                   unsigned channel) {
    dd2_effect_voice state = {.sample = DD2_EFFECT_NO_SAMPLE};
    if (effects != NULL && dd2_mixer_voice(mixer, channel, &state.voice)) {
        state.sample = effects->samples[channel];
    }
    return state;
}

uint64_t dd2_effects_played(const dd2_effects *effects, dd2_sound_cue cue) {
    return effects != NULL && cue >= DD2_SOUND_IMPACT && cue < DD2_SOUND_CUE_COUNT
               ? effects->played[cue]
               : 0;
}
