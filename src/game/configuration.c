#include "game/configuration.h"

#include "assets/bytes.h"
#include "assets/car_class.h"
#include "assets/save_profile.h"
#include "audio/mixer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum {
    DD2_CONFIG_DEFAULT_DISTANCE = 26000,
    DD2_CONFIG_INITIAL_CIRCUITS = 4,
    DD2_CONFIG_DEFAULT_CARS = 20,
    DD2_CONFIG_VERSION = 1,
    DD2_CONFIG_TAG_BYTES = 4,
    DD2_CONFIG_VERSION_OFFSET = 4,
    DD2_CONFIG_LENGTH_OFFSET = 6,
    DD2_CONFIG_GAIN_OFFSET = 8,
    DD2_CONFIG_FLAGS_OFFSET = 10,
    DD2_CONFIG_CHECK_OFFSET = 12
};
static const uint8_t dd2_config_tag[DD2_CONFIG_TAG_BYTES] = {'D', '2', 'C', 'F'};
bool dd2_configuration_car(const dd2_configuration *configuration, dd2_car_class *car_class) {
    if (configuration == NULL || car_class == NULL || configuration->source.header.car < 0 ||
        configuration->source.header.car >= DD2_CAR_CLASSES) {
        return false;
    }
    *car_class = (dd2_car_class)configuration->source.header.car;
    return true;
}

bool dd2_configuration_set_car(dd2_configuration *configuration, dd2_car_class car_class) {
    if (configuration == NULL || (unsigned)car_class >= DD2_CAR_CLASSES) {
        return false;
    }
    configuration->source.header.car = (int16_t)car_class;
    return true;
}

