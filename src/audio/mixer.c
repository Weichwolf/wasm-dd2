#include "audio/mixer.h"

#include "assets/audio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum { DD2_MIXER_CDDA_RATE = 44100, DD2_MIXER_CDDA_BITS = 16 };

typedef struct {
    dd2_pcm_view pcm;
    dd2_voice_state state;
} dd2_mixer_voice_storage;

struct dd2_mixer {
    dd2_mixer_voice_storage voices[DD2_MIXER_CHANNELS];
    dd2_mixer_voice_storage music;
    dd2_music_state transport;
    uint32_t rate;
    unsigned next_channel;
    bool paused;
};

static bool dd2_mixer_config_valid(dd2_voice_config config) {
    return config.frequency > 0 && config.frequency <= DD2_MIXER_FREQUENCY_MAX &&
           config.gain <= DD2_MIXER_GAIN_ONE && config.pan >= DD2_MIXER_PAN_LEFT &&
           config.pan <= DD2_MIXER_PAN_RIGHT;
}

static bool dd2_mixer_pcm_valid(const dd2_pcm_view *pcm) {
    int16_t sample = 0;
    return dd2_pcm_sample(pcm, 0, 0, &sample);
}

dd2_mixer *dd2_mixer_create(uint32_t output_rate) {
    if (output_rate < DD2_MIXER_RATE_MIN || output_rate > DD2_MIXER_RATE_MAX) {
        return NULL;
    }
    dd2_mixer *mixer = calloc(1, sizeof(*mixer));
    if (mixer != NULL) {
        mixer->rate = output_rate;
        mixer->transport.gain = DD2_MIXER_GAIN_ONE;
    }
    return mixer;
}

void dd2_mixer_destroy(dd2_mixer *mixer) {
    free(mixer);
}

static int dd2_mixer_choose(const dd2_mixer *mixer) {
    for (unsigned offset = 0; offset < DD2_MIXER_CHANNELS; ++offset) {
        const unsigned channel = (mixer->next_channel + offset) % DD2_MIXER_CHANNELS;
        if (!mixer->voices[channel].state.locked) {
            return (int)channel;
        }
    }
    return -1;
}

int dd2_mixer_play(dd2_mixer *mixer, unsigned channel, const dd2_pcm_view *pcm,
                   dd2_voice_config config) {
    if (mixer == NULL || channel > DD2_MIXER_AUTO_CHANNEL || !dd2_mixer_config_valid(config) ||
        !dd2_mixer_pcm_valid(pcm)) {
        return -1;
    }
    const int selected = channel == DD2_MIXER_AUTO_CHANNEL ? dd2_mixer_choose(mixer) : (int)channel;
    if (selected < 0) {
        return -1;
    }
    dd2_mixer_voice_storage *voice = &mixer->voices[(unsigned)selected];
    *voice = (dd2_mixer_voice_storage){
        .pcm = *pcm, .state = {.config = config, .playing = true, .locked = voice->state.locked}};
    if (channel == DD2_MIXER_AUTO_CHANNEL) {
        mixer->next_channel = ((unsigned)selected + 1) % DD2_MIXER_CHANNELS;
    }
    return selected;
}

bool dd2_mixer_modify(dd2_mixer *mixer, unsigned channel, dd2_voice_config config) {
    if (mixer == NULL || channel >= DD2_MIXER_CHANNELS || !dd2_mixer_config_valid(config) ||
        !mixer->voices[channel].state.playing) {
        return false;
    }
    mixer->voices[channel].state.config = config;
    return true;
}

bool dd2_mixer_stop(dd2_mixer *mixer, unsigned channel) {
    if (mixer == NULL || channel >= DD2_MIXER_CHANNELS) {
        return false;
    }
    dd2_mixer_voice_storage *voice = &mixer->voices[channel];
    const bool locked = voice->state.locked;
    *voice = (dd2_mixer_voice_storage){.state = {.locked = locked}};
    return true;
}

