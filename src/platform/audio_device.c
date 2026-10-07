#include "platform/audio_device.h"

#include "assets/audio.h"
#include "audio/mixer.h"

#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_stdinc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum { DD2_DEVICE_RATE = 48000, DD2_DEVICE_FRAMES = 1024 };

struct dd2_audio_device {
    SDL_AudioDeviceID native;
    dd2_mixer *mixer;
    uint32_t rate;
    bool initialized;
};

static void dd2_audio_device_callback(void *context, Uint8 *stream, int length) {
    dd2_audio_device *device = context;
    if (length <= 0) {
        return;
    }
    for (size_t index = 0; index < (size_t)length; ++index) {
        stream[index] = 0;
    }
    const size_t frames = (size_t)length / (sizeof(int16_t) * DD2_PCM_STEREO);
    dd2_mixer_render(device->mixer, (int16_t *)stream, frames);
}

void dd2_audio_device_destroy(dd2_audio_device *device) {
    if (device != NULL) {
        if (device->native != 0) {
            SDL_CloseAudioDevice(device->native);
        }
        dd2_mixer_destroy(device->mixer);
        if (device->initialized) {
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
        }
        free(device);
    }
}

dd2_audio_device *dd2_audio_device_create(void) {
    dd2_audio_device *device = calloc(1, sizeof(*device));
    if (device == NULL) {
        return NULL;
    }
    device->initialized = SDL_InitSubSystem(SDL_INIT_AUDIO) == 0;
    if (!device->initialized) {
        dd2_audio_device_destroy(device);
        return NULL;
    }
    const SDL_AudioSpec desired = {.freq = DD2_DEVICE_RATE,
                                   .format = AUDIO_S16SYS,
                                   .channels = DD2_PCM_STEREO,
                                   .samples = DD2_DEVICE_FRAMES,
                                   .callback = dd2_audio_device_callback,
                                   .userdata = device};
    SDL_AudioSpec obtained = {0};
    device->native =
        SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (device->native != 0 && obtained.freq > 0 && obtained.format == AUDIO_S16SYS &&
        obtained.channels == DD2_PCM_STEREO) {
        device->rate = (uint32_t)obtained.freq;
        device->mixer = dd2_mixer_create(device->rate);
    }
    if (device->mixer == NULL) {
        dd2_audio_device_destroy(device);
        return NULL;
    }
    return device;
}

uint32_t dd2_audio_device_rate(const dd2_audio_device *device) {
    return device != NULL ? device->rate : 0;
}

void dd2_audio_device_set_paused(dd2_audio_device *device, bool paused) {
    if (device != NULL) {
        SDL_PauseAudioDevice(device->native, (int)paused);
    }
}

dd2_mixer *dd2_audio_device_acquire(dd2_audio_device *device) {
    if (device == NULL) {
        return NULL;
    }
    SDL_LockAudioDevice(device->native);
    return device->mixer;
}

void dd2_audio_device_release(dd2_audio_device *device) {
    if (device != NULL) {
        SDL_UnlockAudioDevice(device->native);
    }
}
