#include "assets/bytes.h"
#include "assets/car_class.h"
#include "assets/save_card.h"
#include "game/application.h"
#include "game/championship.h"
#include "game/driving.h"
#include "game/league.h"
#include "game/race.h"
#include "physics/vehicle.h"
#include "platform/save_store.h"

#include <stdatomic.h>
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
/* Link-only allocation failure seam; production allocation is unchanged. */
static atomic_bool dd2_profile_app_fail_allocation;
void *dd2_profile_app_real_calloc(size_t count, size_t size) __asm__("__real_calloc");
void *dd2_profile_app_fault_calloc(size_t count, size_t size) __asm__("__wrap_calloc");
void *dd2_profile_app_fault_calloc(size_t count, size_t size) {
    if (atomic_exchange(&dd2_profile_app_fail_allocation, false)) {
        return NULL;
    }
    return dd2_profile_app_real_calloc(count, size);
}
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
static void dd2_profile_app_field(dd2_car_class car_class) {
    const dd2_driving *field = dd2_application_driving_view();
    dd2_profile_app_require(dd2_application_current_car() == (int)car_class &&
                                dd2_driving_class(field, 0) == car_class,
                            "Profile class and physical owner differ\n");
    for (unsigned driver = 1; driver < dd2_driving_vehicle_count(field); ++driver) {
        dd2_profile_app_require(dd2_driving_class(field, driver) == DD2_CAR_PRO,
                                "Profile changed NPC handling\n");
    }
}
static void dd2_profile_app_motion(const dd2_driving *field, dd2_vehicle before) {
    const dd2_vehicle *after = dd2_driving_vehicle(dd2_application_driving_view());
    dd2_profile_app_require(
        dd2_application_driving_view() == field && after->steps == before.steps &&
            after->position.x == before.position.x && after->position.y == before.position.y &&
            after->position.z == before.position.z && after->velocity.x == before.velocity.x &&
            after->velocity.y == before.velocity.y && after->velocity.z == before.velocity.z &&
            after->rotation.x == before.rotation.x && after->rotation.y == before.rotation.y &&
            after->rotation.z == before.rotation.z && after->rotation.w == before.rotation.w &&
            after->angular_velocity.x == before.angular_velocity.x &&
            after->angular_velocity.y == before.angular_velocity.y &&
            after->angular_velocity.z == before.angular_velocity.z &&
            after->steering == before.steering,
        "Profile replaced or moved active field\n");
}
static void dd2_profile_app_car_test(void) {
    const char *const names[DD2_CAR_CLASSES] = {"C0", "C1", "C2"};
    dd2_profile_app_gains(DD2_PROFILE_APP_EFFECTS_A, DD2_PROFILE_APP_MUSIC_A);
    for (unsigned index = 0; index < DD2_CAR_CLASSES; ++index) {
        dd2_profile_app_require(dd2_application_select_car((int)index) &&
                                    dd2_application_set_player_name(names[index]) &&
                                    dd2_application_save_profile(index, names[index]) &&
                                    dd2_application_saves_poll() == DD2_SAVE_STORE_OK,
                                "Cannot persist car classes\n");
    }
    for (unsigned index = 0; index < DD2_CAR_CLASSES; ++index) {
        dd2_profile_app_require(dd2_application_load_profile(index) &&
                                    strcmp(dd2_application_player_name(), names[index]) == 0,
                                "Cannot restore car class\n");
        dd2_profile_app_field((dd2_car_class)index);
    }
    /* Different-class candidate failure must precede every live publication. */
    dd2_profile_app_require(dd2_application_set_player_name("LOCAL"), "Cannot edit player\n");
    dd2_profile_app_gains(DD2_PROFILE_APP_EFFECTS_B, DD2_PROFILE_APP_MUSIC_B);
    const dd2_driving *field = dd2_application_driving_view();
    const dd2_vehicle before = *dd2_driving_vehicle(field);
    atomic_store(&dd2_profile_app_fail_allocation, true);
    dd2_profile_app_require(!dd2_application_load_profile(0) &&
                                !atomic_load(&dd2_profile_app_fail_allocation) &&
                                strcmp(dd2_application_player_name(), "LOCAL") == 0 &&
                                dd2_application_effects_gain() == DD2_PROFILE_APP_EFFECTS_B &&
                                dd2_application_music_gain() == DD2_PROFILE_APP_MUSIC_B,
                            "Failed candidate published profile state\n");
    dd2_profile_app_field(DD2_CAR_PRO);
    dd2_profile_app_motion(field, before);
    dd2_profile_app_require(dd2_application_load_profile(0) && dd2_application_set_driving(1) &&
                                dd2_application_advance((dd2_driving_frame){
                                    .seconds = dd2_profile_app_step, .control.throttle = 1.0}),
                            "Cannot drive restored class\n");
    field = dd2_application_driving_view();
    const dd2_vehicle moving = *dd2_driving_vehicle(field);
    dd2_profile_app_require(dd2_application_set_player_name("LOCAL"), "Cannot edit player\n");
    dd2_profile_app_gains(DD2_PROFILE_APP_EFFECTS_B, DD2_PROFILE_APP_MUSIC_B);
    dd2_profile_app_require(!dd2_application_load_profile(1) &&
                                strcmp(dd2_application_player_name(), "LOCAL") == 0 &&
                                dd2_application_effects_gain() == DD2_PROFILE_APP_EFFECTS_B &&
                                dd2_application_music_gain() == DD2_PROFILE_APP_MUSIC_B,
                            "Different-class load changed active play\n");
    dd2_profile_app_field(DD2_CAR_ROOKIE);
    dd2_profile_app_motion(field, moving);
    dd2_profile_app_require(dd2_application_load_preferences(2) &&
                                strcmp(dd2_application_player_name(), "LOCAL") == 0 &&
                                dd2_application_load_profile(0) &&
                                strcmp(dd2_application_player_name(), "C0") == 0,
                            "Audio-only/same-class restore failed during play\n");
    dd2_profile_app_field(DD2_CAR_ROOKIE);
    dd2_profile_app_motion(field, moving);
    dd2_profile_app_require(dd2_application_set_driving(0) &&
                                dd2_application_start_championship(DD2_RACE_STOCKCAR),
                            "Cannot start selected-class season\n");
    field = dd2_application_driving_view();
    dd2_profile_app_require(
        !dd2_application_load_profile(2) && dd2_application_load_preferences(2) &&
            dd2_application_driving_view() == field && dd2_application_exit_championship() &&
            dd2_application_load_profile(0),
        "Profile replaced championship class\n");
    dd2_profile_app_field(DD2_CAR_ROOKIE);
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
    dd2_profile_app_require(argv[DD2_PROFILE_APP_LOGICAL][0] >= '0' &&
                                argv[DD2_PROFILE_APP_LOGICAL][0] <= '2',
                            "Invalid fixture logical slot\n");
    const unsigned logical = (unsigned)(argv[DD2_PROFILE_APP_LOGICAL][0] - '0');
    bool loaded = false;
    if (strcmp(argv[DD2_PROFILE_APP_ACTION], "self-test") == 0) {
        dd2_profile_app_self_test();
        loaded = true;
    } else if (strcmp(argv[DD2_PROFILE_APP_ACTION], "car-test") == 0) {
        dd2_profile_app_car_test();
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
    const int printed = printf("{\"loaded\":%u,\"effects\":%u,\"music\":%u,\"car\":%d}\n",
                               (unsigned)loaded, dd2_application_effects_gain(),
                               dd2_application_music_gain(), dd2_application_current_car());
    dd2_profile_app_require(printed > 0 && dd2_application_close() &&
                                dd2_application_saves_poll() == DD2_SAVE_STORE_OK,
                            "Close failed or erased completion\n");
    return EXIT_SUCCESS;
}
