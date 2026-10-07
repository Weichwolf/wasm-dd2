#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "game/course.h"
#include "game/driving.h"
#include "game/laps.h"
#include "game/starting_grid.h"
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
    DD2_LAP_EXPORT_LEVELS = 7,
    DD2_LAP_EXPORT_SPLIT = 8,
    DD2_LAP_EXPORT_STEPS = 1200,
    DD2_LAP_EXPORT_PARTS = 5,
    DD2_LAP_EXPORT_SPLIT_LIMIT = 8,
    DD2_LAP_EXPORT_ROUTE_LIMIT = 65536
};
static const double dd2_lap_export_frame = 0.025;
static const double dd2_lap_export_partial = 0.004;
static const double dd2_lap_export_resume = 0.001;

static dd2_road *dd2_lap_export_load(const char *path, char code) {
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
        road = dd2_road_create(&level, DD2_ROAD_RACING);
    }
    dd2_archive_close(archive);
    dd2_file_release(&file);
    return road;
}

static void dd2_lap_export_state(dd2_lap_driver lap) {
    printf("[%llu,%llu,%llu,%llu,%llu,%u,%u,%u,%u,%u,%d,%d]", (unsigned long long)lap.steps,
           (unsigned long long)lap.lap_start, (unsigned long long)lap.last_lap,
           (unsigned long long)lap.best_lap, (unsigned long long)lap.finish_step, lap.cell,
           lap.relative, lap.checkpoint, lap.started_laps, lap.credited_laps, (int)lap.retired,
           (int)lap.finished);
}

static bool dd2_lap_export_same(dd2_lap_driver first, dd2_lap_driver second) {
    return first.steps == second.steps && first.lap_start == second.lap_start &&
           first.last_lap == second.last_lap && first.best_lap == second.best_lap &&
           first.finish_step == second.finish_step && first.cell == second.cell &&
           first.relative == second.relative && first.checkpoint == second.checkpoint &&
           first.started_laps == second.started_laps &&
           first.credited_laps == second.credited_laps && first.retired == second.retired &&
           first.finished == second.finished;
}

static uint32_t dd2_lap_export_next(const dd2_road *road, uint32_t strip, unsigned variant) {
    const dd2_road_strip *strips = dd2_road_strips(road);
    unsigned bit = 0;
    for (size_t index = 0; index < dd2_road_strip_count(road); ++index) {
        if (strips[index].kind != DD2_LAP_EXPORT_SPLIT ||
            strips[index].main_order == DD2_ROAD_NO_STRIP) {
            continue;
        }
        if (index == strip && (variant & (1U << bit)) != 0) {
            return strips[strip].branch;
        }
        ++bit;
    }
    return strips[strip].next;
}

static bool dd2_lap_export_routes(const dd2_road *road, const dd2_course *course,
                                  const dd2_grid_start *starts) {
    const dd2_road_strip *strips = dd2_road_strips(road);
    unsigned splits = 0;
    for (size_t index = 0; index < dd2_road_strip_count(road); ++index) {
        splits += (unsigned)(strips[index].kind == DD2_LAP_EXPORT_SPLIT &&
                             strips[index].main_order != DD2_ROAD_NO_STRIP);
    }
    if (splits > DD2_LAP_EXPORT_SPLIT_LIMIT) {
        return false;
    }
    const unsigned variants = 1U << splits;
    for (unsigned variant = 0; variant < variants; ++variant) {
        dd2_lap_driver lap = {0};
        if (!dd2_laps_reset(&lap, course, starts[0].cell)) {
            return false;
        }
        uint32_t strip = dd2_road_cells(road)[starts[0].cell].strip;
        for (unsigned step = 0; step < DD2_LAP_EXPORT_ROUTE_LIMIT && !lap.finished; ++step) {
            strip = dd2_lap_export_next(road, strip, variant);
            const uint32_t cell = strips[strip].first_cell;
            if (!dd2_laps_step(&lap, course, (dd2_lap_observation){.cells = &cell, .count = 1})) {
                return false;
            }
            printf("{\"kind\":\"route\",\"variant\":%u,\"state\":", variant);
            dd2_lap_export_state(lap);
            puts("}");
        }
        if (!lap.finished || dd2_laps_completed(&lap) != dd2_course_laps(course) ||
            lap.best_lap == 0) {
            return false;
        }
    }
    return true;
}

