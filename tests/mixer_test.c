#include "assets/audio.h"
#include "assets/bytes.h"
#include "audio/mixer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_MIXER_TEST_RATE = 48000,
    DD2_MIXER_TEST_HALF_RATE = 24000,
    DD2_MIXER_TEST_CDDA_RATE = 44100,
    DD2_MIXER_TEST_SAMPLE = 12000,
    DD2_MIXER_TEST_HALF_SAMPLE = 6000,
    DD2_MIXER_TEST_NEGATIVE_SAMPLE = -12000,
    DD2_MIXER_TEST_FRAMES = 97,
    DD2_MIXER_TEST_PART = 13,
    DD2_MIXER_TEST_BYTE_BITS = 8,
    DD2_MIXER_TEST_WORD_BITS = 16,
    DD2_MIXER_TEST_MONO_BYTES = 6,
    DD2_MIXER_TEST_STEREO_BYTES = 12
};

static const dd2_voice_config dd2_mixer_test_config = {.frequency = DD2_MIXER_TEST_RATE,
                                                       .gain = DD2_MIXER_GAIN_ONE};

static void dd2_mixer_test_word(uint8_t *bytes, int16_t sample) {
    const uint16_t word = (uint16_t)sample;
    bytes[0] = (uint8_t)word;
    bytes[1] = (uint8_t)(word >> DD2_MIXER_TEST_BYTE_BITS);
}

static dd2_pcm_view dd2_mixer_test_source(uint8_t bytes[DD2_MIXER_TEST_MONO_BYTES]) {
    dd2_mixer_test_word(bytes, 0);
    dd2_mixer_test_word(bytes + 2, DD2_MIXER_TEST_SAMPLE);
    dd2_mixer_test_word(bytes + 4, DD2_MIXER_TEST_NEGATIVE_SAMPLE);
    return (dd2_pcm_view){.samples = {bytes, DD2_MIXER_TEST_MONO_BYTES},
                          .frames = 3,
                          .sample_rate = DD2_MIXER_TEST_RATE,
                          .channels = 1,
                          .bits_per_sample = DD2_MIXER_TEST_WORD_BITS};
}

static bool dd2_mixer_test_resampling(void) {
    uint8_t bytes[DD2_MIXER_TEST_MONO_BYTES];
    const dd2_pcm_view pcm = dd2_mixer_test_source(bytes);
    dd2_mixer *mixer = dd2_mixer_create(DD2_MIXER_TEST_RATE);
    dd2_voice_config config = dd2_mixer_test_config;
    config.frequency = DD2_MIXER_TEST_HALF_RATE;
    const int16_t expected[] = {0, DD2_MIXER_TEST_HALF_SAMPLE,     DD2_MIXER_TEST_SAMPLE,
                                0, DD2_MIXER_TEST_NEGATIVE_SAMPLE, DD2_MIXER_TEST_NEGATIVE_SAMPLE,
                                0};
    int16_t output[sizeof(expected) / sizeof(expected[0]) * DD2_PCM_STEREO] = {0};
    bool valid = mixer != NULL && dd2_mixer_play(mixer, 0, &pcm, config) == 0 &&
                 dd2_mixer_render(mixer, output, sizeof(expected) / sizeof(expected[0]));
    for (size_t frame = 0; valid && frame < sizeof(expected) / sizeof(expected[0]); ++frame) {
        valid = output[frame * DD2_PCM_STEREO] == expected[frame] &&
                output[(frame * DD2_PCM_STEREO) + 1] == expected[frame];
    }
    dd2_voice_state state = {0};
    valid = valid && dd2_mixer_voice(mixer, 0, &state) && !state.playing && state.frame == 3 &&
            state.fraction == 0;
    dd2_mixer_destroy(mixer);
    return valid;
}

