#include "asset_fixture.h"
#include "assets/audio.h"
#include "assets/bytes.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_AUDIO_TEST_CAPACITY = 256,
    DD2_AUDIO_TEST_FORMAT = 18,
    DD2_AUDIO_TEST_FORMAT_DATA = 20,
    DD2_AUDIO_TEST_FRAMES = 3,
    DD2_AUDIO_TEST_SAMPLES = 6,
    DD2_AUDIO_TEST_RATE = 22050,
    DD2_AUDIO_TEST_FREQUENCY = 5500,
    DD2_AUDIO_TEST_BYTE_BITS = 8,
    DD2_AUDIO_TEST_WORD_BITS = 16,
    DD2_AUDIO_TEST_RATE_OFFSET = 4,
    DD2_AUDIO_TEST_BYTE_RATE_OFFSET = 8,
    DD2_AUDIO_TEST_ALIGNMENT_OFFSET = 12,
    DD2_AUDIO_TEST_BITS_OFFSET = 14,
    DD2_AUDIO_TEST_RIFF_HEADER = 12,
    DD2_AUDIO_TEST_CHUNK_HEADER = 8,
    DD2_AUDIO_TEST_SAMPLE_OFFSET = 56,
    DD2_AUDIO_TEST_BANK_HEADER = 16,
    DD2_AUDIO_TEST_BANK_RECORD = 28,
    DD2_AUDIO_TEST_BANK_DATA = 72,
    DD2_AUDIO_TEST_BANK_SIZE = 8,
    DD2_AUDIO_TEST_BANK_COUNT = 12,
    DD2_AUDIO_TEST_BANK_FREQUENCY = 28,
    DD2_AUDIO_TEST_BANK_FLAGS = 32,
    DD2_AUDIO_TEST_UNSIGNED_MAX = 255,
    DD2_AUDIO_TEST_UNSIGNED_MID = 128,
    DD2_AUDIO_TEST_BYTE_MAX = 32512,
    DD2_AUDIO_TEST_WORD_VALUE = 12345
};

static size_t dd2_audio_test_chunk(uint8_t *bytes, size_t offset, const char *tag,
                                   dd2_byte_view payload) {
    for (unsigned index = 0; index < 4; ++index) {
        bytes[offset + index] = (uint8_t)tag[index];
    }
    dd2_test_write_le32(bytes + offset + 4, (uint32_t)payload.size);
    if (payload.size != 0) {
        for (size_t index = 0; index < payload.size; ++index) {
            bytes[offset + DD2_AUDIO_TEST_CHUNK_HEADER + index] = payload.data[index];
        }
    }
    return offset + DD2_AUDIO_TEST_CHUNK_HEADER + payload.size + (payload.size % 2);
}

