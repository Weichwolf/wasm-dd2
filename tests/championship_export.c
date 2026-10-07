#include "ai/driver.h"
#include "assets/archive.h"
#include "assets/road.h"
#include "assets/track.h"
#include "game/championship.h"
#include "game/championship_session.h"
#include "game/driving.h"
#include "game/league.h"
#include "game/race.h"
#include "game/sound_events.h"
#include "game/starting_grid.h"
#include "physics/damage.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "platform/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_CHAMP_EXPORT_BOUND = 300000,
    DD2_CHAMP_EXPORT_RECEIPT_INTERVAL = 5000,
    DD2_CHAMP_EXPORT_HOLD = 10,
    DD2_CHAMP_EXPORT_SUSPEND = 200
};

static const double dd2_champ_export_partial = 0.4;
static const double dd2_champ_export_hold_seconds = 0.05;

static bool dd2_champ_export_grid(const dd2_championship_session *session, unsigned level) {
    const dd2_driving *driving = dd2_championship_session_driving(session);
    const dd2_road *road = dd2_track_road(dd2_championship_session_track(session));
    const dd2_championship *state = dd2_championship_session_state(session);
    dd2_road_surface *surface = dd2_road_surface_create(road);
    dd2_grid_start original[DD2_LEAGUE_DRIVERS] = {0};
    unsigned slots[DD2_LEAGUE_DRIVERS] = {0};
    bool valid = surface != NULL && dd2_league_grid(&state->league, slots) &&
                 dd2_starting_grid(road, surface, level, DD2_LEAGUE_DRIVERS, original) &&
                 dd2_driving_race(driving)->phase == DD2_RACE_COUNTDOWN &&
                 dd2_driving_race(driving)->steps == 0 &&
                 dd2_driving_vehicle_count(driving) == DD2_LEAGUE_DRIVERS;
    for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS && valid; ++driver) {
        const dd2_vehicle_spawn *start = dd2_driving_grid_start(driving, driver);
        const dd2_vehicle_spawn expected = original[slots[driver]].spawn;
        valid = start != NULL && start->position.x == expected.position.x &&
                start->position.y == expected.position.y &&
                start->position.z == expected.position.z && start->yaw == expected.yaw &&
                dd2_vehicle_valid(&dd2_driving_vehicles(driving)[driver]);
    }
    dd2_road_surface_destroy(surface);
    return valid;
}

static void dd2_champ_export_results(const dd2_race *race) {
    printf(
        "{\"kind\":\"result\",\"mode\":%u,\"steps\":%llu,\"elapsed\":%llu,\"end\":%u,\"drivers\":[",
        (unsigned)race->rules.mode, (unsigned long long)race->steps,
        (unsigned long long)race->elapsed, (unsigned)race->end);
    for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        const dd2_race_driver result = race->drivers[driver];
        printf("%s[%u,%u,%u,%u,%u]", driver == 0 ? "" : ",", result.place, result.accident_points,
               result.finish_points, result.total_points, result.credited_laps);
    }
    puts("]}");
}

static bool dd2_champ_export_live(const dd2_championship_session *session, unsigned tick) {
    const dd2_driving *driving = dd2_championship_session_driving(session);
    const dd2_race *race = dd2_driving_race(driving);
    printf("{\"kind\":\"checkpoint\",\"tick\":%u,\"phase\":%u,\"elapsed\":%llu,\"engine\":%.17g}\n",
           tick, (unsigned)race->phase, (unsigned long long)race->elapsed,
           dd2_damage_health(dd2_driving_damage(driving)));
    return fflush(stdout) == 0;
}

static bool dd2_champ_export_run(dd2_championship_session *session) {
    const dd2_driving *driving = dd2_championship_session_driving(session);
    const dd2_race *race = dd2_driving_race(driving);
    const dd2_road *road = dd2_track_road(dd2_championship_session_track(session));
    dd2_road_surface *surface = dd2_road_surface_create(road);
    dd2_ai_driver pilot = {0};
    bool valid =
        surface != NULL &&
        dd2_ai_driver_reset(&pilot, (dd2_ai_start){.road = road,
                                                   .cell = dd2_driving_laps(driving)[0].cell,
                                                   .slot = 0,
                                                   .count = DD2_LEAGUE_DRIVERS});
    for (unsigned tick = 0;
         tick < DD2_CHAMP_EXPORT_BOUND && valid && race->phase != DD2_RACE_RESULTS; ++tick) {
        /* Existing AI supplies ordinary controls only. Never inject damage,
         * positions, laps, retirement, points or result state. */
        dd2_vehicle_control input = {.brake = 1};
        const dd2_ai_observation observation = {.road = road,
                                                .surface = surface,
                                                .vehicles = dd2_driving_vehicles(driving),
                                                .count = DD2_LEAGUE_DRIVERS,
                                                .slot = 0};
        if (race->phase == DD2_RACE_RUNNING) {
            valid = dd2_ai_driver_step(&pilot, &observation, &input);
        }
        valid = valid && dd2_championship_session_advance(
                             session, (dd2_driving_frame){.seconds = DD2_VEHICLE_STEP_SECONDS,
                                                          .control = input});
        if (valid && tick % DD2_CHAMP_EXPORT_RECEIPT_INTERVAL == 0) {
            valid = dd2_champ_export_live(session, tick);
        }
    }
    dd2_road_surface_destroy(surface);
    return valid && race->phase == DD2_RACE_RESULTS && race->end != DD2_RACE_WITHDRAWN &&
           race->coasting == DD2_RACE_COAST_STEPS;
}

