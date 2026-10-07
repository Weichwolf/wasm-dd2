#include "assets/audio.h"

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_WAVE_HEADER = 12,
    DD2_WAVE_CHUNK_HEADER = 8,
    DD2_WAVE_FORMAT_BYTES = 16,
    DD2_WAVE_PCM = 1,
    DD2_WAVE_FORM_OFFSET = 8,
    DD2_PCM_BYTE_BITS = 8,
    DD2_PCM_WORD_BITS = 16,
    DD2_PCM_BYTE_CENTER = 128,
    DD2_PCM_BYTE_SCALE = 256,
    DD2_CDDA_RATE = 44100,
    DD2_BANK_HEADER = 16,
    DD2_BANK_RECORD = 28,
    DD2_BANK_SIZE_OFFSET = 8,
    DD2_BANK_COUNT_OFFSET = 12,
    DD2_BANK_WAVE_SIZE_OFFSET = 4,
    DD2_BANK_LOOP_OFFSET = 8,
    DD2_BANK_FREQUENCY_OFFSET = 12,
    DD2_BANK_FLAGS_OFFSET = 16
};

struct dd2_sound_bank {
    dd2_sound sounds[DD2_SOUND_BANK_LIMIT];
    unsigned count;
};

static bool dd2_pcm_valid(const dd2_pcm_view *pcm) {
    if (pcm == NULL || pcm->sample_rate == 0 || pcm->channels == 0 ||
        pcm->channels > DD2_PCM_STEREO ||
        (pcm->bits_per_sample != DD2_PCM_BYTE_BITS && pcm->bits_per_sample != DD2_PCM_WORD_BITS) ||
        (pcm->samples.data == NULL && pcm->samples.size != 0)) {
        return false;
    }
    const size_t alignment = (size_t)pcm->channels * (pcm->bits_per_sample / DD2_PCM_BYTE_BITS);
    return pcm->samples.size % alignment == 0 && pcm->frames == pcm->samples.size / alignment;
}

static bool dd2_wave_format(dd2_byte_view bytes, dd2_pcm_view *pcm) {
    if (bytes.size < DD2_WAVE_FORMAT_BYTES || dd2_read_le16(bytes.data) != DD2_WAVE_PCM) {
        return false;
    }
    const uint16_t channels = dd2_read_le16(bytes.data + 2);
    const uint32_t rate = dd2_read_le32(bytes.data + 4);
    const uint32_t byte_rate = dd2_read_le32(bytes.data + 8);
    const uint16_t alignment = dd2_read_le16(bytes.data + 12);
    const uint16_t bits = dd2_read_le16(bytes.data + 14);
    if (channels == 0 || channels > DD2_PCM_STEREO || rate == 0 ||
        (bits != DD2_PCM_BYTE_BITS && bits != DD2_PCM_WORD_BITS) ||
        alignment != channels * (bits / DD2_PCM_BYTE_BITS) ||
        (uint64_t)rate * alignment != byte_rate) {
        return false;
    }
    pcm->channels = channels;
    pcm->sample_rate = rate;
    pcm->bits_per_sample = bits;
    return true;
}

bool dd2_wave_decode(dd2_byte_view bytes, dd2_pcm_view *result) {
    if (result == NULL) {
        return false;
    }
    *result = (dd2_pcm_view){0};
    if (bytes.data == NULL || bytes.size < DD2_WAVE_HEADER || memcmp(bytes.data, "RIFF", 4) != 0 ||
        memcmp(bytes.data + DD2_WAVE_FORM_OFFSET, "WAVE", 4) != 0 ||
        dd2_read_le32(bytes.data + 4) != bytes.size - DD2_WAVE_CHUNK_HEADER) {
        return false;
    }
    dd2_pcm_view decoded = {0};
    bool format = false;
    bool samples = false;
    size_t offset = DD2_WAVE_HEADER;
    while (offset < bytes.size) {
        if (bytes.size - offset < DD2_WAVE_CHUNK_HEADER) {
            return false;
        }
        const uint8_t *header = bytes.data + offset;
        const size_t length = dd2_read_le32(header + 4);
        offset += DD2_WAVE_CHUNK_HEADER;
        const size_t remaining = bytes.size - offset;
        const size_t padding = length % 2;
        if (length > remaining || padding > remaining - length) {
            return false;
        }
        const dd2_byte_view chunk = {.data = bytes.data + offset, .size = length};
        if (memcmp(header, "fmt ", 4) == 0) {
            if (format || !dd2_wave_format(chunk, &decoded)) {
                return false;
            }
            format = true;
        } else if (memcmp(header, "data", 4) == 0) {
            if (samples) {
                return false;
            }
            decoded.samples = chunk;
            samples = true;
        }
        offset += length + padding;
    }
    if (!format || !samples) {
        return false;
    }
    const size_t alignment =
        (size_t)decoded.channels * (decoded.bits_per_sample / DD2_PCM_BYTE_BITS);
    decoded.frames = decoded.samples.size / alignment;
    if (!dd2_pcm_valid(&decoded)) {
        return false;
    }
    *result = decoded;
    return true;
}

