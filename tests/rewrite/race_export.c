#include "ai/driver.h"
#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "game/accidents.h"
#include "game/course.h"
#include "game/driving.h"
#include "game/laps.h"
#include "game/race.h"
#include "physics/damage.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"
#include "platform/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_RACE_EXPORT_ARGUMENTS = 5,
    DD2_RACE_EXPORT_RACING_LEVELS = 7,
    DD2_RACE_EXPORT_LIVE_STEPS = 1600,
    DD2_RACE_EXPORT_ROUTE_LIMIT = 20000,
    DD2_RACE_EXPORT_PARTS = 5,
    DD2_RACE_EXPORT_SCORE_UNIT = 10,
    DD2_RACE_EXPORT_AUTO_LIMIT = 240000
};
static const double dd2_race_export_frame = 0.025;
static const double dd2_race_export_partial = 0.004;
static const double dd2_race_export_resume = 0.001;

static dd2_road *dd2_race_export_load(const char *path, char code) {
    dd2_file file = {0};
    dd2_archive *archive = NULL;
    dd2_asset asset = {0};
    dd2_level_data level = {0};
    dd2_road *road = NULL;
    char name[] = "LEV0\\LEVEL.DAT";
    name[3] = code;
    if (dd2_file_read(path, &file) &&
        dd2_archive_open(file.data, file.size, &archive) == DD2_ARCHIVE_OK &&
        dd2_archive_find(archive, name, &asset) &&
        dd2_level_decode((dd2_byte_view){.data = asset.bytes, .size = asset.size}, &level)) {
        road = dd2_road_create(&level,
                               strchr("1234567", code) != NULL ? DD2_ROAD_RACING : DD2_ROAD_ARENA);
    }
    dd2_archive_close(archive);
    dd2_file_release(&file);
    return road;
}

static void dd2_race_export_state(const dd2_race *race) {
    printf("\"state\":[%d,%d,%llu,%llu,%u,%u,%u],\"drivers\":[", (int)race->phase, (int)race->end,
           (unsigned long long)race->steps, (unsigned long long)race->elapsed, race->coasting,
           race->finishers, race->alive);
    for (unsigned slot = 0; slot < race->rules.count; ++slot) {
        const dd2_race_driver *driver = &race->drivers[slot];
        printf("%s[%llu,%llu,%u,%u,%u,%u,%u,%u,%u,%d]", slot == 0 ? "" : ",",
               (unsigned long long)driver->finish_step, (unsigned long long)driver->retired_step,
               driver->credited_laps, driver->relative, driver->place, driver->finish_place,
               driver->accident_points, driver->finish_points, driver->total_points,
               (int)driver->retired);
    }
    printf("],\"order\":[");
    for (unsigned index = 0; index < race->rules.count; ++index) {
        printf("%s%u", index == 0 ? "" : ",", race->order[index]);
    }
    printf("],\"results\":[");
    for (unsigned index = 0; index < race->rules.count; ++index) {
        printf("%s%u", index == 0 ? "" : ",", race->results[index]);
    }
    printf("]");
}
static void dd2_race_export_laps(const dd2_lap_driver *laps) {
    printf(",\"laps\":[");
    if (laps != NULL) {
        for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
            const dd2_lap_driver *lap = &laps[slot];
            printf("%s[%llu,%llu,%u,%u,%d,%d]", slot == 0 ? "" : ",",
                   (unsigned long long)lap->steps, (unsigned long long)lap->finish_step,
                   lap->credited_laps, lap->relative, (int)lap->retired, (int)lap->finished);
        }
    }
    printf("]");
}
static void dd2_race_export_live(const dd2_driving *driving, const char *kind) {
    printf("{\"kind\":\"%s\",", kind);
    dd2_race_export_state(dd2_driving_race(driving));
    dd2_race_export_laps(dd2_driving_laps(driving));
    printf(",\"cars\":[");
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        const dd2_vehicle *vehicle = &dd2_driving_vehicles(driving)[slot];
        const dd2_accident_driver *accident = &dd2_driving_accidents(driving)[slot];
        printf("%s[%llu,%.17g,%.17g,%.17g,%llu,%d,%u]", slot == 0 ? "" : ",",
               (unsigned long long)vehicle->steps, vehicle->position.x, vehicle->position.y,
               vehicle->position.z, (unsigned long long)accident->steps,
               (int)dd2_driving_damage(driving)[slot].retired, accident->points);
    }
    puts("]}");
}
static void dd2_race_export_meta(const dd2_driving *driving, const dd2_road *road,
                                 dd2_race_mode mode) {
    const dd2_course *course = dd2_driving_course(driving);
    printf("{\"kind\":\"meta\",\"mode\":%d,\"length\":%u,\"laps\":%u,\"strips\":[", (int)mode,
           dd2_course_length(course), dd2_course_laps(course));
    for (size_t index = 0; index < dd2_course_strip_count(course); ++index) {
        printf("%s[%u,%u,%u]", index == 0 ? "" : ",", dd2_road_strips(road)[index].source_offset,
               dd2_course_numbers(course)[index], dd2_road_strips(road)[index].first_cell);
    }
    printf("],\"starts\":[");
    const dd2_lap_driver *laps = dd2_driving_laps(driving);
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT && laps != NULL; ++slot) {
        printf("%s%u", slot == 0 ? "" : ",", laps[slot].cell);
    }
    puts("]}");
}

