#include "assets/bytes.h"
#include "assets/save_card.h"
#include "assets/track.h"
#include "game/application.h"
#include "game/championship.h"
#include "game/driving.h"
#include "game/league.h"
#include "game/race.h"
#include "platform/save_store.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_CONFIG_APP_EFFECTS_A = 128,
    DD2_CONFIG_APP_MUSIC_A = 64,
    DD2_CONFIG_APP_EFFECTS_B = 32,
    DD2_CONFIG_APP_MUSIC_B = 16,
    DD2_CONFIG_APP_ENGINE_LEVEL_A = 40,
    DD2_CONFIG_APP_ENGINE_LEVEL_B = 10,
    DD2_CONFIG_APP_ARGUMENTS = 6
};
static const char dd2_config_app_tmp[] = "/tmp/wasm-dd2/";
static const double dd2_config_app_step = 0.005;
static void dd2_config_app_require(bool valid, const char *message) {
    if (!valid) {
        if (fputs(message, stderr) == EOF) {
            abort();
        }
        exit(EXIT_FAILURE);
    }
}
static void dd2_config_app_gains(unsigned effects, unsigned music) {
    dd2_config_app_require(
        dd2_application_set_effects_gain(effects) && dd2_application_set_music_gain(music) &&
            dd2_application_effects_gain() == effects && dd2_application_music_gain() == music,
        "Live preference gain update failed\n");
}
static void dd2_config_app_self_test(void) {
    dd2_config_app_gains(DD2_CONFIG_APP_EFFECTS_A, DD2_CONFIG_APP_MUSIC_A);
    const dd2_save_card *previous = dd2_application_saves_view();
    dd2_config_app_require(dd2_application_save_preferences(0, "A") &&
                               dd2_application_saves_view() == previous,
                           "Save published before application acknowledgement\n");
    dd2_config_app_gains(DD2_CONFIG_APP_EFFECTS_B, DD2_CONFIG_APP_MUSIC_B);
    dd2_config_app_require(dd2_application_saves_poll() == DD2_SAVE_STORE_OK &&
                               dd2_application_save_preferences(1, "B") &&
                               dd2_application_saves_poll() == DD2_SAVE_STORE_OK,
                           "Two real application saves failed\n");
    dd2_config_app_require(!dd2_application_save_preferences(2, "B") &&
                               dd2_application_saves_poll() == DD2_SAVE_STORE_DUPLICATE,
                           "Application duplicate name accepted\n");
    dd2_config_app_require(
        dd2_application_set_driving(1) &&
            dd2_application_advance((dd2_driving_frame){.seconds = dd2_config_app_step}) &&
            dd2_application_load_preferences(0) &&
            dd2_application_music_gain() == DD2_CONFIG_APP_MUSIC_A &&
            dd2_application_effects_gain() == DD2_CONFIG_APP_EFFECTS_A &&
            dd2_application_effect_voice(DD2_EFFECT_QUERY_GAIN) == DD2_CONFIG_APP_ENGINE_LEVEL_A,
        "Load did not change real music/engine gain\n");
    dd2_config_app_require(dd2_application_start_championship(DD2_RACE_STOCKCAR),
                           "Cannot prepare live championship for preference load\n");
    const dd2_championship before = *dd2_application_championship_view();
    const dd2_driving *field = dd2_application_driving_view();
    dd2_config_app_gains(DD2_CONFIG_APP_EFFECTS_B, DD2_CONFIG_APP_MUSIC_B);
    dd2_config_app_require(dd2_application_load_preferences(0), "Audio load rejected\n");
    const dd2_championship *after = dd2_application_championship_view();
    dd2_config_app_require(after->phase == before.phase && after->ticket == before.ticket &&
                               after->mode == before.mode && after->circuits == before.circuits &&
                               after->arenas == before.arenas &&
                               after->history_count == before.history_count &&
                               dd2_application_driving_view() == field,
                           "Audio load changed active championship state\n");
    for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        dd2_config_app_require(
            after->league.drivers[driver].points == before.league.drivers[driver].points &&
                after->league.drivers[driver].division == before.league.drivers[driver].division &&
                after->league.drivers[driver].rank == before.league.drivers[driver].rank,
            "Audio load changed active championship league\n");
    }
    dd2_config_app_require(
        dd2_application_delete_save(0) && dd2_application_saves_poll() == DD2_SAVE_STORE_OK &&
            strcmp(dd2_application_save_name(0), "B") == 0 && dd2_application_load_preferences(0) &&
            dd2_application_music_gain() == DD2_CONFIG_APP_MUSIC_B &&
            dd2_application_effects_gain() == DD2_CONFIG_APP_EFFECTS_B,
        "Compacted entry did not load its selected payload\n");
    dd2_config_app_require(
        dd2_application_exit_championship() && dd2_application_select_level(2) &&
            dd2_application_show_car(1) && dd2_application_show_car(0) &&
            dd2_application_saves_count() == 1 && dd2_application_set_driving(1) &&
            dd2_application_advance((dd2_driving_frame){.seconds = dd2_config_app_step}) &&
            dd2_application_effect_voice(DD2_EFFECT_QUERY_GAIN) == DD2_CONFIG_APP_ENGINE_LEVEL_B &&
            dd2_application_reload_saves() && dd2_application_saves_poll() == DD2_SAVE_STORE_OK &&
            dd2_application_present(),
        "Reload/presentation failed\n");
}
int main(int argc, char **argv) {
    dd2_config_app_require(
        argc == DD2_CONFIG_APP_ARGUMENTS &&
            strncmp(argv[2], dd2_config_app_tmp, sizeof(dd2_config_app_tmp) - 1) == 0 &&
            strncmp(argv[3], dd2_config_app_tmp, sizeof(dd2_config_app_tmp) - 1) == 0,
        "Use Dirinfo, dedicated tmp directory, dump, operation and logical index\n");
    const unsigned logical = strcmp(argv[5], "1") == 0 ? 1U : 0U;
    dd2_config_app_require(dd2_application_open(argv[1], 1) &&
                               dd2_application_saves_open(argv[2]) &&
                               dd2_application_saves_poll() == DD2_SAVE_STORE_OK,
                           "Actual application/storage cannot open\n");
    int loaded = 1;
    if (strcmp(argv[4], "self-test") == 0) {
        dd2_config_app_self_test();
    } else {
        if (strcmp(argv[4], "restore") != 0) {
            dd2_config_app_gains(DD2_CONFIG_APP_EFFECTS_B, DD2_CONFIG_APP_MUSIC_B);
        }
        const dd2_track *track = dd2_application_track_view();
        const dd2_driving *driving = dd2_application_driving_view();
        loaded = dd2_application_load_preferences(logical);
        dd2_config_app_require(dd2_application_track_view() == track &&
                                   dd2_application_driving_view() == driving,
                               "Audio import replaced owned game resources\n");
        if (strcmp(argv[4], "load-save") == 0 && loaded != 0) {
            dd2_config_app_require(dd2_application_save_preferences(logical, "EDIT") &&
                                       dd2_application_saves_poll() == DD2_SAVE_STORE_OK,
                                   "Imported preference edit could not save\n");
        }
    }
    const dd2_byte_view bytes = dd2_save_card_image(dd2_application_saves_view());
    FILE *output = fopen(argv[3], "wb");
    dd2_config_app_require(output != NULL, "Application image cannot open\n");
    const bool written = fwrite(bytes.data, 1, bytes.size, output) == bytes.size;
    const bool closed = fclose(output) == 0;
    dd2_config_app_require(written && closed, "Application image incomplete\n");
    dd2_config_app_require(printf("{\"loaded\":%d,\"effects\":%u,\"music\":%u,\"count\":%u}\n",
                                  loaded, dd2_application_effects_gain(),
                                  dd2_application_music_gain(), dd2_application_saves_count()) > 0,
                           "Application report failed\n");
    dd2_config_app_require(dd2_application_close() && dd2_application_current_level() == 0,
                           "Application resources did not close\n");
    dd2_config_app_require(dd2_application_saves_poll() == DD2_SAVE_STORE_OK,
                           "Close erased the completed storage receipt\n");
    return EXIT_SUCCESS;
}
