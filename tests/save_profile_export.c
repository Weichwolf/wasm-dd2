#include "assets/bytes.h"
#include "assets/save_card.h"
#include "assets/save_profile.h"
#include "platform/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_PROFILE_EXPORT_EFFECTS = -1234,
    DD2_PROFILE_EXPORT_FRACTION = 0x8123,
    DD2_PROFILE_EXPORT_RETIREMENTS = 0x8abc,
    DD2_PROFILE_EXPORT_STANDING = 7,
    DD2_PROFILE_EXPORT_PENDING = 42,
    DD2_PROFILE_EXPORT_POINTS = 49,
    DD2_PROFILE_EXPORT_PLACEMENT = 30,
    DD2_PROFILE_EXPORT_RESERVED = 0x5a,
    DD2_PROFILE_EXPORT_SECONDS = 59,
    DD2_PROFILE_EXPORT_BINDING = 0xf5,
    DD2_PROFILE_EXPORT_SUFFIX = 0xab
};
static void dd2_profile_export_hex(const void *data, size_t size) {
    const unsigned char *bytes = data;
    printf("\"");
    for (size_t index = 0; index < size; ++index) {
        printf("%02x", (unsigned)bytes[index]);
    }
    printf("\"");
}
static void dd2_profile_export_season(const dd2_save_profile_season *record) {
    printf("{\"tracks\":[");
    for (unsigned track = 0; track < DD2_SAVE_PROFILE_TRACKS; ++track) {
        const dd2_save_profile_track *entry = &record->tracks[track];
        printf("%s[", track == 0 ? "" : ",");
        dd2_profile_export_hex(entry->winner, sizeof(entry->winner));
        printf(",%u,%u]", (unsigned)entry->destructions, (unsigned)entry->retirements);
    }
    printf("],\"drivers\":[");
    for (unsigned driver = 0; driver < DD2_SAVE_PROFILE_DRIVERS; ++driver) {
        const dd2_save_profile_statistics *entry = &record->drivers[driver];
        printf("%s[", driver == 0 ? "" : ",");
        dd2_profile_export_hex(entry->name, sizeof(entry->name));
        printf(",%u,%u,%u]", (unsigned)entry->wins, (unsigned)entry->destructions,
               (unsigned)entry->retirements);
    }
    printf("],\"standings\":[");
    for (unsigned driver = 0; driver < DD2_SAVE_PROFILE_DRIVERS; ++driver) {
        printf("%s", driver == 0 ? "" : ",");
        dd2_profile_export_hex(record->standings[driver], DD2_SAVE_PROFILE_NAME);
    }
    printf("]}");
}
static void dd2_profile_export_json(const dd2_save_profile *profile) {
    printf("{\"header\":{");
    printf("\"kind\":%d", (int)profile->header.kind);
    printf(",\"race_mode\":%d", (int)profile->header.race_mode);
    printf(",\"race_type\":%d", (int)profile->header.race_type);
    printf(",\"car\":%d", (int)profile->header.car);
    printf(",\"track\":%d", (int)profile->header.track);
    printf(",\"unlocked_circuits\":%d", (int)profile->header.unlocked_circuits);
    printf(",\"unlocked_arenas\":%d", (int)profile->header.unlocked_arenas);
    printf(",\"view_distance\":%d", (int)profile->header.view_distance);
    printf(",\"effects_volume\":%d", (int)profile->header.effects_volume);
    printf(",\"controller_type\":%d", (int)profile->header.controller_type);
    printf(",\"controller_option\":%d", (int)profile->header.controller_option);
    printf(",\"quick_race_type\":%d", (int)profile->header.quick_race_type);
    printf(",\"statistics_recorded\":%d", (int)profile->header.statistics_recorded);
    printf(",\"player_car\":%d", (int)profile->header.player_car);
    printf(",\"player\":%d", (int)profile->header.player);
    printf(",\"simultaneous_players\":%d", (int)profile->header.simultaneous_players);
    printf(",\"race\":%d", (int)profile->header.race);
    printf(",\"difficulty\":%d", (int)profile->header.difficulty);
    printf(",\"races\":%d", (int)profile->header.races);
    printf(",\"players\":%d", (int)profile->header.players);
    printf(",\"cars\":%d", (int)profile->header.cars);
    printf(",\"active_players\":%d", (int)profile->header.active_players);
    printf(",\"recording_season\":%d", (int)profile->header.recording_season);
    printf(",\"season_number\":%d", (int)profile->header.season_number);
    printf("},\"seasons\":[");
    for (unsigned season = 0; season < DD2_SAVE_PROFILE_SEASONS; ++season) {
        printf("%s", season == 0 ? "" : ",");
        dd2_profile_export_season(&profile->seasons[season]);
    }
    printf("],\"drivers\":[");
    for (unsigned driver = 0; driver < DD2_SAVE_PROFILE_DRIVERS; ++driver) {
        const dd2_save_profile_driver *entry = &profile->drivers[driver];
        printf("%s[", driver == 0 ? "" : ",");
        dd2_profile_export_hex(entry->name, sizeof(entry->name));
        printf(",%d,%d,%d,%d,%d,%d,%d,", (int)entry->points, (int)entry->division, (int)entry->rank,
               (int)entry->pending_points, (int)entry->finish_place, (int)entry->race_place,
               (int)entry->round_points);
        dd2_profile_export_hex(entry->reserved, sizeof(entry->reserved));
        printf("]");
    }
    printf("],\"players\":[");
    for (unsigned player = 0; player < DD2_SAVE_PROFILE_PLAYERS; ++player) {
        printf("%s", player == 0 ? "" : ",");
        dd2_profile_export_hex(profile->players[player], DD2_SAVE_PROFILE_PLAYER_NAME);
    }
    printf("],\"laps\":[");
    for (unsigned circuit = 0; circuit < DD2_SAVE_PROFILE_CIRCUITS; ++circuit) {
        printf("%s[", circuit == 0 ? "" : ",");
        for (unsigned lap = 0; lap < DD2_SAVE_PROFILE_LAPS; ++lap) {
            const dd2_save_profile_lap *entry = &profile->laps[circuit][lap];
            printf("%s[", lap == 0 ? "" : ",");
            dd2_profile_export_hex(entry->name, sizeof(entry->name));
            printf(",%u,%u,%u]", (unsigned)entry->minutes, (unsigned)entry->seconds,
                   (unsigned)entry->fraction);
        }
        printf("]");
    }
    printf("],\"bindings\":");
    dd2_profile_export_hex(profile->bindings, sizeof(profile->bindings));
    printf(",\"reserved\":");
    dd2_profile_export_hex(profile->reserved, sizeof(profile->reserved));
    puts("}");
}
static void dd2_profile_export_edit(dd2_save_profile *profile) {
    profile->header.effects_volume = DD2_PROFILE_EXPORT_EFFECTS;
    dd2_save_profile_track *track = &profile->seasons[4].tracks[DD2_SAVE_PROFILE_TRACKS - 1];
    track->winner[0] = 'Q';
    track->destructions = UINT16_MAX;
    track->retirements = DD2_PROFILE_EXPORT_FRACTION;
    dd2_save_profile_statistics *statistics =
        &profile->seasons[3].drivers[DD2_SAVE_PROFILE_DRIVERS - 1];
    statistics->wins = UINT8_MAX;
    statistics->destructions = UINT8_MAX - 1;
    statistics->retirements = DD2_PROFILE_EXPORT_RETIREMENTS;
    profile->seasons[2].standings[DD2_PROFILE_EXPORT_STANDING][0] = 'S';
    dd2_save_profile_driver *driver = &profile->drivers[DD2_SAVE_PROFILE_DRIVERS - 1];
    driver->points = 1 - DD2_SAVE_PROFILE_DRIVERS;
    driver->division = 3;
    driver->rank = 4;
    driver->pending_points = DD2_PROFILE_EXPORT_PENDING;
    driver->finish_place = DD2_PROFILE_EXPORT_POINTS;
    driver->race_place = DD2_SAVE_PROFILE_DRIVERS;
    driver->round_points = DD2_PROFILE_EXPORT_PLACEMENT;
    driver->reserved[0] = DD2_PROFILE_EXPORT_RESERVED;
    profile->players[DD2_SAVE_PROFILE_PLAYERS - 1][0] = 'P';
    dd2_save_profile_lap *lap =
        &profile->laps[DD2_SAVE_PROFILE_CIRCUITS - 1][DD2_SAVE_PROFILE_LAPS - 1];
    lap->name[0] = 'L';
    lap->minutes = 2;
    lap->seconds = DD2_PROFILE_EXPORT_SECONDS;
    lap->fraction = DD2_PROFILE_EXPORT_FRACTION;
    profile->bindings[DD2_SAVE_PROFILE_BINDINGS - 1] = DD2_PROFILE_EXPORT_BINDING;
    profile->reserved[sizeof(profile->reserved) - 1] = DD2_PROFILE_EXPORT_SUFFIX;
}
static bool dd2_profile_export_run(char **argv) {
    dd2_file file = {0};
    dd2_save_profile profile = {0};
    uint8_t output[DD2_SAVE_CARD_BLOCK_BYTES];
    bool valid =
        dd2_file_read(argv[1], &file) &&
        dd2_save_profile_read((dd2_byte_view){.data = file.data, .size = file.size}, &profile);
    if (valid && strcmp(argv[3], "edit") == 0) {
        dd2_profile_export_edit(&profile);
    } else if (valid && strcmp(argv[3], "inspect") != 0) {
        valid = false;
    }
    valid = valid && dd2_save_profile_write(
                         &profile, (dd2_byte_buffer){.data = output, .size = sizeof(output)});
    if (valid) {
        FILE *written = fopen(argv[2], "wb");
        valid = written != NULL;
        if (written != NULL) {
            valid = fwrite(output, 1, sizeof(output), written) == sizeof(output);
            const int closed = fclose(written);
            valid = valid && closed == 0;
        }
    }
    if (valid) {
        dd2_profile_export_json(&profile);
    }
    dd2_file_release(&file);
    return valid;
}
int main(int argc, char **argv) {
    return argc == 4 && dd2_profile_export_run(argv) ? EXIT_SUCCESS : EXIT_FAILURE;
}
