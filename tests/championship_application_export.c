#include "ai/driver.h"
#include "assets/road.h"
#include "assets/track.h"
#include "game/application.h"
#include "game/championship.h"
#include "game/driving.h"
#include "game/league.h"
#include "game/race.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_CHAMP_APP_BOUND = 300000,
    DD2_CHAMP_APP_BATCH = 1000,
    DD2_CHAMP_APP_CHECKPOINT = 5000,
    DD2_CHAMP_APP_CHANNELS = 4,
    DD2_CHAMP_APP_PATH_BYTES = 512,
    DD2_CHAMP_APP_ARGUMENTS = 5,
    DD2_CHAMP_APP_PRACTICE = 8,
    DD2_CHAMP_APP_WIDTH = 640,
    DD2_CHAMP_APP_HEIGHT = 480
};

typedef struct {
    dd2_road_surface *surface;
    dd2_ai_driver pilot;
    char archive[DD2_CHAMP_APP_PATH_BYTES];
    char output[DD2_CHAMP_APP_PATH_BYTES];
    dd2_race_mode mode;
    unsigned tick;
    bool missing;
    bool ready;
    bool failed;
} dd2_champ_app_export;

static dd2_champ_app_export dd2_champ_app;
static const double dd2_champ_app_hold_seconds = 0.1;

/* Browser harness drives this executable's normal application library in
 * bounded batches. Only ordinary analog controls reach production gameplay. */
int dd2_champ_app_step(void);
int dd2_champ_app_finish(void);

static bool dd2_champ_app_capture(const char *name) {
    if (!dd2_application_present()) {
        return false;
    }
    const dd2_application_image image = dd2_application_image_view();
    char path[DD2_CHAMP_APP_PATH_BYTES] = {0};
    const char *const parts[] = {dd2_champ_app.output, "/", name, ".ppm"};
    size_t length = 0;
    for (unsigned part = 0; part < sizeof(parts) / sizeof(parts[0]); ++part) {
        for (size_t byte = 0; parts[part][byte] != '\0'; ++byte) {
            if (length >= sizeof(path) - 1) {
                return false;
            }
            path[length++] = parts[part][byte];
        }
    }
    if (image.pixels == NULL || image.viewport.width != DD2_CHAMP_APP_WIDTH ||
        image.viewport.height != DD2_CHAMP_APP_HEIGHT) {
        return false;
    }
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    const char header[] = "P6\n640 480\n255\n";
    bool valid = fwrite(header, 1, sizeof(header) - 1, file) == sizeof(header) - 1;
    for (int row = image.viewport.height - 1; row >= 0 && valid; --row) {
        for (int column = 0; column < image.viewport.width && valid; ++column) {
            const size_t offset = (((size_t)row * (size_t)image.viewport.width) + (size_t)column) *
                                  DD2_CHAMP_APP_CHANNELS;
            valid = fwrite(image.pixels + offset, 1, 3, file) == 3;
        }
    }
    return fclose(file) == 0 && valid;
}

static bool dd2_champ_app_entry(void) {
    if (!dd2_application_open(dd2_champ_app.archive, DD2_CHAMP_APP_PRACTICE) ||
        dd2_application_open(dd2_champ_app.archive, 1) ||
        dd2_application_start_championship(DD2_RACE_TIME_TRIAL) ||
        !dd2_application_start_championship(DD2_RACE_WRECKING)) {
        return false;
    }
    const dd2_driving *driving = dd2_application_driving_view();
    const dd2_track *track = dd2_application_track_view();
    if (dd2_application_start_race(DD2_RACE_TOTAL_DESTRUCTION) ||
        dd2_application_continue_championship() || dd2_application_select_level(0) ||
        driving != dd2_application_driving_view() || track != dd2_application_track_view() ||
        dd2_application_advance((dd2_driving_frame){.seconds = -1}) ||
        !dd2_application_advance((dd2_driving_frame){.seconds = DD2_VEHICLE_STEP_SECONDS}) ||
        !dd2_application_set_paused(1)) {
        return false;
    }
    const unsigned steps = dd2_application_race_steps();
    if (!dd2_application_advance((dd2_driving_frame){.seconds = dd2_champ_app_hold_seconds}) ||
        dd2_application_race_steps() != steps ||
        dd2_application_advance((dd2_driving_frame){.seconds = -1}) ||
        !dd2_application_restart_championship() || dd2_application_race_steps() != 0 ||
        dd2_application_championship_points(0) != 0 || !dd2_application_exit_championship() ||
        dd2_application_current_level() != DD2_CHAMP_APP_PRACTICE ||
        dd2_application_championship_phase() != -1 || dd2_application_current_view() != 0 ||
        !dd2_application_start_championship((int)dd2_champ_app.mode)) {
        return false;
    }
    return dd2_application_current_level() == 1 &&
           dd2_application_championship_phase() == DD2_CHAMPIONSHIP_RACING &&
           dd2_application_championship_division() == DD2_LEAGUE_DIVISIONS &&
           dd2_application_championship_season() == 1 &&
           dd2_application_championship_round() == 1 && dd2_champ_app_capture("start");
}

