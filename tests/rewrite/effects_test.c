#include "asset_fixture.h"
#include "assets/audio.h"
#include "assets/bytes.h"
#include "audio/effects.h"
#include "audio/mixer.h"
#include "game/sound_events.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_EFFECT_TEST_COUNT = 12,
    DD2_EFFECT_TEST_HEADER = 16,
    DD2_EFFECT_TEST_RECORD = 28,
    DD2_EFFECT_TEST_WAVE = 48,
    DD2_EFFECT_TEST_TABLE =
        DD2_EFFECT_TEST_HEADER + (DD2_EFFECT_TEST_COUNT * DD2_EFFECT_TEST_RECORD),
    DD2_EFFECT_TEST_SIZE = DD2_EFFECT_TEST_TABLE + (DD2_EFFECT_TEST_COUNT * DD2_EFFECT_TEST_WAVE),
    DD2_EFFECT_TEST_RATE = 8000,
    DD2_EFFECT_TEST_FREQUENCY = 4000,
    DD2_EFFECT_TEST_IDLE = 6192,
    DD2_EFFECT_TEST_EXPECTED = 2896,
    DD2_EFFECT_TEST_ENGINE_GAIN = 80,
    DD2_EFFECT_TEST_SPEED = 2000,
    DD2_EFFECT_TEST_BYTE_CENTER = 128,
    DD2_EFFECT_TEST_WORD_BITS = 16,
    DD2_EFFECT_TEST_DATA = 44,
    DD2_EFFECT_TEST_FMT = 16,
    DD2_EFFECT_TEST_BITS = 8,
    DD2_EFFECT_TEST_BYTE_RATE = 28,
    DD2_EFFECT_TEST_ALIGN = 32,
    DD2_EFFECT_TEST_DATA_TAG = 36,
    DD2_EFFECT_TEST_DATA_SIZE = 40,
    DD2_EFFECT_TEST_BANK_SIZE_OFFSET = 8,
    DD2_EFFECT_TEST_BANK_COUNT_OFFSET = 12,
    DD2_EFFECT_TEST_BANK_LOOP_OFFSET = 8,
    DD2_EFFECT_TEST_BANK_FREQ_OFFSET = 12,
    DD2_EFFECT_TEST_WAVE_TAG = 8,
    DD2_EFFECT_TEST_FMT_TAG = 12,
    DD2_EFFECT_TEST_PCM_FORMAT = 20,
    DD2_EFFECT_TEST_CHANNELS = 22,
    DD2_EFFECT_TEST_RATE_OFFSET = 24,
    DD2_EFFECT_TEST_BITS_OFFSET = 34,
    DD2_EFFECT_TEST_GO_SAMPLE = 8
};

static dd2_sound_bank *dd2_effect_test_bank(uint8_t bytes[DD2_EFFECT_TEST_SIZE]) {
    dd2_test_write_le32(bytes + DD2_EFFECT_TEST_BANK_SIZE_OFFSET, DD2_EFFECT_TEST_SIZE);
    dd2_test_write_le32(bytes + DD2_EFFECT_TEST_BANK_COUNT_OFFSET, DD2_EFFECT_TEST_COUNT);
    const uint8_t tags[] = {'R', 'I', 'F', 'F', 'W', 'A', 'V', 'E',
                            'f', 'm', 't', ' ', 'd', 'a', 't', 'a'};
    for (unsigned index = 0; index < DD2_EFFECT_TEST_COUNT; ++index) {
        const unsigned offset = DD2_EFFECT_TEST_TABLE + (index * DD2_EFFECT_TEST_WAVE);
        uint8_t *record = bytes + DD2_EFFECT_TEST_HEADER + ((size_t)index * DD2_EFFECT_TEST_RECORD);
        dd2_test_write_le32(record, offset);
        dd2_test_write_le32(record + 4, DD2_EFFECT_TEST_WAVE);
        dd2_test_write_le32(record + DD2_EFFECT_TEST_BANK_LOOP_OFFSET, index == 0);
        dd2_test_write_le32(record + DD2_EFFECT_TEST_BANK_FREQ_OFFSET, DD2_EFFECT_TEST_FREQUENCY);
        uint8_t *wave = bytes + offset;
        for (unsigned tag = 0; tag < 4; ++tag) {
            wave[tag] = tags[tag];
            wave[DD2_EFFECT_TEST_WAVE_TAG + tag] = tags[4 + tag];
            wave[DD2_EFFECT_TEST_FMT_TAG + tag] = tags[DD2_EFFECT_TEST_WAVE_TAG + tag];
            wave[DD2_EFFECT_TEST_DATA_TAG + tag] = tags[DD2_EFFECT_TEST_FMT_TAG + tag];
        }
        dd2_test_write_le32(wave + 4, DD2_EFFECT_TEST_WAVE - DD2_EFFECT_TEST_WAVE_TAG);
        dd2_test_write_le32(wave + DD2_EFFECT_TEST_FMT, DD2_EFFECT_TEST_FMT);
        dd2_test_write_le16(wave + DD2_EFFECT_TEST_PCM_FORMAT, 1);
        dd2_test_write_le16(wave + DD2_EFFECT_TEST_CHANNELS, 1);
        dd2_test_write_le32(wave + DD2_EFFECT_TEST_RATE_OFFSET, DD2_EFFECT_TEST_RATE);
        dd2_test_write_le32(wave + DD2_EFFECT_TEST_BYTE_RATE, DD2_EFFECT_TEST_RATE);
        dd2_test_write_le16(wave + DD2_EFFECT_TEST_ALIGN, 1);
        dd2_test_write_le16(wave + DD2_EFFECT_TEST_BITS_OFFSET, DD2_EFFECT_TEST_BITS);
        dd2_test_write_le32(wave + DD2_EFFECT_TEST_DATA_SIZE, 4);
        for (unsigned frame = 0; frame < 4; ++frame) {
            wave[DD2_EFFECT_TEST_DATA + frame] = (uint8_t)(DD2_EFFECT_TEST_BYTE_CENTER + index + 1);
        }
    }
    return dd2_sound_bank_create((dd2_byte_view){bytes, DD2_EFFECT_TEST_SIZE});
}