bool dd2_cdda_decode(dd2_byte_view bytes, dd2_pcm_view *result) {
    if (result == NULL) {
        return false;
    }
    *result = (dd2_pcm_view){0};
    const dd2_pcm_view decoded = {.samples = bytes,
                                  .frames = bytes.size / (DD2_PCM_STEREO * sizeof(int16_t)),
                                  .sample_rate = DD2_CDDA_RATE,
                                  .channels = DD2_PCM_STEREO,
                                  .bits_per_sample = DD2_PCM_WORD_BITS};
    if (!dd2_pcm_valid(&decoded)) {
        return false;
    }
    *result = decoded;
    return true;
}

bool dd2_pcm_sample(const dd2_pcm_view *pcm, size_t frame, unsigned channel, int16_t *result) {
    if (result == NULL) {
        return false;
    }
    *result = 0;
    if (!dd2_pcm_valid(pcm) || frame >= pcm->frames || channel >= pcm->channels) {
        return false;
    }
    const size_t sample_bytes = pcm->bits_per_sample / DD2_PCM_BYTE_BITS;
    const size_t offset = ((frame * pcm->channels) + channel) * sample_bytes;
    if (pcm->bits_per_sample == DD2_PCM_WORD_BITS) {
        *result = dd2_read_le_i16(pcm->samples.data + offset);
    } else {
        *result =
            (int16_t)(((int)pcm->samples.data[offset] - DD2_PCM_BYTE_CENTER) * DD2_PCM_BYTE_SCALE);
    }
    return true;
}

dd2_sound_bank *dd2_sound_bank_create(dd2_byte_view bytes) {
    if (bytes.data == NULL || bytes.size < DD2_BANK_HEADER ||
        dd2_read_le32(bytes.data + DD2_BANK_SIZE_OFFSET) != bytes.size) {
        return NULL;
    }
    const uint32_t count = dd2_read_le32(bytes.data + DD2_BANK_COUNT_OFFSET);
    if (count > DD2_SOUND_BANK_LIMIT) {
        return NULL;
    }
    const size_t table_end = DD2_BANK_HEADER + ((size_t)count * DD2_BANK_RECORD);
    if (table_end > bytes.size) {
        return NULL;
    }
    dd2_sound_bank *bank = calloc(1, sizeof(*bank));
    if (bank == NULL) {
        return NULL;
    }
    bank->count = count;
    for (unsigned index = 0; index < bank->count; ++index) {
        const uint8_t *record = bytes.data + DD2_BANK_HEADER + ((size_t)index * DD2_BANK_RECORD);
        const size_t offset = dd2_read_le32(record);
        const size_t size = dd2_read_le32(record + DD2_BANK_WAVE_SIZE_OFFSET);
        dd2_sound *sound = &bank->sounds[index];
        sound->frequency = dd2_read_le32(record + DD2_BANK_FREQUENCY_OFFSET);
        sound->loop = dd2_read_le32(record + DD2_BANK_LOOP_OFFSET) != 0;
        sound->channel_flags = dd2_read_le32(record + DD2_BANK_FLAGS_OFFSET);
        if (offset < table_end || offset > bytes.size || size > bytes.size - offset ||
            sound->frequency == 0 ||
            !dd2_wave_decode((dd2_byte_view){.data = bytes.data + offset, .size = size},
                             &sound->pcm)) {
            dd2_sound_bank_destroy(bank);
            return NULL;
        }
    }
    return bank;
}

void dd2_sound_bank_destroy(dd2_sound_bank *bank) {
    free(bank);
}

unsigned dd2_sound_bank_count(const dd2_sound_bank *bank) {
    return bank == NULL ? 0 : bank->count;
}

const dd2_sound *dd2_sound_bank_get(const dd2_sound_bank *bank, unsigned index) {
    return bank != NULL && index < bank->count ? &bank->sounds[index] : NULL;
}
