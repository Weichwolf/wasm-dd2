#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "game/driving.h"
#include "game/league.h"
#include "game/race.h"
#include "game/starting_grid.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "platform/file.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_GRID_CHECK_SETTLE = 200, DD2_GRID_CHECK_ACTIVE = 100, DD2_GRID_CHECK_RACING_LEVELS = 7 };
static const char dd2_grid_check_codes[] = "123456789AB";

static dd2_road *dd2_grid_check_road(const dd2_archive *archive, char code) {
    char name[] = "LEV0\\LEVEL.DAT";
    name[3] = code;
    dd2_asset asset = {0};
    dd2_level_data level = {0};
    if (!dd2_archive_find(archive, name, &asset) ||
        !dd2_level_decode((dd2_byte_view){.data = asset.bytes, .size = asset.size}, &level)) {
        return NULL;
    }
    return dd2_road_create(&level,
                           strchr("1234567", code) != NULL ? DD2_ROAD_RACING : DD2_ROAD_ARENA);
}

static bool dd2_grid_check_starts(const dd2_driving *driving, const dd2_grid_start *physical,
                                  const unsigned *slots) {
    for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        const dd2_vehicle_spawn *actual = dd2_driving_grid_start(driving, driver);
        const dd2_vehicle_spawn expected = physical[slots[driver]].spawn;
        if (actual == NULL || actual->position.x != expected.position.x ||
            actual->position.y != expected.position.y ||
            actual->position.z != expected.position.z || actual->yaw != expected.yaw ||
            !dd2_vehicle_valid(&dd2_driving_vehicles(driving)[driver]) ||
            dd2_driving_vehicles(driving)[driver].steps != DD2_GRID_CHECK_SETTLE) {
            return false;
        }
    }
    return dd2_driving_vehicle(driving) == &dd2_driving_vehicles(driving)[0];
}

static bool dd2_grid_check_run(dd2_driving *driving, dd2_race_mode mode, unsigned count) {
    if (!dd2_driving_set_race(driving, true, mode) || dd2_driving_vehicle_count(driving) != count) {
        return false;
    }
    for (unsigned step = 0; step < DD2_RACE_START_STEPS + DD2_GRID_CHECK_ACTIVE; ++step) {
        if (!dd2_driving_advance(driving, (dd2_driving_frame){.seconds = DD2_VEHICLE_STEP_SECONDS,
                                                              .control = {.throttle = 1}})) {
            return false;
        }
    }
    const dd2_race *race = dd2_driving_race(driving);
    for (unsigned driver = 0; driver < count; ++driver) {
        if (!dd2_vehicle_valid(&dd2_driving_vehicles(driving)[driver]) ||
            dd2_driving_vehicles(driving)[driver].steps !=
                DD2_GRID_CHECK_SETTLE + DD2_GRID_CHECK_ACTIVE ||
            (mode == DD2_RACE_TOTAL_DESTRUCTION && driver != 0 &&
             dd2_driving_drivers(driving)[driver].target != 0)) {
            return false;
        }
    }
    return race->phase == DD2_RACE_RUNNING && race->elapsed == DD2_GRID_CHECK_ACTIVE &&
           dd2_driving_withdraw(driving) && race->phase == DD2_RACE_RESULTS &&
           race->end == DD2_RACE_WITHDRAWN;
}

static bool dd2_grid_check_season(const dd2_road *road, unsigned level,
                                  const dd2_grid_start *physical, const dd2_league *league) {
    unsigned slots[DD2_LEAGUE_DRIVERS] = {0};
    if (!dd2_league_grid(league, slots)) {
        return false;
    }
    dd2_driving *driving = dd2_driving_create_grid(road, level, slots);
    if (driving == NULL) {
        return false;
    }
    const dd2_vehicle_spawn human = physical[slots[0]].spawn;
    const unsigned human_slot = slots[0];
    const dd2_race_mode mode =
        level <= DD2_GRID_CHECK_RACING_LEVELS ? DD2_RACE_STOCKCAR : DD2_RACE_TOTAL_DESTRUCTION;
    bool valid = dd2_grid_check_starts(driving, physical, slots) &&
                 dd2_grid_check_run(driving, mode, DD2_LEAGUE_DRIVERS);
    if (level <= DD2_GRID_CHECK_RACING_LEVELS) {
        valid = valid && dd2_grid_check_run(driving, DD2_RACE_TIME_TRIAL, 1);
    }
    valid = valid && dd2_driving_set_race(driving, true, mode) &&
            dd2_grid_check_starts(driving, physical, slots);
    for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        slots[driver] = DD2_LEAGUE_DRIVERS;
    }
    valid = valid && dd2_driving_reset(driving) &&
            dd2_driving_start(driving)->position.x == human.position.x &&
            dd2_driving_start(driving)->position.y == human.position.y &&
            dd2_driving_start(driving)->position.z == human.position.z &&
            dd2_driving_start(driving)->yaw == human.yaw;
    printf("{\"level\":%u,\"division\":%u,\"human_grid_slot\":%u,\"valid\":%s,\"scope\":\"Assigned "
           "original nominal starts, settled physical field, real active steps, withdrawal and "
           "mode/reset ownership; fixture league promotion only\"}\n",
           level, league->drivers[0].division, human_slot, valid ? "true" : "false");
    dd2_driving_destroy(driving);
    return valid;
}

static bool dd2_grid_check_level(const dd2_archive *archive, unsigned level) {
    dd2_road *road = dd2_grid_check_road(archive, dd2_grid_check_codes[level - 1]);
    dd2_road_surface *surface = dd2_road_surface_create(road);
    dd2_grid_start physical[DD2_LEAGUE_DRIVERS] = {0};
    dd2_league league = {0};
    bool valid = surface != NULL &&
                 dd2_starting_grid(road, surface, level, DD2_LEAGUE_DRIVERS, physical) &&
                 dd2_league_reset(&league);
    for (unsigned season = 0; season < DD2_LEAGUE_DIVISIONS && valid; ++season) {
        valid = dd2_grid_check_season(road, level, physical, &league);
        if (season + 1 < DD2_LEAGUE_DIVISIONS) {
            unsigned points[DD2_LEAGUE_DRIVERS] = {0};
            points[0] = DD2_LEAGUE_RACE_POINT_LIMIT;
            valid = valid && dd2_league_add_points(&league, points) && dd2_league_sort(&league) &&
                    dd2_league_transfer(&league) && dd2_league_clear_points(&league);
        }
    }
    dd2_road_surface_destroy(surface);
    dd2_road_destroy(road);
    return valid;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        return EXIT_FAILURE;
    }
    dd2_file file = {0};
    dd2_archive *archive = NULL;
    bool valid = dd2_file_read(argv[1], &file) &&
                 dd2_archive_open(file.data, file.size, &archive) == DD2_ARCHIVE_OK;
    for (unsigned level = 1; level < sizeof(dd2_grid_check_codes) && valid; ++level) {
        valid = dd2_grid_check_level(archive, level);
    }
    dd2_archive_close(archive);
    dd2_file_release(&file);
    return valid && ferror(stdout) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
