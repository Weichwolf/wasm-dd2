#include "assets/save_profile.h"

#include "assets/bytes.h"
#include "assets/save_card.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static int16_t dd2_profile_read_signed(const uint8_t **bytes) {
    const int16_t value = dd2_read_le_i16(*bytes);
    *bytes += sizeof(value);
    return value;
}
static uint16_t dd2_profile_read_unsigned(const uint8_t **bytes) {
    const uint16_t value = dd2_read_le16(*bytes);
    *bytes += sizeof(value);
    return value;
}
static void dd2_profile_read_bytes(const uint8_t **bytes, void *out, size_t size) {
    unsigned char *destination = out;
    for (size_t index = 0; index < size; ++index) {
        destination[index] = (*bytes)[index];
    }
    *bytes += size;
}
static void dd2_profile_write_word(uint8_t **bytes, uint16_t value) {
    (*bytes)[0] = (uint8_t)value;
    (*bytes)[1] = (uint8_t)(value >> DD2_BYTE_BITS);
    *bytes += sizeof(value);
}
static void dd2_profile_write_bytes(uint8_t **bytes, const void *source, size_t size) {
    const unsigned char *input = source;
    for (size_t index = 0; index < size; ++index) {
        (*bytes)[index] = input[index];
    }
    *bytes += size;
}
static bool dd2_profile_season_valid(const dd2_save_profile_season *record) {
    for (unsigned track = 0; track < DD2_SAVE_PROFILE_TRACKS; ++track) {
        if (memchr(record->tracks[track].winner, 0, DD2_SAVE_PROFILE_NAME) == NULL) {
            return false;
        }
    }
    for (unsigned driver = 0; driver < DD2_SAVE_PROFILE_DRIVERS; ++driver) {
        if (memchr(record->drivers[driver].name, 0, DD2_SAVE_PROFILE_NAME) == NULL ||
            memchr(record->standings[driver], 0, DD2_SAVE_PROFILE_NAME) == NULL) {
            return false;
        }
    }
    return true;
}
static bool dd2_profile_valid(const dd2_save_profile *profile) {
    if (profile == NULL || (profile->header.kind != DD2_SAVE_PROFILE_CONFIG &&
                            profile->header.kind != DD2_SAVE_PROFILE_STARTUP &&
                            profile->header.kind != DD2_SAVE_PROFILE_GAME)) {
        return false;
    }
    for (unsigned season = 0; season < DD2_SAVE_PROFILE_SEASONS; ++season) {
        if (!dd2_profile_season_valid(&profile->seasons[season])) {
            return false;
        }
    }
    for (unsigned driver = 0; driver < DD2_SAVE_PROFILE_DRIVERS; ++driver) {
        if (memchr(profile->drivers[driver].name, 0, DD2_SAVE_PROFILE_NAME) == NULL) {
            return false;
        }
    }
    for (unsigned player = 0; player < DD2_SAVE_PROFILE_PLAYERS; ++player) {
        if (memchr(profile->players[player], 0, DD2_SAVE_PROFILE_PLAYER_NAME) == NULL) {
            return false;
        }
    }
    for (unsigned circuit = 0; circuit < DD2_SAVE_PROFILE_CIRCUITS; ++circuit) {
        for (unsigned lap = 0; lap < DD2_SAVE_PROFILE_LAPS; ++lap) {
            if (memchr(profile->laps[circuit][lap].name, 0, DD2_SAVE_PROFILE_LAP_NAME) == NULL) {
                return false;
            }
        }
    }
    return true;
}
bool dd2_save_profile_read(dd2_byte_view bytes, dd2_save_profile *out) {
    if (out == NULL || bytes.data == NULL || bytes.size != DD2_SAVE_CARD_BLOCK_BYTES) {
        return false;
    }
    dd2_save_profile staged = {0};
    const uint8_t *cursor = bytes.data;
    staged.header.kind = dd2_profile_read_signed(&cursor);
    staged.header.race_mode = dd2_profile_read_signed(&cursor);
    staged.header.race_type = dd2_profile_read_signed(&cursor);
    staged.header.car = dd2_profile_read_signed(&cursor);
    staged.header.track = dd2_profile_read_signed(&cursor);
    staged.header.unlocked_circuits = dd2_profile_read_signed(&cursor);
    staged.header.unlocked_arenas = dd2_profile_read_signed(&cursor);
    staged.header.view_distance = dd2_profile_read_signed(&cursor);
    staged.header.effects_volume = dd2_profile_read_signed(&cursor);
    staged.header.controller_type = dd2_profile_read_signed(&cursor);
    staged.header.controller_option = dd2_profile_read_signed(&cursor);
    staged.header.quick_race_type = dd2_profile_read_signed(&cursor);
    staged.header.statistics_recorded = dd2_profile_read_signed(&cursor);
    staged.header.player_car = dd2_profile_read_signed(&cursor);
    staged.header.player = dd2_profile_read_signed(&cursor);
    staged.header.simultaneous_players = dd2_profile_read_signed(&cursor);
    staged.header.race = dd2_profile_read_signed(&cursor);
    staged.header.difficulty = dd2_profile_read_signed(&cursor);
    staged.header.races = dd2_profile_read_signed(&cursor);
    staged.header.players = dd2_profile_read_signed(&cursor);
    staged.header.cars = dd2_profile_read_signed(&cursor);
    staged.header.active_players = dd2_profile_read_signed(&cursor);
    staged.header.recording_season = dd2_profile_read_signed(&cursor);
    staged.header.season_number = dd2_profile_read_signed(&cursor);

    for (unsigned season = 0; season < DD2_SAVE_PROFILE_SEASONS; ++season) {
        dd2_save_profile_season *record = &staged.seasons[season];
        for (unsigned track = 0; track < DD2_SAVE_PROFILE_TRACKS; ++track) {
            dd2_save_profile_track *entry = &record->tracks[track];
            dd2_profile_read_bytes(&cursor, entry->winner, sizeof(entry->winner));
            entry->destructions = dd2_profile_read_unsigned(&cursor);
            entry->retirements = dd2_profile_read_unsigned(&cursor);
        }
        for (unsigned driver = 0; driver < DD2_SAVE_PROFILE_DRIVERS; ++driver) {
            dd2_save_profile_statistics *entry = &record->drivers[driver];
            dd2_profile_read_bytes(&cursor, entry->name, sizeof(entry->name));
            entry->wins = *cursor++;
            entry->destructions = *cursor++;
            entry->retirements = dd2_profile_read_unsigned(&cursor);
        }
        dd2_profile_read_bytes(&cursor, record->standings, sizeof(record->standings));
    }
    for (unsigned driver = 0; driver < DD2_SAVE_PROFILE_DRIVERS; ++driver) {
        dd2_save_profile_driver *entry = &staged.drivers[driver];
        dd2_profile_read_bytes(&cursor, entry->name, sizeof(entry->name));
        entry->points = dd2_profile_read_signed(&cursor);
        entry->division = dd2_profile_read_signed(&cursor);
        entry->rank = dd2_profile_read_signed(&cursor);
        entry->pending_points = dd2_profile_read_signed(&cursor);
        entry->finish_place = dd2_profile_read_signed(&cursor);
        entry->race_place = dd2_profile_read_signed(&cursor);
        entry->round_points = dd2_profile_read_signed(&cursor);

        dd2_profile_read_bytes(&cursor, entry->reserved, sizeof(entry->reserved));
    }
    dd2_profile_read_bytes(&cursor, staged.players, sizeof(staged.players));
    for (unsigned circuit = 0; circuit < DD2_SAVE_PROFILE_CIRCUITS; ++circuit) {
        for (unsigned lap = 0; lap < DD2_SAVE_PROFILE_LAPS; ++lap) {
            dd2_save_profile_lap *entry = &staged.laps[circuit][lap];
            dd2_profile_read_bytes(&cursor, entry->name, sizeof(entry->name));
            entry->minutes = dd2_profile_read_unsigned(&cursor);
            entry->seconds = dd2_profile_read_unsigned(&cursor);
            entry->fraction = dd2_profile_read_unsigned(&cursor);
        }
    }
    dd2_profile_read_bytes(&cursor, staged.bindings, sizeof(staged.bindings));
    dd2_profile_read_bytes(&cursor, staged.reserved, sizeof(staged.reserved));
    if (!dd2_profile_valid(&staged)) {
        return false;
    }
    *out = staged;
    return true;
}
bool dd2_save_profile_write(const dd2_save_profile *profile, dd2_byte_buffer output) {
    if (output.data == NULL || output.size != DD2_SAVE_CARD_BLOCK_BYTES ||
        !dd2_profile_valid(profile)) {
        return false;
    }
    uint8_t staged[DD2_SAVE_CARD_BLOCK_BYTES];
    uint8_t *cursor = staged;
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.kind);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.race_mode);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.race_type);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.car);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.track);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.unlocked_circuits);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.unlocked_arenas);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.view_distance);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.effects_volume);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.controller_type);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.controller_option);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.quick_race_type);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.statistics_recorded);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.player_car);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.player);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.simultaneous_players);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.race);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.difficulty);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.races);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.players);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.cars);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.active_players);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.recording_season);
    dd2_profile_write_word(&cursor, (uint16_t)profile->header.season_number);

    for (unsigned season = 0; season < DD2_SAVE_PROFILE_SEASONS; ++season) {
        const dd2_save_profile_season *record = &profile->seasons[season];
        for (unsigned track = 0; track < DD2_SAVE_PROFILE_TRACKS; ++track) {
            const dd2_save_profile_track *entry = &record->tracks[track];
            dd2_profile_write_bytes(&cursor, entry->winner, sizeof(entry->winner));
            dd2_profile_write_word(&cursor, entry->destructions);
            dd2_profile_write_word(&cursor, entry->retirements);
        }
        for (unsigned driver = 0; driver < DD2_SAVE_PROFILE_DRIVERS; ++driver) {
            const dd2_save_profile_statistics *entry = &record->drivers[driver];
            dd2_profile_write_bytes(&cursor, entry->name, sizeof(entry->name));
            *cursor++ = entry->wins;
            *cursor++ = entry->destructions;
            dd2_profile_write_word(&cursor, entry->retirements);
        }
        dd2_profile_write_bytes(&cursor, record->standings, sizeof(record->standings));
    }
    for (unsigned driver = 0; driver < DD2_SAVE_PROFILE_DRIVERS; ++driver) {
        const dd2_save_profile_driver *entry = &profile->drivers[driver];
        dd2_profile_write_bytes(&cursor, entry->name, sizeof(entry->name));
        dd2_profile_write_word(&cursor, (uint16_t)entry->points);
        dd2_profile_write_word(&cursor, (uint16_t)entry->division);
        dd2_profile_write_word(&cursor, (uint16_t)entry->rank);
        dd2_profile_write_word(&cursor, (uint16_t)entry->pending_points);
        dd2_profile_write_word(&cursor, (uint16_t)entry->finish_place);
        dd2_profile_write_word(&cursor, (uint16_t)entry->race_place);
        dd2_profile_write_word(&cursor, (uint16_t)entry->round_points);

        dd2_profile_write_bytes(&cursor, entry->reserved, sizeof(entry->reserved));
    }
    dd2_profile_write_bytes(&cursor, profile->players, sizeof(profile->players));
    for (unsigned circuit = 0; circuit < DD2_SAVE_PROFILE_CIRCUITS; ++circuit) {
        for (unsigned lap = 0; lap < DD2_SAVE_PROFILE_LAPS; ++lap) {
            const dd2_save_profile_lap *entry = &profile->laps[circuit][lap];
            dd2_profile_write_bytes(&cursor, entry->name, sizeof(entry->name));
            dd2_profile_write_word(&cursor, entry->minutes);
            dd2_profile_write_word(&cursor, entry->seconds);
            dd2_profile_write_word(&cursor, entry->fraction);
        }
    }
    dd2_profile_write_bytes(&cursor, profile->bindings, sizeof(profile->bindings));
    dd2_profile_write_bytes(&cursor, profile->reserved, sizeof(profile->reserved));
    for (size_t index = 0; index < sizeof(staged); ++index) {
        output.data[index] = staged[index];
    }
    return true;
}
