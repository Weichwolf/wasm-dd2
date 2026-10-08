#ifndef DD2_ASSETS_SAVE_PROFILE_H
#define DD2_ASSETS_SAVE_PROFILE_H

#include "assets/bytes.h"
#include "assets/save_card.h"

#include <stdbool.h>
#include <stdint.h>

enum {
    DD2_SAVE_PROFILE_CONFIG = 0x1010,
    DD2_SAVE_PROFILE_STARTUP = 0x1020,
    DD2_SAVE_PROFILE_GAME = 0x3030,
    DD2_SAVE_PROFILE_BYTES = 6526,
    DD2_SAVE_PROFILE_SEASONS = 5,
    DD2_SAVE_PROFILE_TRACKS = 11,
    DD2_SAVE_PROFILE_DRIVERS = 20,
    DD2_SAVE_PROFILE_PLAYERS = 10,
    DD2_SAVE_PROFILE_CIRCUITS = 7,
    DD2_SAVE_PROFILE_LAPS = 5,
    DD2_SAVE_PROFILE_BINDINGS = 18,
    DD2_SAVE_PROFILE_NAME = 16,
    DD2_SAVE_PROFILE_PLAYER_NAME = 12,
    DD2_SAVE_PROFILE_LAP_NAME = 10,
    DD2_SAVE_PROFILE_DRIVER_RESERVED = 24
};

/* Original signed header words. Mode/type and controller IDs remain source IDs;
 * applying them to a playable rewrite session requires explicit translation and
 * validation. The original block contains effects gain, but no Redbook gain. */
typedef struct {
    int16_t kind;
    int16_t race_mode;
    int16_t race_type;
    int16_t car;
    int16_t track;
    int16_t unlocked_circuits;
    int16_t unlocked_arenas;
    int16_t view_distance;
    int16_t effects_volume;
    int16_t controller_type;
    int16_t controller_option;
    int16_t quick_race_type;
    int16_t statistics_recorded;
    int16_t player_car;
    int16_t player;
    int16_t simultaneous_players;
    int16_t race;
    int16_t difficulty;
    int16_t races;
    int16_t players;
    int16_t cars;
    int16_t active_players;
    int16_t recording_season;
    int16_t season_number;
} dd2_save_profile_header;

typedef struct {
    char winner[DD2_SAVE_PROFILE_NAME];
    uint16_t destructions;
    uint16_t retirements;
} dd2_save_profile_track;

typedef struct {
    char name[DD2_SAVE_PROFILE_NAME];
    uint8_t wins;
    uint8_t destructions;
    uint16_t retirements;
} dd2_save_profile_statistics;

typedef struct {
    dd2_save_profile_track tracks[DD2_SAVE_PROFILE_TRACKS];
    dd2_save_profile_statistics drivers[DD2_SAVE_PROFILE_DRIVERS];
    char standings[DD2_SAVE_PROFILE_DRIVERS][DD2_SAVE_PROFILE_NAME];
} dd2_save_profile_season;

typedef struct {
    char name[DD2_SAVE_PROFILE_NAME];
    int16_t points;
    int16_t division;
    int16_t rank;
    int16_t pending_points;
    int16_t finish_place;
    int16_t race_place;
    int16_t round_points;
    uint8_t reserved[DD2_SAVE_PROFILE_DRIVER_RESERVED];
} dd2_save_profile_driver;

typedef struct {
    char name[DD2_SAVE_PROFILE_LAP_NAME];
    uint16_t minutes;
    uint16_t seconds;
    uint16_t fraction; /* Original 16-bit fractional second, not centiseconds. */
} dd2_save_profile_lap;

/* Value owner: no file views, allocations or original addresses. Fixed text is
 * terminated within its field; bytes following the terminator are retained.
 * Reserved driver data and the unused block suffix survive edits. */
typedef struct {
    dd2_save_profile_header header;
    dd2_save_profile_season seasons[DD2_SAVE_PROFILE_SEASONS];
    dd2_save_profile_driver drivers[DD2_SAVE_PROFILE_DRIVERS];
    char players[DD2_SAVE_PROFILE_PLAYERS][DD2_SAVE_PROFILE_PLAYER_NAME];
    dd2_save_profile_lap laps[DD2_SAVE_PROFILE_CIRCUITS][DD2_SAVE_PROFILE_LAPS];
    uint8_t bindings[DD2_SAVE_PROFILE_BINDINGS];
    uint8_t reserved[DD2_SAVE_CARD_BLOCK_BYTES - DD2_SAVE_PROFILE_BYTES];
} dd2_save_profile;

/* Exact full physical payload only. CONFIG/STARTUP/GAME share this layout;
 * replay magic 0x2020 does not. Invalid input leaves out/output unchanged.
 * Numeric fields are lossless: parsing never makes corrupt gameplay indices
 * safe to consume. Read/encode stage aliases before publishing success. */
bool dd2_save_profile_read(dd2_byte_view bytes, dd2_save_profile *out);
bool dd2_save_profile_write(const dd2_save_profile *profile, dd2_byte_buffer output);

#endif