bool dd2_mixer_lock(dd2_mixer *mixer, unsigned channel, bool locked) {
    if (mixer == NULL || channel >= DD2_MIXER_CHANNELS) {
        return false;
    }
    mixer->voices[channel].state.locked = locked;
    return true;
}

bool dd2_mixer_voice(const dd2_mixer *mixer, unsigned channel, dd2_voice_state *result) {
    if (result == NULL) {
        return false;
    }
    *result = (dd2_voice_state){0};
    if (mixer == NULL || channel >= DD2_MIXER_CHANNELS) {
        return false;
    }
    *result = mixer->voices[channel].state;
    return true;
}

bool dd2_mixer_set_paused(dd2_mixer *mixer, bool paused) {
    if (mixer == NULL) {
        return false;
    }
    mixer->paused = paused;
    return true;
}

bool dd2_mixer_music_select(dd2_mixer *mixer, unsigned track, const dd2_pcm_view *pcm,
                            bool repeat) {
    if (mixer == NULL || track < DD2_MIXER_FIRST_TRACK || track > DD2_MIXER_LAST_TRACK ||
        !dd2_mixer_pcm_valid(pcm) || pcm->channels != DD2_PCM_STEREO ||
        pcm->bits_per_sample != DD2_MIXER_CDDA_BITS || pcm->sample_rate != DD2_MIXER_CDDA_RATE) {
        return false;
    }
    const unsigned gain = mixer->transport.gain;
    mixer->music = (dd2_mixer_voice_storage){
        .pcm = *pcm,
        .state = {.config = {.frequency = DD2_MIXER_CDDA_RATE, .gain = gain, .loop = repeat}}};
    mixer->transport =
        (dd2_music_state){.phase = DD2_MUSIC_READY, .track = track, .gain = gain, .repeat = repeat};
    return true;
}

bool dd2_mixer_music_start(dd2_mixer *mixer) {
    if (mixer == NULL || mixer->transport.phase == DD2_MUSIC_EMPTY) {
        return false;
    }
    mixer->music.state.frame = 0;
    mixer->music.state.fraction = 0;
    mixer->music.state.playing = true;
    mixer->transport.phase = DD2_MUSIC_PLAYING;
    mixer->transport.frame = 0;
    mixer->transport.fraction = 0;
    return true;
}

bool dd2_mixer_music_pause(dd2_mixer *mixer) {
    if (mixer == NULL || mixer->transport.phase != DD2_MUSIC_PLAYING) {
        return false;
    }
    mixer->music.state.playing = false;
    mixer->transport.phase = DD2_MUSIC_PAUSED;
    return true;
}

bool dd2_mixer_music_resume(dd2_mixer *mixer) {
    if (mixer == NULL || mixer->transport.phase != DD2_MUSIC_PAUSED) {
        return false;
    }
    mixer->music.state.playing = true;
    mixer->transport.phase = DD2_MUSIC_PLAYING;
    return true;
}

bool dd2_mixer_music_stop(dd2_mixer *mixer) {
    if (mixer == NULL) {
        return false;
    }
    const unsigned gain = mixer->transport.gain;
    mixer->music = (dd2_mixer_voice_storage){0};
    mixer->transport = (dd2_music_state){.gain = gain};
    return true;
}

bool dd2_mixer_music_gain(dd2_mixer *mixer, unsigned gain) {
    if (mixer == NULL || gain > DD2_MIXER_GAIN_ONE) {
        return false;
    }
    mixer->transport.gain = gain;
    mixer->music.state.config.gain = gain;
    return true;
}

bool dd2_mixer_music(const dd2_mixer *mixer, dd2_music_state *result) {
    if (result == NULL) {
        return false;
    }
    *result = (dd2_music_state){0};
    if (mixer == NULL) {
        return false;
    }
    *result = mixer->transport;
    return true;
}

static int64_t dd2_mixer_divide(int64_t value, uint32_t divisor) {
    const int64_t half = (int64_t)divisor / 2;
    return (value + (value >= 0 ? half : -half)) / divisor;
}