static bool dd2_mixer_test_partition(void) {
    uint8_t bytes[DD2_MIXER_TEST_MONO_BYTES];
    const dd2_pcm_view pcm = dd2_mixer_test_source(bytes);
    dd2_voice_config config = dd2_mixer_test_config;
    config.frequency = DD2_MIXER_TEST_CDDA_RATE;
    config.loop = true;
    dd2_mixer *whole = dd2_mixer_create(DD2_MIXER_TEST_RATE);
    dd2_mixer *parts = dd2_mixer_create(DD2_MIXER_TEST_RATE);
    int16_t first[DD2_MIXER_TEST_FRAMES * DD2_PCM_STEREO] = {0};
    int16_t second[DD2_MIXER_TEST_FRAMES * DD2_PCM_STEREO] = {0};
    bool valid = whole != NULL && parts != NULL && dd2_mixer_play(whole, 0, &pcm, config) == 0 &&
                 dd2_mixer_play(parts, 0, &pcm, config) == 0 &&
                 dd2_mixer_render(whole, first, DD2_MIXER_TEST_FRAMES);
    size_t offset = 0;
    while (valid && offset < DD2_MIXER_TEST_FRAMES) {
        const size_t remaining = DD2_MIXER_TEST_FRAMES - offset;
        const size_t count = remaining < DD2_MIXER_TEST_PART ? remaining : DD2_MIXER_TEST_PART;
        valid = dd2_mixer_render(parts, second + (offset * DD2_PCM_STEREO), count);
        offset += count;
    }
    for (unsigned index = 0; valid && index < DD2_MIXER_TEST_FRAMES * DD2_PCM_STEREO; ++index) {
        valid = first[index] == second[index];
    }
    dd2_voice_state state = {0};
    const uint64_t phase = (uint64_t)DD2_MIXER_TEST_FRAMES * DD2_MIXER_TEST_CDDA_RATE;
    valid = valid && dd2_mixer_voice(parts, 0, &state) && state.playing &&
            state.frame == (phase / DD2_MIXER_TEST_RATE) % pcm.frames &&
            state.fraction == phase % DD2_MIXER_TEST_RATE;
    int16_t silence[2] = {1, 1};
    valid = valid && dd2_mixer_set_paused(parts, true) && dd2_mixer_render(parts, silence, 1) &&
            silence[0] == 0 && silence[1] == 0;
    dd2_voice_state paused = {0};
    valid = valid && dd2_mixer_voice(parts, 0, &paused) && paused.frame == state.frame &&
            paused.fraction == state.fraction && dd2_mixer_set_paused(parts, false);
    dd2_mixer_destroy(whole);
    dd2_mixer_destroy(parts);
    return valid;
}

static bool dd2_mixer_test_channels(void) {
    uint8_t bytes[DD2_MIXER_TEST_MONO_BYTES];
    const dd2_pcm_view pcm = dd2_mixer_test_source(bytes);
    dd2_mixer *mixer = dd2_mixer_create(DD2_MIXER_TEST_RATE);
    bool valid = mixer != NULL;
    for (unsigned channel = 0; valid && channel < DD2_MIXER_CHANNELS; ++channel) {
        valid = dd2_mixer_play(mixer, DD2_MIXER_AUTO_CHANNEL, &pcm, dd2_mixer_test_config) ==
                    (int)channel &&
                dd2_mixer_lock(mixer, channel, true);
    }
    valid = valid &&
            dd2_mixer_play(mixer, DD2_MIXER_AUTO_CHANNEL, &pcm, dd2_mixer_test_config) == -1 &&
            dd2_mixer_play(mixer, 0, &pcm, dd2_mixer_test_config) == 0 && dd2_mixer_stop(mixer, 0);
    dd2_voice_state state = {0};
    valid = valid && dd2_mixer_voice(mixer, 0, &state) && state.locked && !state.playing &&
            dd2_mixer_lock(mixer, 2, false) &&
            dd2_mixer_play(mixer, DD2_MIXER_AUTO_CHANNEL, &pcm, dd2_mixer_test_config) == 2;
    dd2_voice_config invalid = dd2_mixer_test_config;
    invalid.frequency = 0;
    valid = valid && !dd2_mixer_modify(mixer, 2, invalid) && dd2_mixer_voice(mixer, 2, &state) &&
            state.config.frequency == DD2_MIXER_TEST_RATE;
    dd2_mixer_destroy(mixer);
    return valid;
}