static bool dd2_race_export_lifecycle(dd2_driving *first) {
    bool valid = true;
    if (valid) {
        valid = dd2_driving_advance(first, (dd2_driving_frame){.seconds = dd2_race_export_partial});
        dd2_driving_suspend(first);
        valid =
            valid &&
            dd2_driving_advance(first, (dd2_driving_frame){.seconds = dd2_race_export_resume}) &&
            !dd2_driving_advance(first, (dd2_driving_frame){.seconds = -1});
        if (valid) {
            dd2_race_export_live(first, "pause");
            valid = dd2_driving_withdraw(first);
        }
        if (valid) {
            dd2_race_export_live(first, "withdraw");
            for (unsigned frame = 0; frame < DD2_RACE_EXPORT_PARTS && valid; ++frame) {
                valid =
                    dd2_driving_advance(first, (dd2_driving_frame){.seconds = dd2_race_export_frame,
                                                                   .control = {.throttle = 1}});
            }
            dd2_race_export_live(first, "frozen");
            valid = valid && dd2_driving_reset(first);
            if (valid) {
                dd2_race_export_live(first, "reset");
            }
        }
    }
    return valid;
}

static bool dd2_race_export_driving(unsigned level, const dd2_road *road, dd2_race_mode mode) {
    dd2_driving *first = dd2_driving_create(road, level);
    dd2_driving *second = dd2_driving_create(road, level);
    bool valid = first != NULL && second != NULL && dd2_driving_set_race(first, true, mode) &&
                 dd2_driving_set_race(second, true, mode);
    if (valid) {
        dd2_race_export_meta(first, road, mode);
        dd2_race_export_live(first, "live");
    }
    for (unsigned step = 0; step < DD2_RACE_EXPORT_LIVE_STEPS && valid; ++step) {
        const dd2_vehicle_control control = {.throttle = 1};
        if (step % DD2_RACE_EXPORT_PARTS == 0) {
            valid = dd2_driving_advance(
                second, (dd2_driving_frame){.seconds = dd2_race_export_frame, .control = control});
        }
        valid = valid &&
                dd2_driving_advance(first, (dd2_driving_frame){.seconds = DD2_VEHICLE_STEP_SECONDS,
                                                               .control = control});
        if (valid) {
            dd2_race_export_live(first, "live");
            if ((step + 1) % DD2_RACE_EXPORT_PARTS == 0) {
                dd2_race_export_live(second, "part");
            }
        }
    }
    valid = valid && dd2_race_export_lifecycle(first);
    dd2_driving_destroy(first);
    dd2_driving_destroy(second);
    return valid;
}