static int64_t dd2_mixer_sample(const dd2_mixer_voice_storage *voice, const dd2_mixer *mixer,
                                unsigned channel) {
    const uint32_t rate = mixer->rate;
    const dd2_voice_state *state = &voice->state;
    const unsigned source_channel = voice->pcm.channels == 1 ? 0 : channel;
    size_t next = state->frame + 1;
    if (next == voice->pcm.frames) {
        next = state->config.loop ? 0 : state->frame;
    }
    int16_t first = 0;
    int16_t second = 0;
    if (!dd2_pcm_sample(&voice->pcm, state->frame, source_channel, &first) ||
        !dd2_pcm_sample(&voice->pcm, next, source_channel, &second)) {
        return 0;
    }
    const int64_t value =
        ((int64_t)first * (rate - state->fraction)) + ((int64_t)second * state->fraction);
    const int64_t sample = dd2_mixer_divide(value, rate);
    const int pan = state->config.pan;
    int attenuation = DD2_MIXER_GAIN_ONE;
    if (channel == 0 && pan > 0) {
        attenuation -= pan;
    }
    if (channel == 1 && pan < 0) {
        attenuation += pan;
    }
    return dd2_mixer_divide(sample * state->config.gain * attenuation,
                            DD2_MIXER_GAIN_ONE * DD2_MIXER_GAIN_ONE);
}

static void dd2_mixer_advance(dd2_mixer_voice_storage *voice, uint32_t rate) {
    const uint64_t phase = (uint64_t)voice->state.fraction + voice->state.config.frequency;
    const size_t steps = (size_t)(phase / rate);
    voice->state.fraction = (uint32_t)(phase % rate);
    const size_t remaining = voice->pcm.frames - voice->state.frame;
    if (steps < remaining) {
        voice->state.frame += steps;
    } else if (voice->state.config.loop) {
        voice->state.frame = (steps - remaining) % voice->pcm.frames;
    } else {
        voice->state.frame = voice->pcm.frames;
        voice->state.fraction = 0;
        voice->state.playing = false;
    }
}

static int16_t dd2_mixer_saturate(int64_t sample) {
    if (sample > INT16_MAX) {
        return INT16_MAX;
    }
    if (sample < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)sample;
}

static void dd2_mixer_channels(dd2_mixer *mixer, int64_t channels[DD2_PCM_STEREO]) {
    if (mixer->paused) {
        return;
    }
    for (unsigned index = 0; index <= DD2_MIXER_CHANNELS; ++index) {
        dd2_mixer_voice_storage *voice =
            index == DD2_MIXER_CHANNELS ? &mixer->music : &mixer->voices[index];
        if (!voice->state.playing) {
            continue;
        }
        for (unsigned side = 0; side < DD2_PCM_STEREO; ++side) {
            channels[side] += dd2_mixer_sample(voice, mixer, side);
        }
        dd2_mixer_advance(voice, mixer->rate);
    }
}
bool dd2_mixer_render(dd2_mixer *mixer, int16_t *output, size_t frames) {
    if (mixer == NULL || (frames != 0 && output == NULL) ||
        frames > SIZE_MAX / (DD2_PCM_STEREO * sizeof(*output))) {
        return false;
    }
    for (size_t frame = 0; frame < frames; ++frame) {
        int64_t channels[DD2_PCM_STEREO] = {0};
        dd2_mixer_channels(mixer, channels);
        for (unsigned side = 0; side < DD2_PCM_STEREO; ++side) {
            output[(frame * DD2_PCM_STEREO) + side] = dd2_mixer_saturate(channels[side]);
        }
    }
    mixer->transport.frame = mixer->music.state.frame;
    mixer->transport.fraction = mixer->music.state.fraction;
    if (mixer->transport.phase == DD2_MUSIC_PLAYING && !mixer->music.state.playing) {
        mixer->transport.phase = DD2_MUSIC_ENDED;
    }
    return true;
}