static bool dd2_mixer_test_gain(void) {
    uint8_t bytes[DD2_MIXER_TEST_MONO_BYTES];
    const dd2_pcm_view pcm = dd2_mixer_test_source(bytes);
    dd2_mixer *mixer = dd2_mixer_create(DD2_MIXER_TEST_RATE);
    dd2_voice_config config = dd2_mixer_test_config;
    config.pan = DD2_MIXER_PAN_LEFT;
    config.gain = DD2_MIXER_GAIN_ONE / 2;
    int16_t output[(size_t)3 * DD2_PCM_STEREO] = {0};
    bool valid = mixer != NULL && dd2_mixer_play(mixer, 0, &pcm, config) == 0 &&
                 dd2_mixer_render(mixer, output, 3) && output[2] == DD2_MIXER_TEST_HALF_SAMPLE &&
                 output[3] == 0 && output[4] == -DD2_MIXER_TEST_HALF_SAMPLE &&
                 output[(2 * DD2_PCM_STEREO) + 1] == 0;
    config = dd2_mixer_test_config;
    for (unsigned channel = 0; valid && channel < DD2_MIXER_CHANNELS; ++channel) {
        valid = dd2_mixer_play(mixer, channel, &pcm, config) == (int)channel;
    }
    valid = valid && dd2_mixer_render(mixer, output, 3) && output[2] == INT16_MAX &&
            output[3] == INT16_MAX && output[4] == INT16_MIN &&
            output[(2 * DD2_PCM_STEREO) + 1] == INT16_MIN;
    dd2_mixer_destroy(mixer);
    return valid;
}

static bool dd2_mixer_test_music(void) {
    uint8_t bytes[DD2_MIXER_TEST_STEREO_BYTES] = {0};
    for (size_t frame = 0; frame < 3; ++frame) {
        dd2_mixer_test_word(bytes + (frame * 4), DD2_MIXER_TEST_SAMPLE);
        dd2_mixer_test_word(bytes + (frame * 4) + 2, DD2_MIXER_TEST_NEGATIVE_SAMPLE);
    }
    dd2_pcm_view pcm = {0};
    dd2_mixer *mixer = dd2_mixer_create(DD2_MIXER_TEST_CDDA_RATE);
    int16_t output[4 * DD2_PCM_STEREO] = {0};
    dd2_music_state state = {0};
    bool valid =
        mixer != NULL && dd2_cdda_decode((dd2_byte_view){bytes, sizeof(bytes)}, &pcm) &&
        dd2_mixer_music_select(mixer, DD2_MIXER_FIRST_TRACK, &pcm, false) &&
        dd2_mixer_music(mixer, &state) && state.phase == DD2_MUSIC_READY &&
        dd2_mixer_render(mixer, output, 1) && output[0] == 0 && dd2_mixer_music_start(mixer) &&
        dd2_mixer_render(mixer, output, 1) && output[0] == DD2_MIXER_TEST_SAMPLE &&
        output[1] == DD2_MIXER_TEST_NEGATIVE_SAMPLE && dd2_mixer_music_pause(mixer) &&
        dd2_mixer_render(mixer, output, 1) && output[0] == 0 && dd2_mixer_music(mixer, &state) &&
        state.phase == DD2_MUSIC_PAUSED && state.frame == 1 && dd2_mixer_music_resume(mixer) &&
        dd2_mixer_render(mixer, output, 3) && output[4] == 0 &&
        output[(2 * DD2_PCM_STEREO) + 1] == 0 && dd2_mixer_music(mixer, &state) &&
        state.phase == DD2_MUSIC_ENDED && state.frame == 3;
    valid = valid && dd2_mixer_music_select(mixer, DD2_MIXER_LAST_TRACK, &pcm, true) &&
            dd2_mixer_music_start(mixer) && dd2_mixer_render(mixer, output, 4) &&
            dd2_mixer_music(mixer, &state) && state.phase == DD2_MUSIC_PLAYING &&
            state.frame == 1 && output[(size_t)3 * DD2_PCM_STEREO] == DD2_MIXER_TEST_SAMPLE &&
            output[(3 * DD2_PCM_STEREO) + 1] == DD2_MIXER_TEST_NEGATIVE_SAMPLE &&
            !dd2_mixer_music_select(mixer, 1, &pcm, false) && dd2_mixer_music(mixer, &state) &&
            state.track == DD2_MIXER_LAST_TRACK && dd2_mixer_music_stop(mixer) &&
            !dd2_mixer_music_start(mixer);
    dd2_mixer_destroy(mixer);
    return valid;
}