static size_t dd2_audio_test_wave(uint8_t bytes[DD2_AUDIO_TEST_CAPACITY], unsigned channels,
                                  unsigned bits) {
    for (size_t index = 0; index < DD2_AUDIO_TEST_CAPACITY; ++index) {
        bytes[index] = 0;
    }
    uint8_t format[DD2_AUDIO_TEST_FORMAT] = {0};
    const unsigned alignment = channels * (bits / DD2_AUDIO_TEST_BYTE_BITS);
    dd2_test_write_le16(format, 1);
    dd2_test_write_le16(format + 2, (uint16_t)channels);
    dd2_test_write_le32(format + DD2_AUDIO_TEST_RATE_OFFSET, DD2_AUDIO_TEST_RATE);
    dd2_test_write_le32(format + DD2_AUDIO_TEST_BYTE_RATE_OFFSET, DD2_AUDIO_TEST_RATE * alignment);
    dd2_test_write_le16(format + DD2_AUDIO_TEST_ALIGNMENT_OFFSET, (uint16_t)alignment);
    dd2_test_write_le16(format + DD2_AUDIO_TEST_BITS_OFFSET, (uint16_t)bits);
    const uint8_t riff[] = {'R', 'I', 'F', 'F'};
    const uint8_t wave[] = {'W', 'A', 'V', 'E'};
    for (unsigned index = 0; index < 4; ++index) {
        bytes[index] = riff[index];
        bytes[DD2_AUDIO_TEST_CHUNK_HEADER + index] = wave[index];
    }
    size_t offset = dd2_audio_test_chunk(bytes, DD2_AUDIO_TEST_RIFF_HEADER, "fmt ",
                                         (dd2_byte_view){format, sizeof(format)});
    const uint8_t junk = DD2_AUDIO_TEST_UNSIGNED_MAX;
    offset = dd2_audio_test_chunk(bytes, offset, "JUNK", (dd2_byte_view){&junk, 1});
    uint8_t samples[DD2_AUDIO_TEST_SAMPLES * sizeof(int16_t)] = {0};
    const int16_t words[] = {
        INT16_MIN, INT16_MAX, -1, 0, DD2_AUDIO_TEST_WORD_VALUE, -DD2_AUDIO_TEST_WORD_VALUE};
    const uint8_t octets[] = {0, DD2_AUDIO_TEST_UNSIGNED_MID, DD2_AUDIO_TEST_UNSIGNED_MAX};
    for (unsigned index = 0; index < DD2_AUDIO_TEST_FRAMES * channels; ++index) {
        if (bits == DD2_AUDIO_TEST_WORD_BITS) {
            dd2_test_write_le16(samples + (index * sizeof(int16_t)), (uint16_t)words[index]);
        } else {
            samples[index] = octets[index % DD2_AUDIO_TEST_FRAMES];
        }
    }
    offset = dd2_audio_test_chunk(
        bytes, offset, "data", (dd2_byte_view){samples, (size_t)DD2_AUDIO_TEST_FRAMES * alignment});
    dd2_test_write_le32(bytes + 4, (uint32_t)(offset - DD2_AUDIO_TEST_CHUNK_HEADER));
    return offset;
}

static bool dd2_audio_test_samples(unsigned channels, unsigned bits) {
    uint8_t bytes[DD2_AUDIO_TEST_CAPACITY];
    const size_t size = dd2_audio_test_wave(bytes, channels, bits);
    dd2_pcm_view pcm = {0};
    if (!dd2_wave_decode((dd2_byte_view){bytes, size}, &pcm) ||
        pcm.frames != DD2_AUDIO_TEST_FRAMES || pcm.sample_rate != DD2_AUDIO_TEST_RATE ||
        pcm.channels != channels || pcm.bits_per_sample != bits) {
        return false;
    }
    const int16_t words[] = {
        INT16_MIN, INT16_MAX, -1, 0, DD2_AUDIO_TEST_WORD_VALUE, -DD2_AUDIO_TEST_WORD_VALUE};
    const int16_t octets[] = {INT16_MIN, 0, DD2_AUDIO_TEST_BYTE_MAX};
    for (unsigned frame = 0; frame < DD2_AUDIO_TEST_FRAMES; ++frame) {
        for (unsigned channel = 0; channel < channels; ++channel) {
            const unsigned index = (frame * channels) + channel;
            int16_t expected = octets[index % DD2_AUDIO_TEST_FRAMES];
            if (bits == DD2_AUDIO_TEST_WORD_BITS) {
                expected = words[index];
            }
            int16_t actual = 1;
            if (!dd2_pcm_sample(&pcm, frame, channel, &actual) || actual != expected) {
                return false;
            }
        }
    }
    int16_t sample = 1;
    if (dd2_pcm_sample(&pcm, pcm.frames, 0, &sample) || sample != 0 ||
        dd2_pcm_sample(&pcm, 0, pcm.channels, &sample)) {
        return false;
    }
    pcm.frames = SIZE_MAX;
    return !dd2_pcm_sample(&pcm, SIZE_MAX - 1, 0, &sample) && sample == 0;
}

