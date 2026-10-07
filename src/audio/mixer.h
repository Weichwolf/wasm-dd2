#ifndef DD2_AUDIO_MIXER_H
#define DD2_AUDIO_MIXER_H

#include "assets/audio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    DD2_MIXER_CHANNELS = 4,
    DD2_MIXER_AUTO_CHANNEL = DD2_MIXER_CHANNELS,
    DD2_MIXER_GAIN_ONE = 256,
    DD2_MIXER_PAN_LEFT = -256,
    DD2_MIXER_PAN_RIGHT = 256,
    DD2_MIXER_RATE_MIN = 8000,
    DD2_MIXER_RATE_MAX = 192000,
    DD2_MIXER_FREQUENCY_MAX = 384000,
    DD2_MIXER_FIRST_TRACK = 2,
    DD2_MIXER_LAST_TRACK = 19
};

typedef struct dd2_mixer dd2_mixer;

typedef struct {
    uint32_t frequency; /* Actual source-frame frequency in Hz, independent of WAVE metadata. */
    unsigned gain;      /* Linear amplitude, 0..DD2_MIXER_GAIN_ONE. */
    int pan;            /* Linear attenuation of the opposite side, -256..256. */
    bool loop;
} dd2_voice_config;

typedef struct {
    size_t frame;
    uint32_t fraction; /* Numerator with the mixer's output rate as denominator. */
    dd2_voice_config config;
    bool playing;
    bool locked; /* Locks automatic selection; explicit channel replacement remains allowed. */
} dd2_voice_state;

typedef enum {
    DD2_MUSIC_EMPTY,
    DD2_MUSIC_READY,
    DD2_MUSIC_PLAYING,
    DD2_MUSIC_PAUSED,
    DD2_MUSIC_ENDED
} dd2_music_phase;

typedef struct {
    dd2_music_phase phase;
    unsigned track;
    size_t frame;
    uint32_t fraction;
    unsigned gain;
    bool repeat;
} dd2_music_state;

/* Owns voice/transport state; immutable PCM bytes are borrowed until replacement,
 * stop or destruction. The caller must serialize render/control calls (including
 * device callbacks) and keep active/paused sources alive. No per-frame allocation.
 * Output is interleaved signed 16-bit stereo, with rational linear resampling and
 * saturating final summation. Creation validates the output rate. */
dd2_mixer *dd2_mixer_create(uint32_t output_rate);
void dd2_mixer_destroy(dd2_mixer *mixer);
bool dd2_mixer_render(dd2_mixer *mixer, int16_t *output, size_t frames);
bool dd2_mixer_set_paused(dd2_mixer *mixer, bool paused);

/* Automatic selection rotates over four unlocked channels and replaces the
 * chosen channel, matching the original policy. All locked returns -1, without
 * the original's unbounded loop. Invalid arguments leave mixer state unchanged. */
int dd2_mixer_play(dd2_mixer *mixer, unsigned channel, const dd2_pcm_view *pcm,
                   dd2_voice_config config);
bool dd2_mixer_modify(dd2_mixer *mixer, unsigned channel, dd2_voice_config config);
bool dd2_mixer_stop(dd2_mixer *mixer, unsigned channel);
bool dd2_mixer_lock(dd2_mixer *mixer, unsigned channel, bool locked);
bool dd2_mixer_voice(const dd2_mixer *mixer, unsigned channel, dd2_voice_state *result);

/* Select one complete provisioned CDDA track (physical disc numbers 2..19).
 * Start rewinds; pause/resume preserves the exact rational cursor; stop rewinds
 * and releases the PCM view. Repeat wraps without adding a silent frame. Music
 * has its own channel, independent of the four effect voices and their locks. */
bool dd2_mixer_music_select(dd2_mixer *mixer, unsigned track, const dd2_pcm_view *pcm, bool repeat);
bool dd2_mixer_music_start(dd2_mixer *mixer);
bool dd2_mixer_music_pause(dd2_mixer *mixer);
bool dd2_mixer_music_resume(dd2_mixer *mixer);
bool dd2_mixer_music_stop(dd2_mixer *mixer);
bool dd2_mixer_music_gain(dd2_mixer *mixer, unsigned gain);
bool dd2_mixer_music(const dd2_mixer *mixer, dd2_music_state *result);

#endif