static bool dd2_race_export_route_step(dd2_lap_driver *laps, const dd2_road *road,
                                       const dd2_course *course, uint32_t *strips,
                                       dd2_accident_driver *accidents, uint64_t elapsed) {
    bool valid = true;
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT && valid; ++slot) {
        const unsigned cadence = slot == 0 ? 3 : (slot % DD2_RACE_EXPORT_PARTS) + 1;
        if (elapsed % cadence == 0) {
            strips[slot] = dd2_road_strips(road)[strips[slot]].next;
        }
        const uint32_t cell = dd2_road_strips(road)[strips[slot]].first_cell;
        valid =
            dd2_laps_step(&laps[slot], course, (dd2_lap_observation){.cells = &cell, .count = 1});
        accidents[slot].points = slot * DD2_RACE_EXPORT_SCORE_UNIT;
    }
    return valid;
}

static bool dd2_race_export_route(unsigned level, const dd2_road *road, dd2_race_mode mode) {
    dd2_driving *driving = dd2_driving_create(road, level);
    if (driving == NULL) {
        return false;
    }
    dd2_race_export_meta(driving, road, mode);
    const dd2_course *course = dd2_driving_course(driving);
    dd2_lap_driver laps[DD2_VEHICLE_FLEET_LIMIT] = {0};
    uint32_t strips[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        laps[slot] = dd2_driving_laps(driving)[slot];
        strips[slot] = dd2_road_cells(road)[laps[slot].cell].strip;
    }
    const dd2_vehicle_damage damage[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_accident_driver accidents[DD2_VEHICLE_FLEET_LIMIT] = {0};
    const dd2_race_observation observation = {
        .laps = laps, .damage = damage, .accidents = accidents, .count = DD2_VEHICLE_FLEET_LIMIT};
    dd2_race race = {0};
    bool valid = dd2_race_reset(&race,
                                (dd2_race_rules){.mode = mode,
                                                 .count = DD2_VEHICLE_FLEET_LIMIT,
                                                 .length = dd2_course_length(course),
                                                 .laps = dd2_course_laps(course)},
                                observation);
    for (unsigned tick = 0;
         tick < DD2_RACE_EXPORT_ROUTE_LIMIT && valid && race.phase != DD2_RACE_RESULTS; ++tick) {
        if (race.phase != DD2_RACE_COUNTDOWN) {
            valid = dd2_race_export_route_step(laps, road, course, strips, accidents, race.elapsed);
        }
        valid = valid && dd2_race_step(&race, observation);
        if (valid) {
            printf("{\"kind\":\"route\",");
            dd2_race_export_state(&race);
            dd2_race_export_laps(laps);
            printf(",\"cells\":[");
            for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
                printf("%s%u", slot == 0 ? "" : ",",
                       dd2_road_strips(road)[strips[slot]].source_offset);
            }
            puts("]}");
        }
    }
    valid = valid && race.phase == DD2_RACE_RESULTS && race.end == DD2_RACE_PLAYER_FINISHED;
    dd2_driving_destroy(driving);
    return valid;
}