static bool dd2_audio_test_rejection(void) {
    uint8_t bytes[DD2_AUDIO_TEST_CAPACITY];
    const size_t size = dd2_audio_test_wave(bytes, 1, DD2_AUDIO_TEST_BYTE_BITS);
    dd2_pcm_view pcm = {.frames = 1};
    for (size_t cut = 0; cut < size; ++cut) {
        if (cut >= DD2_AUDIO_TEST_RIFF_HEADER) {
            dd2_test_write_le32(bytes + 4, (uint32_t)(cut - DD2_AUDIO_TEST_CHUNK_HEADER));
        }
        if (dd2_wave_decode((dd2_byte_view){bytes, cut}, &pcm) || pcm.frames != 0 ||
            pcm.samples.data != NULL || pcm.sample_rate != 0) {
            return false;
        }
    }
    const unsigned offsets[] = {DD2_AUDIO_TEST_FORMAT_DATA,
                                DD2_AUDIO_TEST_FORMAT_DATA + 2,
                                DD2_AUDIO_TEST_FORMAT_DATA + DD2_AUDIO_TEST_RATE_OFFSET,
                                DD2_AUDIO_TEST_FORMAT_DATA + DD2_AUDIO_TEST_BYTE_RATE_OFFSET,
                                DD2_AUDIO_TEST_FORMAT_DATA + DD2_AUDIO_TEST_ALIGNMENT_OFFSET,
                                DD2_AUDIO_TEST_FORMAT_DATA + DD2_AUDIO_TEST_BITS_OFFSET};
    for (unsigned index = 0; index < sizeof(offsets) / sizeof(offsets[0]); ++index) {
        (void)dd2_audio_test_wave(bytes, 1, DD2_AUDIO_TEST_BYTE_BITS);
        dd2_test_write_le16(bytes + offsets[index], 0);
        if (dd2_wave_decode((dd2_byte_view){bytes, size}, &pcm)) {
            return false;
        }
    }
    (void)dd2_audio_test_wave(bytes, 1, DD2_AUDIO_TEST_BYTE_BITS);
    dd2_test_write_le32(bytes + DD2_AUDIO_TEST_FORMAT_DATA - 4, UINT32_MAX);
    return !dd2_wave_decode((dd2_byte_view){bytes, size}, &pcm) &&
           !dd2_wave_decode((dd2_byte_view){0}, &pcm) && !dd2_wave_decode((dd2_byte_view){0}, NULL);
}

