#include "assets/bytes.h"
#include "assets/car_class.h"
#include "assets/save_card.h"
#include "assets/save_profile.h"
#include "audio/mixer.h"
#include "game/configuration.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_CONFIG_TEST_PATTERN = 251, DD2_CONFIG_TEST_BAD_GAIN = 257 };
static uint8_t dd2_config_test_block[DD2_SAVE_CARD_BLOCK_BYTES];
static uint8_t dd2_config_test_before[DD2_SAVE_CARD_BLOCK_BYTES];
static dd2_configuration dd2_config_test_owner;
static void dd2_config_test_require(bool good, const char *message) {
    if (!good) {
        if (fputs(message, stderr) == EOF) {
            abort();
        }
        exit(EXIT_FAILURE);
    }
}
static dd2_byte_view dd2_config_test_view(void) {
    return (dd2_byte_view){.data = dd2_config_test_block, .size = sizeof(dd2_config_test_block)};
}
static dd2_byte_buffer dd2_config_test_output(void) {
    return (dd2_byte_buffer){.data = dd2_config_test_block, .size = sizeof(dd2_config_test_block)};
}
static void dd2_config_test_failed_read(dd2_byte_view bytes) {
    const dd2_configuration before = dd2_config_test_owner;
    dd2_config_test_require(!dd2_configuration_read(bytes, 0, &dd2_config_test_owner) &&
                                memcmp(&before, &dd2_config_test_owner, sizeof(before)) == 0,
                            "Invalid configuration changed its owner\n");
}
static void dd2_config_test_gains(void) {
    dd2_configuration source;
    dd2_configuration_defaults(&source);
    dd2_config_test_require(source.source.header.controller_type == 1 &&
                                dd2_configuration_effects_gain(&source) == DD2_MIXER_GAIN_ONE,
                            "Factory preferences invalid\n");
    for (unsigned gain = 0; gain <= DD2_MIXER_GAIN_ONE; ++gain) {
        dd2_config_test_require(
            dd2_configuration_set_effects(&source, gain) &&
                dd2_configuration_set_music(&source, gain) &&
                dd2_configuration_write(&source, dd2_config_test_output()) &&
                dd2_configuration_read(dd2_config_test_view(), 0, &dd2_config_test_owner) &&
                dd2_config_test_owner.music_gain == gain &&
                dd2_configuration_effects_gain(&dd2_config_test_owner) == gain &&
                dd2_config_test_owner.source.header.controller_type == 1,
            "Gain round trip changed gain or controller\n");
    }
    for (unsigned volume = 0; volume <= DD2_CONFIGURATION_EFFECTS_MAX; ++volume) {
        source.source.header.effects_volume = (int16_t)volume;
        const unsigned expected =
            (volume * DD2_MIXER_GAIN_ONE + (DD2_CONFIGURATION_EFFECTS_MAX / 2)) /
            DD2_CONFIGURATION_EFFECTS_MAX;
        dd2_config_test_require(
            dd2_configuration_write(&source, dd2_config_test_output()) &&
                dd2_configuration_read(dd2_config_test_view(), 0, &dd2_config_test_owner) &&
                dd2_config_test_owner.source.header.effects_volume == (int16_t)volume &&
                dd2_configuration_effects_gain(&dd2_config_test_owner) == expected,
            "Exact original effects volume was quantized on load\n");
    }
    const dd2_configuration before = source;
    dd2_config_test_require(!dd2_configuration_set_music(&source, DD2_CONFIG_TEST_BAD_GAIN) &&
                                !dd2_configuration_set_effects(&source, DD2_CONFIG_TEST_BAD_GAIN) &&
                                memcmp(&before, &source, sizeof(source)) == 0,
                            "Invalid gain partially applied\n");
}
static void dd2_config_test_car(void) {
    dd2_configuration source;
    dd2_configuration_defaults(&source);
    dd2_car_class selected = DD2_CAR_PRO;
    dd2_config_test_require(!dd2_configuration_car(NULL, &selected) && selected == DD2_CAR_PRO &&
                                !dd2_configuration_car(&source, NULL) &&
                                !dd2_configuration_set_car(NULL, DD2_CAR_ROOKIE) &&
                                !dd2_configuration_set_car(&source, DD2_CAR_CLASSES) &&
                                source.source.header.car == 0,
                            "Invalid class access changed configuration\n");
    for (unsigned index = 0; index < DD2_CAR_CLASSES; ++index) {
        dd2_config_test_require(
            dd2_configuration_set_car(&source, (dd2_car_class)index) &&
                dd2_configuration_write(&source, dd2_config_test_output()) &&
                dd2_configuration_read(dd2_config_test_view(), 0, &dd2_config_test_owner) &&
                dd2_configuration_car(&dd2_config_test_owner, &selected) &&
                (unsigned)selected == index,
            "Selected class did not survive the original profile codec\n");
    }
    static const int16_t invalid[] = {INT16_MIN, -1, DD2_CAR_CLASSES, INT16_MAX};
    for (unsigned index = 0; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        source.source.header.car = invalid[index];
        selected = DD2_CAR_PRO;
        dd2_config_test_require(
            dd2_configuration_write(&source, dd2_config_test_output()) &&
                dd2_configuration_read(dd2_config_test_view(), 0, &dd2_config_test_owner) &&
                dd2_config_test_owner.source.header.car == invalid[index] &&
                !dd2_configuration_car(&dd2_config_test_owner, &selected) &&
                selected == DD2_CAR_PRO,
            "Dormant class was discarded or invalid class was published\n");
    }
}
static void dd2_config_test_legacy(void) {
    dd2_configuration source;
    dd2_configuration_defaults(&source);
    source.source.header.kind = DD2_SAVE_PROFILE_STARTUP;
    source.source.header.effects_volume = DD2_CONFIGURATION_EFFECTS_MAX - 1;
    for (unsigned index = 0; index < sizeof(source.source.reserved); ++index) {
        source.source.reserved[index] = (uint8_t)(1 + (index % DD2_CONFIG_TEST_PATTERN));
    }
    dd2_config_test_require(dd2_save_profile_write(&source.source, dd2_config_test_output()) &&
                                dd2_configuration_read(dd2_config_test_view(), DD2_MIXER_GAIN_ONE,
                                                       &dd2_config_test_owner) &&
                                dd2_config_test_owner.music_gain == DD2_MIXER_GAIN_ONE,
                            "Legacy startup configuration did not preserve music fallback\n");
    for (unsigned index = 0; index < sizeof(dd2_config_test_before); ++index) {
        dd2_config_test_before[index] = dd2_config_test_block[index];
    }
    dd2_config_test_require(
        dd2_configuration_write(&dd2_config_test_owner, dd2_config_test_output()),
        "Legacy configuration could not be saved\n");
    for (unsigned index = 2; index < sizeof(dd2_config_test_before); ++index) {
        if (index < DD2_SAVE_PROFILE_BYTES ||
            index >= DD2_SAVE_PROFILE_BYTES + DD2_CONFIGURATION_EXTENSION_BYTES) {
            dd2_config_test_require(dd2_config_test_block[index] == dd2_config_test_before[index],
                                    "Preference edit changed retained source bytes\n");
        }
    }
    for (unsigned index = DD2_SAVE_PROFILE_BYTES + 4;
         index < DD2_SAVE_PROFILE_BYTES + DD2_CONFIGURATION_EXTENSION_BYTES; ++index) {
        dd2_config_test_block[index] ^= 1;
        dd2_config_test_failed_read(dd2_config_test_view());
        dd2_config_test_block[index] ^= 1;
    }
    for (size_t size = 0; size < sizeof(dd2_config_test_block); ++size) {
        dd2_config_test_failed_read((dd2_byte_view){.data = dd2_config_test_block, .size = size});
    }
    dd2_config_test_require(!dd2_configuration_read(dd2_config_test_view(),
                                                    DD2_CONFIG_TEST_BAD_GAIN,
                                                    &dd2_config_test_owner),
                            "Invalid fallback accepted\n");
    dd2_config_test_block[0] = (uint8_t)DD2_SAVE_PROFILE_GAME;
    dd2_config_test_block[1] = (uint8_t)(DD2_SAVE_PROFILE_GAME >> DD2_BYTE_BITS);
    dd2_config_test_failed_read(dd2_config_test_view());
    source.source.header.effects_volume = -1;
    dd2_config_test_require(dd2_save_profile_write(&source.source, dd2_config_test_output()),
                            "Negative original volume fixture invalid\n");
    dd2_config_test_failed_read(dd2_config_test_view());
    source.source.header.effects_volume = DD2_CONFIGURATION_EFFECTS_MAX + 1;
    dd2_config_test_require(dd2_save_profile_write(&source.source, dd2_config_test_output()),
                            "Excess original volume fixture invalid\n");
    dd2_config_test_failed_read(dd2_config_test_view());
}
int main(int argc, char **argv) {
    dd2_configuration_defaults(&dd2_config_test_owner);
    dd2_configuration_defaults(NULL);
    dd2_config_test_gains();
    dd2_config_test_car();
    dd2_config_test_legacy();
    if (argc == 2) {
        dd2_configuration_defaults(&dd2_config_test_owner);
        dd2_config_test_require(
            dd2_configuration_write(&dd2_config_test_owner, dd2_config_test_output()),
            "Factory encoding failed\n");
        FILE *file = fopen(argv[1], "wb");
        dd2_config_test_require(file != NULL, "Factory output cannot open\n");
        const bool written = fwrite(dd2_config_test_block, 1, sizeof(dd2_config_test_block),
                                    file) == sizeof(dd2_config_test_block);
        const bool closed = fclose(file) == 0;
        dd2_config_test_require(written && closed, "Factory output incomplete\n");
    }
    return EXIT_SUCCESS;
}
