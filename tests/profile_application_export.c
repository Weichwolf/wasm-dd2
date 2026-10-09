#include "assets/bytes.h"
#include "assets/save_card.h"
#include "game/application.h"
#include "game/championship.h"
#include "game/driving.h"
#include "game/league.h"
#include "game/race.h"
#include "platform/save_store.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_PROFILE_APP_EFFECTS_A = 128,
    DD2_PROFILE_APP_MUSIC_A = 64,
    DD2_PROFILE_APP_EFFECTS_B = 32,
    DD2_PROFILE_APP_MUSIC_B = 16,
    DD2_PROFILE_APP_DRIVER_BYTES = 16,
    DD2_PROFILE_APP_ARGUMENTS = 7,
    DD2_PROFILE_APP_ACTION = 5,
    DD2_PROFILE_APP_LOGICAL = 6
};
static const double dd2_profile_app_step = 0.005;
static void dd2_profile_app_require(bool good, const char *message) {
    if (!good) {
        if (fputs(message, stderr) == EOF) {
            abort();
        }
        exit(EXIT_FAILURE);
    }
}
static void dd2_profile_app_gains(unsigned effects, unsigned music) {
    dd2_profile_app_require(dd2_application_set_effects_gain(effects) &&
                                dd2_application_set_music_gain(music),
                            "Cannot set audio\n");
}
static void dd2_profile_app_self_test(void) {
    dd2_profile_app_require(dd2_application_set_player_name("Racer_7!"), "Cannot set player\n");
    dd2_profile_app_gains(DD2_PROFILE_APP_EFFECTS_A, DD2_PROFILE_APP_MUSIC_A);
    const dd2_save_card *before = dd2_application_saves_view();
    dd2_profile_app_require(dd2_application_save_profile(0, "A") &&
                                dd2_application_saves_view() == before &&
                                dd2_application_set_player_name("LOCAL"),
                            "Early publication or name edit failure\n");
    dd2_profile_app_gains(DD2_PROFILE_APP_EFFECTS_B, DD2_PROFILE_APP_MUSIC_B);
    dd2_profile_app_require(dd2_application_saves_poll() == DD2_SAVE_STORE_OK &&
                                dd2_application_load_preferences(0) &&
                                strcmp(dd2_application_player_name(), "LOCAL") == 0 &&
                                dd2_application_music_gain() == DD2_PROFILE_APP_MUSIC_A,
                            "Audio-only load changed identity\n");
    dd2_profile_app_require(
        dd2_application_load_profile(0) && strcmp(dd2_application_player_name(), "Racer_7!") == 0 &&
            dd2_application_set_driving(1) &&
            dd2_application_advance((dd2_driving_frame){.seconds = dd2_profile_app_step}),
        "Profile did not restore live driving identity\n");
    const dd2_driving *field = dd2_application_driving_view();
    dd2_profile_app_require(!dd2_application_set_player_name("123456789") &&
                                !dd2_application_set_player_name("Bad\n") &&
                                !dd2_application_set_player_name("\xc3\xa4") &&
                                strcmp(dd2_application_player_name(), "Racer_7!") == 0 &&
                                dd2_application_driving_view() == field,
                            "Invalid player changed live state\n");
    dd2_profile_app_require(dd2_application_start_championship(DD2_RACE_STOCKCAR),
                            "Cannot start championship\n");
    const dd2_championship championship = *dd2_application_championship_view();
    field = dd2_application_driving_view();
    dd2_profile_app_require(
        !dd2_application_set_player_name("OTHER") && !dd2_application_load_profile(0) &&
            dd2_application_load_preferences(0) &&
            strcmp(dd2_application_driver_name(0), "Racer_7!") == 0 &&
            dd2_application_driving_view() == field &&
            dd2_application_championship_view()->ticket == championship.ticket &&
            dd2_application_championship_view()->phase == championship.phase &&
            dd2_application_present(),
        "Active championship identity changed\n");
    dd2_profile_app_require(dd2_application_exit_championship() &&
                                dd2_application_select_level(2) &&
                                dd2_application_set_player_name("") &&
                                strcmp(dd2_application_player_name(), "PLAYER") == 0,
                            "Navigation/default identity failed\n");
    dd2_profile_app_gains(DD2_PROFILE_APP_EFFECTS_B, DD2_PROFILE_APP_MUSIC_B);
    dd2_profile_app_require(dd2_application_save_profile(1, "B") &&
                                dd2_application_saves_poll() == DD2_SAVE_STORE_OK &&
                                dd2_application_load_profile(0),
                            "Two profile saves failed\n");
}
static void dd2_profile_app_snapshot(const char *path) {
    const dd2_byte_view image = dd2_save_card_image(dd2_application_saves_view());
    FILE *output = fopen(path, "wb");
    dd2_profile_app_require(output != NULL, "Cannot open card output\n");
    const bool written = fwrite(image.data, 1, image.size, output) == image.size;
    const bool closed = fclose(output) == 0;
    dd2_profile_app_require(written && closed, "Cannot write card\n");
}
static void dd2_profile_app_roster(const char *path) {
    FILE *output = fopen(path, "wb");
    dd2_profile_app_require(output != NULL, "Cannot open roster output\n");
    for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        char record[DD2_PROFILE_APP_DRIVER_BYTES] = {0};
        const char *name = dd2_application_driver_name(driver);
        for (unsigned index = 0; index + 1 < sizeof(record) && name[index] != '\0'; ++index) {
            record[index] = name[index];
        }
        dd2_profile_app_require(fwrite(record, 1, sizeof(record), output) == sizeof(record),
                                "Cannot write roster\n");
    }
    dd2_profile_app_require(fclose(output) == 0, "Cannot close roster\n");
}
int main(int argc, char **argv) {
    dd2_profile_app_require(argc == DD2_PROFILE_APP_ARGUMENTS,
                            "archive/directory/card/roster/action/logical\n");
    dd2_profile_app_require(dd2_application_open(argv[1], 1) &&
                                dd2_application_saves_open(argv[2]) &&
                                dd2_application_saves_poll() == DD2_SAVE_STORE_OK,
                            "Cannot open actual profile application\n");
    const unsigned logical = argv[DD2_PROFILE_APP_LOGICAL][0] == '1' ? 1U : 0U;
    bool loaded = false;
    if (strcmp(argv[DD2_PROFILE_APP_ACTION], "self-test") == 0) {
        dd2_profile_app_self_test();
        loaded = true;
    } else {
        dd2_profile_app_require(dd2_application_set_player_name("LOCAL"),
                                "Cannot set baseline identity\n");
        dd2_profile_app_gains(DD2_PROFILE_APP_EFFECTS_B, DD2_PROFILE_APP_MUSIC_B);
        loaded = dd2_application_load_profile(logical) != 0;
        if (loaded && strcmp(argv[DD2_PROFILE_APP_ACTION], "load-save") == 0) {
            dd2_profile_app_require(dd2_application_save_profile(logical, "EDIT") &&
                                        dd2_application_saves_poll() == DD2_SAVE_STORE_OK,
                                    "Cannot save imported profile\n");
        }
    }
    dd2_profile_app_snapshot(argv[3]);
    dd2_profile_app_roster(argv[4]);
    const int printed = printf("{\"loaded\":%u,\"effects\":%u,\"music\":%u}\n", (unsigned)loaded,
                               dd2_application_effects_gain(), dd2_application_music_gain());
    dd2_profile_app_require(printed > 0 && dd2_application_close() &&
                                dd2_application_saves_poll() == DD2_SAVE_STORE_OK,
                            "Close failed or erased completion\n");
    return EXIT_SUCCESS;
}