static bool dd2_audio_test_chunks(void) {
    uint8_t source[DD2_AUDIO_TEST_CAPACITY];
    const size_t original_size = dd2_audio_test_wave(source, 1, DD2_AUDIO_TEST_BYTE_BITS);
    const dd2_byte_view format = {.data = source + DD2_AUDIO_TEST_FORMAT_DATA,
                                  .size = DD2_AUDIO_TEST_FORMAT};
    const dd2_byte_view samples = {.data = source + DD2_AUDIO_TEST_SAMPLE_OFFSET,
                                   .size = DD2_AUDIO_TEST_FRAMES};
    uint8_t bytes[DD2_AUDIO_TEST_CAPACITY] = {0};
    for (unsigned index = 0; index < DD2_AUDIO_TEST_RIFF_HEADER; ++index) {
        bytes[index] = source[index];
    }
    size_t size = dd2_audio_test_chunk(bytes, DD2_AUDIO_TEST_RIFF_HEADER, "data", samples);
    size = dd2_audio_test_chunk(bytes, size, "fmt ", format);
    dd2_test_write_le32(bytes + 4, (uint32_t)(size - DD2_AUDIO_TEST_CHUNK_HEADER));
    dd2_pcm_view pcm = {0};
    if (!dd2_wave_decode((dd2_byte_view){bytes, size}, &pcm) ||
        pcm.frames != DD2_AUDIO_TEST_FRAMES) {
        return false;
    }
    const size_t duplicate = dd2_audio_test_chunk(bytes, size, "fmt ", format);
    dd2_test_write_le32(bytes + 4, (uint32_t)(duplicate - DD2_AUDIO_TEST_CHUNK_HEADER));
    if (dd2_wave_decode((dd2_byte_view){bytes, duplicate}, &pcm) || pcm.samples.data != NULL) {
        return false;
    }
    const size_t duplicate_data = dd2_audio_test_chunk(bytes, size, "data", samples);
    dd2_test_write_le32(bytes + 4, (uint32_t)(duplicate_data - DD2_AUDIO_TEST_CHUNK_HEADER));
    if (dd2_wave_decode((dd2_byte_view){bytes, duplicate_data}, &pcm)) {
        return false;
    }
    /* The original odd data chunk needs its final padding byte. */
    dd2_test_write_le32(source + 4, (uint32_t)(original_size - 1 - DD2_AUDIO_TEST_CHUNK_HEADER));
    if (dd2_wave_decode((dd2_byte_view){source, original_size - 1}, &pcm)) {
        return false;
    }
    size = dd2_audio_test_chunk(bytes, DD2_AUDIO_TEST_RIFF_HEADER, "fmt ", format);
    size = dd2_audio_test_chunk(bytes, size, "data", (dd2_byte_view){0});
    dd2_test_write_le32(bytes + 4, (uint32_t)(size - DD2_AUDIO_TEST_CHUNK_HEADER));
    int16_t sample = 1;
    return dd2_wave_decode((dd2_byte_view){bytes, size}, &pcm) && pcm.frames == 0 &&
           !dd2_pcm_sample(&pcm, 0, 0, &sample) && sample == 0;
}

static bool dd2_audio_test_bank(void) {
    uint8_t wave[DD2_AUDIO_TEST_CAPACITY];
    const size_t wave_size = dd2_audio_test_wave(wave, 1, DD2_AUDIO_TEST_BYTE_BITS);
    uint8_t bytes[DD2_AUDIO_TEST_CAPACITY] = {0};
    const size_t size = DD2_AUDIO_TEST_BANK_DATA + wave_size;
    dd2_test_write_le32(bytes + DD2_AUDIO_TEST_BANK_SIZE, (uint32_t)size);
    dd2_test_write_le32(bytes + DD2_AUDIO_TEST_BANK_COUNT, DD2_PCM_STEREO);
    for (unsigned index = 0; index < DD2_PCM_STEREO; ++index) {
        uint8_t *record =
            bytes + DD2_AUDIO_TEST_BANK_HEADER + ((size_t)index * DD2_AUDIO_TEST_BANK_RECORD);
        dd2_test_write_le32(record, DD2_AUDIO_TEST_BANK_DATA);
        dd2_test_write_le32(record + 4, (uint32_t)wave_size);
        dd2_test_write_le32(record + DD2_AUDIO_TEST_CHUNK_HEADER, index);
        dd2_test_write_le32(record + DD2_AUDIO_TEST_RIFF_HEADER, DD2_AUDIO_TEST_FREQUENCY);
        dd2_test_write_le32(record + DD2_AUDIO_TEST_BANK_HEADER, index);
    }
    for (size_t index = 0; index < wave_size; ++index) {
        bytes[DD2_AUDIO_TEST_BANK_DATA + index] = wave[index];
    }
    uint8_t snapshot[DD2_AUDIO_TEST_CAPACITY];
    for (size_t index = 0; index < sizeof(snapshot); ++index) {
        snapshot[index] = bytes[index];
    }
    dd2_sound_bank *bank = dd2_sound_bank_create((dd2_byte_view){bytes, size});
    const dd2_sound *first = dd2_sound_bank_get(bank, 0);
    const dd2_sound *second = dd2_sound_bank_get(bank, 1);
    const bool valid =
        dd2_sound_bank_count(bank) == DD2_PCM_STEREO && first != NULL && second != NULL &&
        !first->loop && second->loop && first->frequency == DD2_AUDIO_TEST_FREQUENCY &&
        second->pcm.sample_rate == DD2_AUDIO_TEST_RATE && first->channel_flags == 0 &&
        second->channel_flags == 1 && first->pcm.samples.data == second->pcm.samples.data &&
        dd2_sound_bank_get(bank, DD2_PCM_STEREO) == NULL &&
        memcmp(snapshot, bytes, sizeof(bytes)) == 0;
    dd2_sound_bank_destroy(bank);
    const unsigned offsets[] = {DD2_AUDIO_TEST_BANK_HEADER, DD2_AUDIO_TEST_BANK_HEADER + 4,
                                DD2_AUDIO_TEST_BANK_SIZE, DD2_AUDIO_TEST_BANK_COUNT};
    for (unsigned index = 0; index < sizeof(offsets) / sizeof(offsets[0]); ++index) {
        for (size_t copy = 0; copy < sizeof(bytes); ++copy) {
            bytes[copy] = snapshot[copy];
        }
        dd2_test_write_le32(bytes + offsets[index], UINT32_MAX);
        bank = dd2_sound_bank_create((dd2_byte_view){bytes, size});
        if (bank != NULL) {
            dd2_sound_bank_destroy(bank);
            return false;
        }
    }
    for (size_t copy = 0; copy < sizeof(bytes); ++copy) {
        bytes[copy] = snapshot[copy];
    }
    dd2_test_write_le32(bytes + DD2_AUDIO_TEST_BANK_FREQUENCY, 0);
    bank = dd2_sound_bank_create((dd2_byte_view){bytes, size});
    if (bank != NULL) {
        dd2_sound_bank_destroy(bank);
        return false;
    }
    return valid && dd2_sound_bank_create((dd2_byte_view){0}) == NULL &&
           dd2_sound_bank_count(NULL) == 0 && dd2_sound_bank_get(NULL, 0) == NULL;
}