static bool dd2_race_export_auto(unsigned level, const dd2_road *road, dd2_race_mode mode) {
    dd2_driving *driving = dd2_driving_create(road, level);
    dd2_road_surface *surface = dd2_road_surface_create(road);
    dd2_ai_driver pilot = {0};
    bool valid = driving != NULL && surface != NULL && dd2_driving_set_race(driving, true, mode);
    if (valid) {
        valid =
            dd2_ai_driver_reset(&pilot, (dd2_ai_start){.road = road,
                                                       .cell = dd2_driving_laps(driving)[0].cell,
                                                       .slot = 0,
                                                       .count = DD2_VEHICLE_FLEET_LIMIT});
        dd2_race_export_meta(driving, road, mode);
        dd2_race_export_live(driving, "auto-start");
    }
    for (unsigned tick = 0; tick < DD2_RACE_EXPORT_AUTO_LIMIT && valid &&
                            dd2_driving_race(driving)->phase != DD2_RACE_RESULTS;
         ++tick) {
        dd2_vehicle_control control = {.brake = 1};
        const dd2_ai_observation observation = {.road = road,
                                                .surface = surface,
                                                .vehicles = dd2_driving_vehicles(driving),
                                                .count = DD2_VEHICLE_FLEET_LIMIT,
                                                .slot = 0};
        if (dd2_driving_race(driving)->phase == DD2_RACE_RUNNING) {
            valid = dd2_ai_driver_step(&pilot, &observation, &control);
        }
        valid = valid && dd2_driving_advance(
                             driving, (dd2_driving_frame){.seconds = DD2_VEHICLE_STEP_SECONDS,
                                                          .control = control});
        if (valid) {
            const dd2_race *race = dd2_driving_race(driving);
            const dd2_lap_driver *lap = dd2_driving_laps(driving);
            const dd2_vehicle *vehicle = dd2_driving_vehicle(driving);
            printf("{\"kind\":\"auto\",\"state\":[%d,%d,%llu,%llu,%u,%u,%u],"
                   "\"lap\":[%llu,%llu,%llu,%llu,%llu,%u,%u,%u,%u,%u,%d,%d],"
                   "\"position\":[%.17g,%.17g,%.17g]}\n",
                   (int)race->phase, (int)race->end, (unsigned long long)race->steps,
                   (unsigned long long)race->elapsed, race->coasting, race->finishers, race->alive,
                   (unsigned long long)lap->steps, (unsigned long long)lap->lap_start,
                   (unsigned long long)lap->last_lap, (unsigned long long)lap->best_lap,
                   (unsigned long long)lap->finish_step, lap->cell, lap->relative, lap->checkpoint,
                   lap->started_laps, lap->credited_laps, (int)lap->retired, (int)lap->finished,
                   vehicle->position.x, vehicle->position.y, vehicle->position.z);
        }
    }
    if (valid) {
        dd2_race_export_live(driving, "auto-final");
        valid = dd2_driving_race(driving)->phase == DD2_RACE_RESULTS &&
                dd2_driving_race(driving)->end == DD2_RACE_PLAYER_FINISHED;
    }
    dd2_driving_destroy(driving);
    dd2_road_surface_destroy(surface);
    return valid;
}

int main(int argc, char **argv) {
    const char codes[] = "123456789AB";
    if (argc != DD2_RACE_EXPORT_ARGUMENTS || strlen(argv[2]) != 1 ||
        strchr(codes, argv[2][0]) == NULL ||
        (strcmp(argv[3], "wreck") != 0 && strcmp(argv[3], "stock") != 0) ||
        (strcmp(argv[4], "live") != 0 && strcmp(argv[4], "route") != 0 &&
         strcmp(argv[4], "auto") != 0)) {
        return EXIT_FAILURE;
    }
    const unsigned level = (unsigned)(strchr(codes, argv[2][0]) - codes) + 1;
    const dd2_race_mode mode =
        strcmp(argv[3], "stock") == 0 ? DD2_RACE_STOCKCAR : DD2_RACE_WRECKING;
    dd2_road *road = dd2_race_export_load(argv[1], argv[2][0]);
    bool valid = false;
    if (road != NULL) {
        if (strcmp(argv[4], "auto") == 0) {
            valid =
                level <= DD2_RACE_EXPORT_RACING_LEVELS && dd2_race_export_auto(level, road, mode);
        } else if (strcmp(argv[4], "route") == 0) {
            valid =
                level <= DD2_RACE_EXPORT_RACING_LEVELS && dd2_race_export_route(level, road, mode);
        } else {
            valid = dd2_race_export_driving(level, road, mode);
        }
    }
    dd2_road_destroy(road);
    return valid && ferror(stdout) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