static bool dd2_champ_export_scores(const dd2_championship_session *session) {
    const dd2_championship *state = dd2_championship_session_state(session);
    const dd2_championship_season *season = dd2_championship_current(state);
    const dd2_race *race = dd2_driving_race(dd2_championship_session_driving(session));
    if (state->phase != DD2_CHAMPIONSHIP_ROUND_RESULTS || season == NULL ||
        season->completed != 1) {
        return false;
    }
    for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        if (state->league.drivers[driver].points != race->drivers[driver].total_points ||
            season->rounds[0].drivers[driver].total_points != race->drivers[driver].total_points ||
            season->standings.drivers[driver].points != race->drivers[driver].total_points) {
            return false;
        }
    }
    return true;
}

static bool dd2_champ_export_hold(dd2_championship_session *session) {
    const dd2_championship *state = dd2_championship_session_state(session);
    const dd2_driving *driving = dd2_championship_session_driving(session);
    const uint64_t steps = dd2_driving_vehicle(driving)->steps;
    const uint64_t elapsed = dd2_driving_race(driving)->elapsed;
    const uint64_t ticket = state->ticket;
    const unsigned points = state->league.drivers[0].points;
    for (unsigned frame = 0; frame < DD2_CHAMP_EXPORT_HOLD; ++frame) {
        if (!dd2_championship_session_advance(
                session, (dd2_driving_frame){.seconds = dd2_champ_export_hold_seconds,
                                             .control = {.throttle = 1}})) {
            return false;
        }
    }
    return !dd2_championship_session_advance(session, (dd2_driving_frame){.seconds = -1}) &&
           dd2_driving_sound_events(driving)->count == 0 &&
           dd2_driving_vehicle(driving)->steps == steps &&
           dd2_driving_race(driving)->elapsed == elapsed && state->ticket == ticket &&
           state->league.drivers[0].points == points && dd2_champ_export_scores(session);
}

static bool dd2_champ_export_continue(dd2_championship_session *session, bool missing) {
    const dd2_driving *previous_driving = dd2_championship_session_driving(session);
    const dd2_track *previous_track = dd2_championship_session_track(session);
    const dd2_championship *state = dd2_championship_session_state(session);
    const uint64_t ticket = state->ticket;
    const unsigned points = state->league.drivers[0].points;
    const bool continued = dd2_championship_session_continue(session);
    if (missing) {
        return !continued && dd2_championship_session_driving(session) == previous_driving &&
               dd2_championship_session_track(session) == previous_track &&
               state->ticket == ticket && dd2_champ_export_scores(session);
    }
    return continued && state->phase == DD2_CHAMPIONSHIP_RACING && state->ticket == ticket + 1 &&
           state->league.drivers[0].points == points && dd2_championship_track(state) == 2 &&
           dd2_champ_export_grid(session, 2);
}

static bool dd2_champ_export_session(const dd2_archive *archive, dd2_race_mode mode, bool missing) {
    dd2_championship_session *session = dd2_championship_session_create(archive, mode);
    bool valid = session != NULL && dd2_champ_export_grid(session, 1);
    if (valid) {
        const dd2_race *race = dd2_driving_race(dd2_championship_session_driving(session));
        const dd2_driving_frame partial = {.seconds =
                                               DD2_VEHICLE_STEP_SECONDS * dd2_champ_export_partial};
        valid = dd2_championship_session_advance(session, partial) && race->steps == 0;
        dd2_championship_session_suspend(session);
        valid = valid &&
                dd2_championship_session_advance(
                    session, (dd2_driving_frame){.seconds = DD2_VEHICLE_STEP_SECONDS *
                                                            (1 - dd2_champ_export_partial)}) &&
                race->steps == 0;
        dd2_championship_session_suspend(session);
        valid = valid && race->steps == 0 && !dd2_championship_session_continue(session) &&
                dd2_champ_export_run(session) && dd2_champ_export_scores(session) &&
                dd2_champ_export_hold(session);
    }
    if (valid) {
        const dd2_race *race = dd2_driving_race(dd2_championship_session_driving(session));
        dd2_champ_export_results(race);
        valid = dd2_champ_export_continue(session, missing);
    }
    if (valid) {
        const dd2_championship *state = dd2_championship_session_state(session);
        const unsigned points = state->league.drivers[0].points;
        const unsigned completed = dd2_championship_current(state)->completed;
        valid = dd2_championship_session_abort(session) &&
                state->phase == DD2_CHAMPIONSHIP_ABORTED &&
                state->league.drivers[0].points == points &&
                dd2_championship_current(state)->completed == completed &&
                !dd2_championship_session_continue(session) &&
                !dd2_championship_session_advance(session, (dd2_driving_frame){.seconds = 1});
    }
    printf("{\"kind\":\"summary\",\"mode\":%u,\"missing_next_track_fixture\":%s,\"valid\":%s}\n",
           (unsigned)mode, missing ? "true" : "false", valid ? "true" : "false");
    dd2_championship_session_destroy(session);
    return valid;
}

int main(int argc, char **argv) {
    if (argc != 4 || (strcmp(argv[2], "stock") != 0 && strcmp(argv[2], "wreck") != 0) ||
        (strcmp(argv[3], "next") != 0 && strcmp(argv[3], "missing") != 0)) {
        return EXIT_FAILURE;
    }
    dd2_file file = {0};
    dd2_archive *archive = NULL;
    const dd2_race_mode mode =
        strcmp(argv[2], "stock") == 0 ? DD2_RACE_STOCKCAR : DD2_RACE_WRECKING;
    bool valid = dd2_file_read(argv[1], &file) &&
                 dd2_archive_open(file.data, file.size, &archive) == DD2_ARCHIVE_OK;
    if (valid) {
        valid = dd2_champ_export_session(archive, mode, strcmp(argv[3], "missing") == 0);
    }
    dd2_archive_close(archive);
    dd2_file_release(&file);
    return valid && ferror(stdout) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
