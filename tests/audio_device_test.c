#include "assets/audio.h"
#include "assets/bytes.h"
#include "audio/mixer.h"
#include "platform/audio_device.h"

#include <SDL_timer.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_DEVICE_TEST_FRAMES = 97,
    DD2_DEVICE_TEST_FRAME_BYTES = 4,
    DD2_DEVICE_TEST_WAIT_MS = 20,
    DD2_DEVICE_TEST_PAUSE_MS = 120,
    DD2_DEVICE_TEST_TIMEOUT_MS = 2000,
    DD2_DEVICE_TEST_LEFT_LOW = 224,
    DD2_DEVICE_TEST_LEFT_HIGH = 46,
    DD2_DEVICE_TEST_RIGHT_LOW = 144,
    DD2_DEVICE_TEST_RIGHT_HIGH = 232
};

static dd2_music_state dd2_device_test_state(dd2_audio_device *device) {
    dd2_music_state state = {0};
    dd2_mixer *mixer = dd2_audio_device_acquire(device);
    dd2_mixer_music(mixer, &state);
    dd2_audio_device_release(device);
    return state;
}

static bool dd2_device_test_same_cursor(dd2_music_state first, dd2_music_state second) {
    return first.frame == second.frame && first.fraction == second.fraction;
}

static bool dd2_device_test_advance(dd2_audio_device *device, dd2_music_state before) {
    const uint64_t deadline = SDL_GetTicks64() + DD2_DEVICE_TEST_TIMEOUT_MS;
    while (SDL_GetTicks64() < deadline) {
        SDL_Delay(DD2_DEVICE_TEST_WAIT_MS);
        if (!dd2_device_test_same_cursor(before, dd2_device_test_state(device))) {
            return true;
        }
    }
    return false;
}

int main(void) {
    uint8_t bytes[DD2_DEVICE_TEST_FRAMES * DD2_DEVICE_TEST_FRAME_BYTES];
    for (size_t frame = 0; frame < DD2_DEVICE_TEST_FRAMES; ++frame) {
        bytes[frame * DD2_DEVICE_TEST_FRAME_BYTES] = DD2_DEVICE_TEST_LEFT_LOW;
        bytes[(frame * DD2_DEVICE_TEST_FRAME_BYTES) + 1] = DD2_DEVICE_TEST_LEFT_HIGH;
        bytes[(frame * DD2_DEVICE_TEST_FRAME_BYTES) + 2] = DD2_DEVICE_TEST_RIGHT_LOW;
        bytes[(frame * DD2_DEVICE_TEST_FRAME_BYTES) + 3] = DD2_DEVICE_TEST_RIGHT_HIGH;
    }
    dd2_pcm_view pcm = {0};
    dd2_audio_device *device = dd2_audio_device_create();
    const uint32_t rate = dd2_audio_device_rate(device);
    dd2_mixer *mixer = dd2_audio_device_acquire(device);
    bool valid = device != NULL && rate >= DD2_MIXER_RATE_MIN && rate <= DD2_MIXER_RATE_MAX &&
                 dd2_cdda_decode((dd2_byte_view){bytes, sizeof(bytes)}, &pcm) &&
                 dd2_mixer_music_select(mixer, DD2_MIXER_FIRST_TRACK, &pcm, true) &&
                 dd2_mixer_music_start(mixer);
    dd2_audio_device_release(device);
    SDL_Delay(DD2_DEVICE_TEST_PAUSE_MS);
    dd2_music_state state = dd2_device_test_state(device);
    valid = valid && state.frame == 0 && state.fraction == 0;
    dd2_audio_device_set_paused(device, false);
    valid = valid && dd2_device_test_advance(device, state);
    dd2_audio_device_set_paused(device, true);
    state = dd2_device_test_state(device);
    SDL_Delay(DD2_DEVICE_TEST_PAUSE_MS);
    valid = valid && dd2_device_test_same_cursor(state, dd2_device_test_state(device));
    mixer = dd2_audio_device_acquire(device);
    valid = valid && dd2_mixer_music_pause(mixer);
    dd2_audio_device_release(device);
    dd2_audio_device_set_paused(device, false);
    SDL_Delay(DD2_DEVICE_TEST_PAUSE_MS);
    valid = valid && dd2_device_test_same_cursor(state, dd2_device_test_state(device));
    mixer = dd2_audio_device_acquire(device);
    valid = valid && dd2_mixer_music_resume(mixer);
    dd2_audio_device_release(device);
    valid = valid && dd2_device_test_advance(device, state);
    /* Closing an active device must join the callback before these stack PCM
     * bytes cease to exist. Reopening exercises balanced SDL subsystem lifetime. */
    dd2_audio_device_destroy(device);
    device = dd2_audio_device_create();
    valid = valid && device != NULL;
    dd2_audio_device_destroy(device);
    dd2_audio_device_destroy(NULL);
    dd2_audio_device_set_paused(NULL, true);
    dd2_audio_device_release(NULL);
    valid = valid && dd2_audio_device_acquire(NULL) == NULL && dd2_audio_device_rate(NULL) == 0;
    printf("SDL stereo device/callback: %s (rate %u)\n", valid ? "PASS" : "FAIL", rate);
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