static bool dd2_mixer_test_extremes(void) {
    const uint8_t sample = UINT8_MAX;
    const dd2_pcm_view pcm = {.samples = {&sample, 1},
                              .frames = 1,
                              .sample_rate = DD2_MIXER_RATE_MIN,
                              .channels = 1,
                              .bits_per_sample = DD2_MIXER_TEST_BYTE_BITS};
    dd2_mixer *mixer = dd2_mixer_create(DD2_MIXER_RATE_MIN);
    dd2_voice_config config = {.frequency = DD2_MIXER_FREQUENCY_MAX,
                               .gain = DD2_MIXER_GAIN_ONE,
                               .pan = DD2_MIXER_PAN_RIGHT,
                               .loop = true};
    int16_t output[DD2_MIXER_TEST_PART * DD2_PCM_STEREO] = {0};
    bool valid = mixer != NULL && dd2_mixer_play(mixer, 0, &pcm, config) == 0 &&
                 dd2_mixer_render(mixer, output, DD2_MIXER_TEST_PART);
    const int16_t expected = (int16_t)((INT16_MAX / DD2_MIXER_GAIN_ONE) * DD2_MIXER_GAIN_ONE);
    for (size_t frame = 0; valid && frame < DD2_MIXER_TEST_PART; ++frame) {
        valid =
            output[frame * DD2_PCM_STEREO] == 0 && output[(frame * DD2_PCM_STEREO) + 1] == expected;
    }
    dd2_voice_state state = {0};
    valid = valid && dd2_mixer_voice(mixer, 0, &state) && state.playing && state.frame == 0 &&
            state.fraction == 0;
    config.gain = 0;
    valid = valid && dd2_mixer_modify(mixer, 0, config) && dd2_mixer_render(mixer, output, 1) &&
            output[0] == 0 && output[1] == 0;
    dd2_mixer_destroy(mixer);
    return valid;
}

static bool dd2_mixer_test_rejection(void) {
    dd2_mixer *mixer = dd2_mixer_create(DD2_MIXER_TEST_RATE);
    int16_t output[2] = {1, 1};
    const bool valid =
        mixer != NULL && dd2_mixer_create(0) == NULL && dd2_mixer_create(UINT32_MAX) == NULL &&
        !dd2_mixer_render(mixer, output, SIZE_MAX) && output[0] == 1 &&
        !dd2_mixer_render(mixer, NULL, 1) && dd2_mixer_render(mixer, NULL, 0) &&
        dd2_mixer_play(mixer, 0, NULL, dd2_mixer_test_config) == -1 &&
        !dd2_mixer_stop(mixer, DD2_MIXER_CHANNELS) && !dd2_mixer_music_pause(mixer) &&
        !dd2_mixer_music_resume(mixer) && !dd2_mixer_music_gain(mixer, DD2_MIXER_GAIN_ONE + 1);
    dd2_mixer_destroy(mixer);
    dd2_mixer_destroy(NULL);
    return valid;
}

int main(void) {
    const bool valid = dd2_mixer_test_resampling() && dd2_mixer_test_partition() &&
                       dd2_mixer_test_channels() && dd2_mixer_test_gain() &&
                       dd2_mixer_test_music() && dd2_mixer_test_extremes() &&
                       dd2_mixer_test_rejection();
    printf("Stereo mixer/transport: %s\n", valid ? "PASS" : "FAIL");
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
