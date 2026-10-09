#include "ai/path.h"
#include "assets/archive.h"
#include "assets/barriers.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "game/course.h"
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

enum { DD2_GAMEPLAY_EXPORT_CIRCUITS = 7, DD2_GAMEPLAY_EXPORT_LEVELS = 11 };
static const double dd2_gameplay_export_lookahead = 4000;

static dd2_road *dd2_gameplay_export_load(const char *path, unsigned level, bool reference) {
    dd2_file file = {0};
    dd2_archive *archive = NULL;
    dd2_road *road = NULL;
    if (!dd2_file_read(path, &file)) {
        return NULL;
    }
    if (reference) {
        const char codes[] = "123456789AB";
        char name[] = "LEV0\\LEVEL.DAT";
        name[3] = codes[level - 1];
        dd2_asset asset = {0};
        dd2_level_data decoded = {0};
        if (dd2_archive_open(file.data, file.size, &archive) == DD2_ARCHIVE_OK &&
            dd2_archive_find(archive, name, &asset) &&
            dd2_level_decode((dd2_byte_view){asset.bytes, asset.size}, &decoded)) {
            road = dd2_road_create(
                &decoded, level <= DD2_GAMEPLAY_EXPORT_CIRCUITS ? DD2_ROAD_RACING : DD2_ROAD_ARENA);
        }
    } else {
        road = dd2_road_create_prepared((dd2_byte_view){file.data, file.size});
    }
    dd2_archive_close(archive);
    dd2_file_release(&file);
    return road;
}

static void dd2_gameplay_export_barriers(const dd2_barriers *barriers) {
    printf("\"barrier_radius\":%.17g,\"barriers\":[", dd2_barriers_radius(barriers));
    for (size_t index = 0; index < dd2_barriers_count(barriers); ++index) {
        const dd2_barrier_segment *segment = &dd2_barriers_segments(barriers)[index];
        printf("%s[%d,%d,%d,%d,%d,%d,%u,%u]", index == 0 ? "" : ",", segment->start.x,
               segment->start.y, segment->start.z, segment->end.x, segment->end.y, segment->end.z,
               segment->strip, segment->side);
    }
    printf("]");
}

static void dd2_gameplay_export_course(const dd2_course *course) {
    printf(",\"course\":{\"length\":%u,\"laps\":%u,\"finish\":%u,\"numbers\":[",
           dd2_course_length(course), dd2_course_laps(course), dd2_course_finish(course));
    for (size_t index = 0; index < dd2_course_strip_count(course); ++index) {
        printf("%s%u", index == 0 ? "" : ",", dd2_course_numbers(course)[index]);
    }
    printf("]}");
}

static void dd2_gameplay_export_paths(const dd2_road *road) {
    printf(",\"paths\":[");
    for (size_t index = 0; index < dd2_road_cell_count(road); ++index) {
        const dd2_road_cell *cell = &dd2_road_cells(road)[index];
        dd2_vehicle_vector position = {0};
        for (size_t corner = 0; corner < DD2_ROAD_CORNERS; ++corner) {
            const dd2_track_vertex point = dd2_road_vertices(road)[cell->vertices[corner]];
            position.x += (double)point.x / (double)DD2_ROAD_CORNERS;
            position.y += (double)point.y / (double)DD2_ROAD_CORNERS;
            position.z += (double)point.z / (double)DD2_ROAD_CORNERS;
        }
        dd2_ai_path_sample sample = {0};
        const bool found =
            dd2_ai_path(road,
                        (dd2_ai_path_query){.cell = (uint32_t)index,
                                            .lane = 0.5,
                                            .position = position,
                                            .distance = dd2_gameplay_export_lookahead},
                        &sample);
        printf("%s[%d,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%u]", index == 0 ? "" : ",",
               (int)found, sample.point.x, sample.point.y, sample.point.z, sample.direction.x,
               sample.direction.y, sample.direction.z, sample.width, sample.curvature,
               sample.strip);
    }
    printf("]");
}

static bool dd2_gameplay_export(const dd2_road *road, unsigned level) {
    dd2_road_surface *surface = dd2_road_surface_create(road);
    dd2_barriers *barriers = dd2_barriers_create(road, level);
    dd2_course_rules rules = {0};
    dd2_course *course = NULL;
    const bool racing = level <= DD2_GAMEPLAY_EXPORT_CIRCUITS;
    if (racing && dd2_course_original_rules(level, &rules)) {
        course = dd2_course_create(road, rules);
    }
    dd2_grid_start starts[DD2_VEHICLE_FLEET_LIMIT] = {0};
    const bool valid = surface != NULL && barriers != NULL && (!racing || course != NULL) &&
                       dd2_starting_grid(road, surface, level, DD2_VEHICLE_FLEET_LIMIT, starts);
    if (valid) {
        printf("{");
        dd2_gameplay_export_barriers(barriers);
        dd2_gameplay_export_course(course);
        printf(",\"starts\":[");
        for (size_t index = 0; index < DD2_VEHICLE_FLEET_LIMIT; ++index) {
            const dd2_vehicle_spawn *spawn = &starts[index].spawn;
            printf("%s[%.17g,%.17g,%.17g,%.17g,%u]", index == 0 ? "" : ",", spawn->position.x,
                   spawn->position.y, spawn->position.z, spawn->yaw, starts[index].cell);
        }
        printf("]");
        dd2_gameplay_export_paths(road);
        printf("}\n");
    }
    dd2_course_destroy(course);
    dd2_barriers_destroy(barriers);
    dd2_road_surface_destroy(surface);
    return valid && ferror(stdout) == 0;
}

int main(int argc, char **argv) {
    const char codes[] = "123456789AB";
    if (argc != 4 || (strcmp(argv[1], "prepared") != 0 && strcmp(argv[1], "reference") != 0) ||
        strlen(argv[3]) != 1) {
        return EXIT_FAILURE;
    }
    const char *code = strchr(codes, argv[3][0]);
    if (code == NULL) {
        return EXIT_FAILURE;
    }
    const unsigned level = (unsigned)(code - codes) + 1;
    dd2_road *road = dd2_gameplay_export_load(argv[2], level, strcmp(argv[1], "reference") == 0);
    const bool passed = road != NULL && dd2_gameplay_export(road, level);
    dd2_road_destroy(road);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
