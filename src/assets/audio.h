#ifndef DD2_ASSETS_AUDIO_H
#define DD2_ASSETS_AUDIO_H

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { DD2_SOUND_BANK_LIMIT = 64, DD2_PCM_STEREO = 2 };

typedef struct {
    dd2_byte_view samples;
    size_t frames;
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits_per_sample;
} dd2_pcm_view;

typedef struct {
    dd2_pcm_view pcm;
    uint32_t frequency;     /* Playback frequency from the bank, separate from WAVE rate. */
    uint32_t channel_flags; /* Retained original metadata; playback owns its interpretation. */
    bool loop;
} dd2_sound;

typedef struct dd2_sound_bank dd2_sound_bank;

/* Decode one complete RIFF/WAVE PCM file (8-bit unsigned or 16-bit signed LE,
 * mono/stereo). Unknown chunks and their even-byte padding are skipped safely.
 * CDDA accepts provisioned little-endian stereo 16-bit 44,100 Hz sample bytes.
 * All sample views borrow immutable input. Failure clears the entire output. */
bool dd2_wave_decode(dd2_byte_view bytes, dd2_pcm_view *result);
bool dd2_cdda_decode(dd2_byte_view bytes, dd2_pcm_view *result);
bool dd2_pcm_sample(const dd2_pcm_view *pcm, size_t frame, unsigned channel, int16_t *result);

/* Owns only decoded metadata, borrowing the complete immutable SBK input for
 * its lifetime. The 16-byte header and 28-byte sound records are never rewritten
 * with handles or pointers. Each sound has a bounded complete RIFF/WAVE extent.
 * Get returns a borrowed sound valid until bank destruction; invalid index is NULL. */
dd2_sound_bank *dd2_sound_bank_create(dd2_byte_view bytes);
void dd2_sound_bank_destroy(dd2_sound_bank *bank);
unsigned dd2_sound_bank_count(const dd2_sound_bank *bank);
const dd2_sound *dd2_sound_bank_get(const dd2_sound_bank *bank, unsigned index);

#endif
