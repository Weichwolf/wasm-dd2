#ifndef DD2_PLATFORM_AUDIO_DEVICE_H
#define DD2_PLATFORM_AUDIO_DEVICE_H

#include "audio/mixer.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct dd2_audio_device dd2_audio_device;

/* Owns an SDL stereo device and its mixer. Opens paused; the negotiated rate
 * drives resampling. Close waits for the callback before destroying the mixer.
 * Keep borrowed PCM alive until replacement/stop under the device lock or close.
 * Control calls are main-thread only. Acquire/release must be paired; do not
 * close or pause the device while holding its lock. */
dd2_audio_device *dd2_audio_device_create(void);
void dd2_audio_device_destroy(dd2_audio_device *device);
uint32_t dd2_audio_device_rate(const dd2_audio_device *device);
void dd2_audio_device_set_paused(dd2_audio_device *device, bool paused);
dd2_mixer *dd2_audio_device_acquire(dd2_audio_device *device);
void dd2_audio_device_release(dd2_audio_device *device);

#endif
