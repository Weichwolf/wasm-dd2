#include "assets/audio.h"
#include "assets/bytes.h"
#include "platform/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_AUDIO_EXPORT_BYTES = 16384, DD2_AUDIO_EXPORT_WORD_SHIFT = 8 };

static bool dd2_audio_export_pcm(FILE *output, const dd2_pcm_view *pcm) {
    uint8_t bytes[DD2_AUDIO_EXPORT_BYTES] = {0};
    size_t used = 0;
    for (size_t frame = 0; frame < pcm->frames; ++frame) {
        for (unsigned channel = 0; channel < pcm->channels; ++channel) {
            int16_t sample = 0;
            if (!dd2_pcm_sample(pcm, frame, channel, &sample)) {
                return false;
            }
            const uint16_t word = (uint16_t)sample;
            bytes[used++] = (uint8_t)word;
            bytes[used++] = (uint8_t)(word >> DD2_AUDIO_EXPORT_WORD_SHIFT);
            if (used == sizeof(bytes)) {
                if (fwrite(bytes, 1, used, output) != used) {
                    return false;
                }
                used = 0;
            }
        }
    }
    return used == 0 || fwrite(bytes, 1, used, output) == used;
}

static bool dd2_audio_export_bank(FILE *output, dd2_byte_view bytes) {
    dd2_sound_bank *bank = dd2_sound_bank_create(bytes);
    if (bank == NULL) {
        return false;
    }
    bool valid = true;
    for (unsigned index = 0; valid && index < dd2_sound_bank_count(bank); ++index) {
        const dd2_sound *sound = dd2_sound_bank_get(bank, index);
        if (sound == NULL) {
            valid = false;
            break;
        }
        printf("{\"index\":%u,\"frames\":%zu,\"rate\":%u,\"frequency\":%u,"
               "\"channels\":%u,\"bits\":%u,\"loop\":%u,\"flags\":%u}\n",
               index, sound->pcm.frames, sound->pcm.sample_rate, sound->frequency,
               (unsigned)sound->pcm.channels, (unsigned)sound->pcm.bits_per_sample,
               (unsigned)sound->loop, sound->channel_flags);
        valid = dd2_audio_export_pcm(output, &sound->pcm);
    }
    dd2_sound_bank_destroy(bank);
    return valid;
}

static bool dd2_audio_export_cdda(FILE *output, dd2_byte_view bytes) {
    dd2_pcm_view pcm = {0};
    if (!dd2_cdda_decode(bytes, &pcm)) {
        return false;
    }
    printf("{\"frames\":%zu,\"rate\":%u,\"channels\":%u,\"bits\":%u}\n", pcm.frames,
           pcm.sample_rate, (unsigned)pcm.channels, (unsigned)pcm.bits_per_sample);
    return dd2_audio_export_pcm(output, &pcm);
}

int main(int argc, char **argv) {
    if (argc != 4 || (strcmp(argv[1], "bank") != 0 && strcmp(argv[1], "cdda") != 0)) {
        (void)fputs("usage: dd2_audio_export bank|cdda INPUT OUTPUT\n", stderr);
        return EXIT_FAILURE;
    }
    dd2_file input = {0};
    if (!dd2_file_read(argv[2], &input)) {
        return EXIT_FAILURE;
    }
    FILE *output = fopen(argv[3], "wb");
    if (output == NULL) {
        dd2_file_release(&input);
        return EXIT_FAILURE;
    }
    const dd2_byte_view bytes = {.data = input.data, .size = input.size};
    const bool valid = strcmp(argv[1], "bank") == 0 ? dd2_audio_export_bank(output, bytes)
                                                    : dd2_audio_export_cdda(output, bytes);
    const bool closed = fclose(output) == 0;
    dd2_file_release(&input);
    return valid && closed ? EXIT_SUCCESS : EXIT_FAILURE;
}