static bool dd2_champ_app_results(void) {
    const dd2_race *race = dd2_driving_race(dd2_application_driving_view());
    const dd2_championship *state = dd2_application_championship_view();
    if (race == NULL || state == NULL || race->phase != DD2_RACE_RESULTS ||
        race->end == DD2_RACE_WITHDRAWN || state->phase != DD2_CHAMPIONSHIP_ROUND_RESULTS ||
        dd2_championship_current(state)->completed != 1) {
        return false;
    }
    printf("{\"kind\":\"result\",\"mode\":%u,\"steps\":%llu,\"elapsed\":%llu,"
           "\"end\":%u,\"drivers\":[",
           (unsigned)race->rules.mode, (unsigned long long)race->steps,
           (unsigned long long)race->elapsed, (unsigned)race->end);
    for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        const dd2_race_driver result = race->drivers[driver];
        if (dd2_application_championship_points(driver) != result.total_points ||
            dd2_championship_current(state)->rounds[0].drivers[driver].total_points !=
                result.total_points) {
            return false;
        }
        printf("%s[%u,%u,%u,%u,%u]", driver == 0 ? "" : ",", result.place, result.accident_points,
               result.finish_points, result.total_points, result.credited_laps);
    }
    puts("]}");
    printf("{\"kind\":\"standings\",\"drivers\":[");
    for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        const dd2_league_driver standing = state->league.drivers[driver];
        printf("%s[%u,%u,%u]", driver == 0 ? "" : ",", standing.division, standing.rank,
               standing.points);
    }
    puts("]}");
    return dd2_champ_app_capture("results") && fflush(stdout) == 0;
}

int dd2_champ_app_step(void) {
    if (dd2_champ_app.failed) {
        return -1;
    }
    if (dd2_champ_app.ready) {
        return 1;
    }
    const dd2_driving *driving = dd2_application_driving_view();
    const dd2_race *race = dd2_driving_race(driving);
    const dd2_road *road = dd2_track_road(dd2_application_track_view());
    for (unsigned step = 0;
         step < DD2_CHAMP_APP_BATCH && dd2_champ_app.tick < DD2_CHAMP_APP_BOUND &&
         race->phase != DD2_RACE_RESULTS;
         ++step) {
        dd2_vehicle_control control = {.brake = 1};
        const dd2_ai_observation observation = {.road = road,
                                                .surface = dd2_champ_app.surface,
                                                .vehicles = dd2_driving_vehicles(driving),
                                                .count = DD2_LEAGUE_DRIVERS,
                                                .slot = 0};
        if ((race->phase == DD2_RACE_RUNNING &&
             !dd2_ai_driver_step(&dd2_champ_app.pilot, &observation, &control)) ||
            !dd2_application_advance(
                (dd2_driving_frame){.seconds = DD2_VEHICLE_STEP_SECONDS, .control = control})) {
            dd2_champ_app.failed = true;
            return -1;
        }
        ++dd2_champ_app.tick;
    }
    if (dd2_champ_app.tick % DD2_CHAMP_APP_CHECKPOINT == 0) {
        printf("{\"kind\":\"checkpoint\",\"tick\":%u,\"phase\":%u}\n", dd2_champ_app.tick,
               (unsigned)race->phase);
        if (fflush(stdout) != 0) {
            dd2_champ_app.failed = true;
            return -1;
        }
    }
    if (race->phase == DD2_RACE_RESULTS) {
        dd2_champ_app.ready = dd2_champ_app_results();
        dd2_champ_app.failed = !dd2_champ_app.ready;
        return dd2_champ_app.ready ? 1 : -1;
    }
    if (dd2_champ_app.tick == DD2_CHAMP_APP_BOUND) {
        dd2_champ_app.failed = true;
        return -1;
    }
    return 0;
}