static uint32_t dd2_config_check(const uint8_t *bytes) {
    uint32_t hash = UINT32_C(2166136261);
    for (unsigned index = 0; index < DD2_CONFIG_CHECK_OFFSET; ++index) {
        hash = (hash ^ bytes[index]) * UINT32_C(16777619);
    }
    return hash;
}
static void dd2_config_word(uint8_t *bytes, uint16_t value) {
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> DD2_BYTE_BITS);
}
void dd2_configuration_defaults(dd2_configuration *configuration) {
    if (configuration == NULL) {
        return;
    }
    *configuration =
        (dd2_configuration){.source.header = {.kind = DD2_SAVE_PROFILE_CONFIG,
                                              .unlocked_circuits = DD2_CONFIG_INITIAL_CIRCUITS,
                                              .unlocked_arenas = 1,
                                              .view_distance = DD2_CONFIG_DEFAULT_DISTANCE,
                                              .effects_volume = DD2_CONFIGURATION_EFFECTS_MAX,
                                              .controller_type = 1,
                                              .races = 1,
                                              .players = 1,
                                              .cars = DD2_CONFIG_DEFAULT_CARS,
                                              .active_players = 1},
                            .music_gain = DD2_MIXER_GAIN_ONE};
    static const uint8_t bindings[DD2_SAVE_PROFILE_BINDINGS] = {0x0d, 0x1b, 0x26, 0x27, 0x28, 0x25,
                                                                0x70, 0x71, 0x20, 0x20, 0x57, 0x53,
                                                                0x41, 0x5a, 0x00, 0x00, 0x00, 0x00};
    for (unsigned index = 0; index < DD2_SAVE_PROFILE_BINDINGS; ++index) {
        configuration->source.bindings[index] = bindings[index];
    }
    static const uint16_t seconds[DD2_SAVE_PROFILE_CIRCUITS] = {25, 0, 50, 26, 40, 30, 45};
    for (unsigned circuit = 0; circuit < DD2_SAVE_PROFILE_CIRCUITS; ++circuit) {
        for (unsigned rank = 0; rank < DD2_SAVE_PROFILE_LAPS; ++rank) {
            dd2_save_profile_lap *lap = &configuration->source.laps[circuit][rank];
            static const char name[] = "Anon";
            for (unsigned index = 0; index < sizeof(name); ++index) {
                lap->name[index] = name[index];
            }
            lap->minutes = (uint16_t)(circuit == 1);
            lap->seconds = (uint16_t)(seconds[circuit] + rank);
        }
    }
}
static bool dd2_config_audio_valid(const dd2_configuration *configuration) {
    return configuration != NULL && configuration->music_gain <= DD2_MIXER_GAIN_ONE &&
           (configuration->source.header.kind == DD2_SAVE_PROFILE_CONFIG ||
            configuration->source.header.kind == DD2_SAVE_PROFILE_STARTUP) &&
           configuration->source.header.effects_volume >= 0 &&
           configuration->source.header.effects_volume <= DD2_CONFIGURATION_EFFECTS_MAX;
}
bool dd2_configuration_read(dd2_byte_view bytes, unsigned fallback_music_gain,
                            dd2_configuration *out) {
    dd2_configuration next = {.music_gain = fallback_music_gain};
    if (out == NULL || fallback_music_gain > DD2_MIXER_GAIN_ONE ||
        !dd2_save_profile_read(bytes, &next.source)) {
        return false;
    }
    const uint8_t *extension = next.source.reserved;
    if (memcmp(extension, dd2_config_tag, sizeof(dd2_config_tag)) == 0) {
        if (dd2_read_le16(extension + DD2_CONFIG_VERSION_OFFSET) != DD2_CONFIG_VERSION ||
            dd2_read_le16(extension + DD2_CONFIG_LENGTH_OFFSET) !=
                DD2_CONFIGURATION_EXTENSION_BYTES ||
            dd2_read_le16(extension + DD2_CONFIG_FLAGS_OFFSET) != 0 ||
            dd2_read_le32(extension + DD2_CONFIG_CHECK_OFFSET) != dd2_config_check(extension)) {
            return false;
        }
        next.music_gain = dd2_read_le16(extension + DD2_CONFIG_GAIN_OFFSET);
    }
    if (!dd2_config_audio_valid(&next)) {
        return false;
    }
    *out = next;
    return true;
}
bool dd2_configuration_write(const dd2_configuration *configuration, dd2_byte_buffer output) {
    if (!dd2_config_audio_valid(configuration)) {
        return false;
    }
    dd2_save_profile profile = configuration->source;
    profile.header.kind = DD2_SAVE_PROFILE_CONFIG;
    for (unsigned index = 0; index < sizeof(dd2_config_tag); ++index) {
        profile.reserved[index] = dd2_config_tag[index];
    }
    dd2_config_word(profile.reserved + DD2_CONFIG_VERSION_OFFSET, DD2_CONFIG_VERSION);
    dd2_config_word(profile.reserved + DD2_CONFIG_LENGTH_OFFSET, DD2_CONFIGURATION_EXTENSION_BYTES);
    dd2_config_word(profile.reserved + DD2_CONFIG_GAIN_OFFSET, (uint16_t)configuration->music_gain);
    dd2_config_word(profile.reserved + DD2_CONFIG_FLAGS_OFFSET, 0);
    const uint32_t check = dd2_config_check(profile.reserved);
    for (unsigned index = 0; index < sizeof(check); ++index) {
        profile.reserved[DD2_CONFIG_CHECK_OFFSET + index] =
            (uint8_t)(check >> (index * DD2_BYTE_BITS));
    }
    return dd2_save_profile_write(&profile, output);
}
unsigned dd2_configuration_effects_gain(const dd2_configuration *configuration) {
    return dd2_config_audio_valid(configuration)
               ? ((unsigned)configuration->source.header.effects_volume * DD2_MIXER_GAIN_ONE +
                  (DD2_CONFIGURATION_EFFECTS_MAX / 2)) /
                     DD2_CONFIGURATION_EFFECTS_MAX
               : 0;
}
bool dd2_configuration_set_effects(dd2_configuration *configuration, unsigned gain) {
    if (!dd2_config_audio_valid(configuration) || gain > DD2_MIXER_GAIN_ONE) {
        return false;
    }
    configuration->source.header.effects_volume =
        (int16_t)((gain * DD2_CONFIGURATION_EFFECTS_MAX + (DD2_MIXER_GAIN_ONE / 2)) /
                  DD2_MIXER_GAIN_ONE);
    return true;
}
bool dd2_configuration_set_music(dd2_configuration *configuration, unsigned gain) {
    if (!dd2_config_audio_valid(configuration) || gain > DD2_MIXER_GAIN_ONE) {
        return false;
    }
    configuration->music_gain = gain;
    return true;
}

static bool dd2_configuration_text_valid(const char *name, unsigned limit) {
    if (name == NULL) {
        return false;
    }
    for (unsigned index = 0; index <= limit; ++index) {
        const unsigned char character = (unsigned char)name[index];
        if (character == 0) {
            return true;
        }
        if (index == limit || character < ' ' || character > '~') {
            return false;
        }
    }
    return false;
}
bool dd2_configuration_name_valid(const char *name) {
    return dd2_configuration_text_valid(name, DD2_CONFIGURATION_NAME_LIMIT);
}
bool dd2_configuration_player_valid(const char *name) {
    return dd2_configuration_text_valid(name, DD2_SAVE_PROFILE_PLAYER_NAME - 1);
}
const char *dd2_configuration_player(const dd2_configuration *configuration) {
    if (configuration == NULL ||
        !dd2_configuration_player_valid(configuration->source.players[0])) {
        return NULL;
    }
    return configuration->source.players[0][0] == '\0' ? "PLAYER"
                                                       : configuration->source.players[0];
}
bool dd2_configuration_set_player(dd2_configuration *configuration, const char *name) {
    if (configuration == NULL || !dd2_configuration_player_valid(name)) {
        return false;
    }
    const char *value = name[0] == '\0' ? "PLAYER" : name;
    char next[DD2_SAVE_PROFILE_PLAYER_NAME] = {0};
    unsigned length = 0;
    do {
        next[length] = value[length];
    } while (value[length++] != '\0');
    for (unsigned index = 0; index < length; ++index) {
        configuration->source.players[0][index] = next[index];
    }
    return true;
}