static void dd2_lap_export_live(const dd2_driving *driving, unsigned step) {
    printf("{\"kind\":\"live\",\"step\":%u,\"cars\":[", step);
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        const dd2_vehicle vehicle = dd2_driving_vehicles(driving)[slot];
        printf("%s{\"position\":[%.17g,%.17g,%.17g],\"retired\":%d,\"state\":",
               slot == 0 ? "" : ",", vehicle.position.x, vehicle.position.y, vehicle.position.z,
               (int)dd2_driving_damage(driving)[slot].retired);
        dd2_lap_export_state(dd2_driving_laps(driving)[slot]);
        printf("}");
    }
    puts("]}");
}

static bool dd2_lap_export_driving(const dd2_road *road, unsigned level) {
    dd2_driving *driving = dd2_driving_create(road, level);
    dd2_driving *partitioned = dd2_driving_create(road, level);
    bool valid = driving != NULL && partitioned != NULL;
    if (valid) {
        dd2_lap_export_live(driving, 0);
    }
    const dd2_vehicle_control control = {.throttle = 1};
    for (unsigned step = 0; step < DD2_LAP_EXPORT_STEPS && valid; ++step) {
        if (step % DD2_LAP_EXPORT_PARTS == 0) {
            valid = dd2_driving_advance(
                partitioned,
                (dd2_driving_frame){.seconds = dd2_lap_export_frame, .control = control});
        }
        valid = valid && dd2_driving_advance(
                             driving, (dd2_driving_frame){.seconds = DD2_VEHICLE_STEP_SECONDS,
                                                          .control = control});
        if (valid && (step + 1) % DD2_LAP_EXPORT_PARTS == 0) {
            for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT && valid; ++slot) {
                valid = dd2_lap_export_same(dd2_driving_laps(driving)[slot],
                                            dd2_driving_laps(partitioned)[slot]);
            }
        }
        if (valid) {
            dd2_lap_export_live(driving, step + 1);
        }
    }
    if (valid) {
        const dd2_lap_driver before = dd2_driving_laps(driving)[0];
        valid = dd2_driving_advance(
            driving, (dd2_driving_frame){.seconds = dd2_lap_export_partial, .control = control});
        dd2_driving_suspend(driving);
        valid = valid &&
                dd2_driving_advance(driving, (dd2_driving_frame){.seconds = dd2_lap_export_resume,
                                                                 .control = control}) &&
                dd2_lap_export_same(before, dd2_driving_laps(driving)[0]) &&
                !dd2_driving_advance(driving, (dd2_driving_frame){.seconds = -1}) &&
                dd2_lap_export_same(before, dd2_driving_laps(driving)[0]) &&
                dd2_driving_reset(driving);
        if (valid) {
            dd2_lap_export_live(driving, 0);
        }
    }
    dd2_driving_destroy(driving);
    dd2_driving_destroy(partitioned);
    return valid;
}

static bool dd2_lap_export(const char *path, char code) {
    const unsigned level = (unsigned)(code - '0');
    dd2_road *road = dd2_lap_export_load(path, code);
    dd2_road_surface *surface = dd2_road_surface_create(road);
    dd2_course_rules rules = {0};
    dd2_course *course = NULL;
    dd2_grid_start starts[DD2_VEHICLE_FLEET_LIMIT] = {0};
    bool valid = road != NULL && surface != NULL && dd2_course_original_rules(level, &rules) &&
                 dd2_starting_grid(road, surface, level, DD2_VEHICLE_FLEET_LIMIT, starts);
    if (valid) {
        course = dd2_course_create(road, rules);
        valid = course != NULL;
    }
    if (valid) {
        printf("{\"kind\":\"course\",\"length\":%u,\"finish\":%u,\"laps\":%u,\"strips\":[",
               dd2_course_length(course), dd2_course_finish(course), dd2_course_laps(course));
        for (size_t index = 0; index < dd2_road_strip_count(road); ++index) {
            const dd2_road_strip strip = dd2_road_strips(road)[index];
            printf("%s[%u,%u,%u]", index == 0 ? "" : ",", strip.source_offset,
                   dd2_course_numbers(course)[index], strip.first_cell);
        }
        printf("],\"starts\":[");
        for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
            printf("%s%u", slot == 0 ? "" : ",", starts[slot].cell);
        }
        puts("]}");
        valid = dd2_lap_export_routes(road, course, starts) &&
                dd2_lap_export_driving(road, level) && ferror(stdout) == 0;
    }
    dd2_course_destroy(course);
    dd2_road_surface_destroy(surface);
    dd2_road_destroy(road);
    return valid;
}

int main(int argc, char **argv) {
    if (argc != 3 || strlen(argv[2]) != 1 || argv[2][0] < '1' ||
        argv[2][0] > '0' + DD2_LAP_EXPORT_LEVELS) {
        return EXIT_FAILURE;
    }
    return dd2_lap_export(argv[1], argv[2][0]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