int dd2_champ_app_finish(void) {
    if (!dd2_champ_app.ready || dd2_champ_app.failed) {
        return 0;
    }
    const dd2_driving *driving = dd2_application_driving_view();
    const dd2_track *track = dd2_application_track_view();
    const unsigned points = dd2_application_championship_points(0);
    const unsigned steps = dd2_application_race_steps();
    bool valid = !dd2_application_restart_championship() &&
                 dd2_application_advance((dd2_driving_frame){.seconds = dd2_champ_app_hold_seconds,
                                                             .control = {.throttle = 1}}) &&
                 dd2_application_race_steps() == steps &&
                 dd2_application_championship_points(0) == points;
    dd2_road_surface_destroy(dd2_champ_app.surface);
    dd2_champ_app.surface = NULL;
    const bool continued = dd2_application_continue_championship() != 0;
    if (dd2_champ_app.missing) {
        valid = valid && !continued && driving == dd2_application_driving_view() &&
                track == dd2_application_track_view() &&
                dd2_application_championship_phase() == DD2_CHAMPIONSHIP_ROUND_RESULTS &&
                dd2_application_current_level() == 1 && dd2_champ_app_capture("retained");
    } else {
        valid = valid && continued &&
                dd2_application_championship_phase() == DD2_CHAMPIONSHIP_RACING &&
                dd2_application_current_level() == 2 && dd2_application_championship_round() == 2 &&
                dd2_application_race_steps() == 0 && dd2_champ_app_capture("next") &&
                dd2_application_restart_championship() && dd2_application_championship_round() == 2;
    }
    valid = valid && dd2_application_championship_points(0) == points &&
            dd2_application_exit_championship() &&
            dd2_application_current_level() == DD2_CHAMP_APP_PRACTICE &&
            dd2_application_championship_phase() == -1 && dd2_champ_app_capture("exit");
    dd2_application_close();
    valid = valid && dd2_application_current_level() == 0 &&
            dd2_application_driving_view() == NULL && dd2_application_image_view().pixels == NULL &&
            dd2_application_open(dd2_champ_app.archive, 1) && dd2_application_present();
    dd2_application_close();
    printf("{\"kind\":\"summary\",\"mode\":%u,\"missing_next_track_fixture\":%s,"
           "\"valid\":%s}\n",
           (unsigned)dd2_champ_app.mode, dd2_champ_app.missing ? "true" : "false",
           valid ? "true" : "false");
    return fflush(stdout) == 0 && valid;
}

int main(int argc, char **argv) {
    const bool observe = argc == DD2_CHAMP_APP_ARGUMENTS + 1 && strcmp(argv[5], "observe") == 0;
    if ((argc != DD2_CHAMP_APP_ARGUMENTS && !observe) ||
        (strcmp(argv[2], "stock") != 0 && strcmp(argv[2], "wreck") != 0) ||
        (strcmp(argv[3], "next") != 0 && strcmp(argv[3], "missing") != 0)) {
        return EXIT_FAILURE;
    }
    dd2_champ_app = (dd2_champ_app_export){
        .mode = strcmp(argv[2], "stock") == 0 ? DD2_RACE_STOCKCAR : DD2_RACE_WRECKING,
        .missing = strcmp(argv[3], "missing") == 0};
    if (strlen(argv[1]) >= sizeof(dd2_champ_app.archive) ||
        strlen(argv[4]) >= sizeof(dd2_champ_app.output)) {
        return EXIT_FAILURE;
    }
    for (size_t byte = 0; byte <= strlen(argv[1]); ++byte) {
        dd2_champ_app.archive[byte] = argv[1][byte];
    }
    for (size_t byte = 0; byte <= strlen(argv[4]); ++byte) {
        dd2_champ_app.output[byte] = argv[4][byte];
    }
    if (!dd2_champ_app_entry()) {
        dd2_application_close();
        return EXIT_FAILURE;
    }
    const dd2_road *road = dd2_track_road(dd2_application_track_view());
    dd2_champ_app.surface = dd2_road_surface_create(road);
    if (dd2_champ_app.surface == NULL ||
        !dd2_ai_driver_reset(
            &dd2_champ_app.pilot,
            (dd2_ai_start){.road = road,
                           .cell = dd2_driving_laps(dd2_application_driving_view())[0].cell,
                           .slot = 0,
                           .count = DD2_LEAGUE_DRIVERS})) {
        dd2_road_surface_destroy(dd2_champ_app.surface);
        dd2_application_close();
        return EXIT_FAILURE;
    }
#ifdef __EMSCRIPTEN__
    return EXIT_SUCCESS;
#else
    int phase = 0;
    while (phase == 0) {
        phase = dd2_champ_app_step();
    }
    if (phase < 0) {
        dd2_road_surface_destroy(dd2_champ_app.surface);
        dd2_application_close();
        return EXIT_FAILURE;
    }
    if (observe && getchar() != '\n') {
        dd2_road_surface_destroy(dd2_champ_app.surface);
        dd2_application_close();
        return EXIT_FAILURE;
    }
    return dd2_champ_app_finish() ? EXIT_SUCCESS : EXIT_FAILURE;
#endif
}