static bool dd2_audio_test_cdda(void) {
    const uint8_t samples[] = {0, DD2_AUDIO_TEST_UNSIGNED_MID, DD2_AUDIO_TEST_UNSIGNED_MAX,
                               DD2_AUDIO_TEST_UNSIGNED_MID - 1};
    dd2_pcm_view pcm = {0};
    int16_t left = 0;
    int16_t right = 0;
    return dd2_cdda_decode((dd2_byte_view){samples, sizeof(samples)}, &pcm) && pcm.frames == 1 &&
           dd2_pcm_sample(&pcm, 0, 0, &left) && left == INT16_MIN &&
           dd2_pcm_sample(&pcm, 0, 1, &right) && right == INT16_MAX &&
           !dd2_cdda_decode((dd2_byte_view){samples, sizeof(samples) - 1}, &pcm) &&
           pcm.samples.data == NULL && pcm.sample_rate == 0 &&
           !dd2_cdda_decode((dd2_byte_view){NULL, sizeof(samples)}, &pcm);
}

int main(void) {
    if (!dd2_audio_test_samples(1, DD2_AUDIO_TEST_BYTE_BITS) ||
        !dd2_audio_test_samples(DD2_PCM_STEREO, DD2_AUDIO_TEST_BYTE_BITS) ||
        !dd2_audio_test_samples(1, DD2_AUDIO_TEST_WORD_BITS) ||
        !dd2_audio_test_samples(DD2_PCM_STEREO, DD2_AUDIO_TEST_WORD_BITS) ||
        !dd2_audio_test_rejection() || !dd2_audio_test_chunks() || !dd2_audio_test_bank() ||
        !dd2_audio_test_cdda()) {
        puts("PCM/WAVE/SBK validation: FAIL");
        return EXIT_FAILURE;
    }
    puts("PCM/WAVE/SBK validation: PASS");
    return EXIT_SUCCESS;
}