static bool dd2_effect_test_mix(dd2_effects *effects, dd2_mixer *mixer) {
    const dd2_sound_batch events = {
        .events =
            {{.cue = DD2_SOUND_GO, .gain = DD2_MIXER_GAIN_ONE},
             {.cue = DD2_SOUND_IMPACT, .gain = DD2_MIXER_GAIN_ONE / 2, .pan = DD2_MIXER_PAN_LEFT},
             {.cue = DD2_SOUND_IMPACT, .gain = DD2_MIXER_GAIN_ONE / 2, .pan = DD2_MIXER_PAN_RIGHT}},
        .count = 3};
    int16_t output[DD2_PCM_STEREO] = {0};
    if (!dd2_effects_reset(effects, mixer) ||
        !dd2_effects_update(effects, mixer, (dd2_engine_sound){.running = true}, &events) ||
        !dd2_mixer_render(mixer, output, 1) || output[0] != DD2_EFFECT_TEST_EXPECTED ||
        output[1] != DD2_EFFECT_TEST_EXPECTED) {
        return false;
    }
    const dd2_effect_voice before = dd2_effects_voice(effects, mixer, 0);
    const dd2_sound_batch empty = {0};
    if (before.voice.config.frequency != DD2_EFFECT_TEST_IDLE || !before.voice.locked ||
        dd2_effects_voice(effects, mixer, 1).sample != DD2_EFFECT_TEST_GO_SAMPLE ||
        dd2_effects_played(effects, DD2_SOUND_IMPACT) != 2 ||
        !dd2_effects_update(
            effects, mixer,
            (dd2_engine_sound){.running = true, .speed = DD2_EFFECT_TEST_SPEED, .throttle = 1},
            &empty)) {
        return false;
    }
    dd2_effect_voice after = dd2_effects_voice(effects, mixer, 0);
    if (after.voice.frame != before.voice.frame || after.voice.fraction != before.voice.fraction ||
        after.voice.config.frequency <= before.voice.config.frequency) {
        return false;
    }
    if (!dd2_effects_gain(effects, mixer, DD2_MIXER_GAIN_ONE / 2) ||
        dd2_effects_gain(effects, mixer, DD2_MIXER_GAIN_ONE + 1)) {
        return false;
    }
    after = dd2_effects_voice(effects, mixer, 0);
    if (after.voice.config.gain != DD2_EFFECT_TEST_ENGINE_GAIN / 2 ||
        after.voice.frame != before.voice.frame || after.voice.fraction != before.voice.fraction) {
        return false;
    }
    dd2_sound_batch invalid = {.events = {{.cue = DD2_SOUND_GO, .gain = DD2_MIXER_GAIN_ONE + 1}},
                               .count = 1};
    if (dd2_effects_update(effects, mixer, (dd2_engine_sound){.running = false}, &invalid) ||
        !dd2_effects_voice(effects, mixer, 0).voice.playing) {
        return false;
    }
    return dd2_effects_update(effects, mixer, (dd2_engine_sound){0}, &empty) &&
           !dd2_effects_voice(effects, mixer, 0).voice.playing;
}

int main(void) {
    uint8_t bytes[DD2_EFFECT_TEST_SIZE] = {0};
    dd2_sound_bank *bank = dd2_effect_test_bank(bytes);
    dd2_effects *effects = dd2_effects_create(bank);
    dd2_mixer *mixer = dd2_mixer_create(DD2_EFFECT_TEST_RATE);
    const bool valid = effects != NULL && mixer != NULL && dd2_effect_test_mix(effects, mixer);
    dd2_effects_destroy(effects);
    dd2_mixer_destroy(mixer);
    dd2_sound_bank_destroy(bank);
    printf("Gameplay effects mixer: %s\n", valid ? "PASS" : "FAIL");
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
