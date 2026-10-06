#include "archive_fixture.h"
#include "assets/archive.h"
#include "assets/barriers.h"
#include "assets/level.h"
#include "assets/road.h"
#include "physics/barrier_world.h"
#include "physics/vehicle.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_BARRIER_EXPORT_RACING = 7,
    DD2_BARRIER_EXPORT_ANGLES = 16,
    DD2_BARRIER_EXPORT_LEVELS = 11
};
static const double dd2_barrier_export_distance = 500;
static const double dd2_barrier_export_radius = 100;
static const double dd2_barrier_export_height = 180;
static const double dd2_barrier_export_bridge = 1000;
static const double dd2_barrier_export_turn = 6.28318530717958647693;

static void dd2_barrier_export_vector(dd2_vehicle_vector vector) {
    printf("[%.17g,%.17g,%.17g]", vector.x, vector.y, vector.z);
}

static void dd2_barrier_export_query(const dd2_barrier_world *world, dd2_barrier_sweep sweep,
                                     const dd2_barriers *barriers, const dd2_road *road) {
    dd2_barrier_contact contact = {0};
    dd2_barrier_statistics statistics = {0};
    const bool found = dd2_barrier_world_sweep(world, sweep, &contact, &statistics);
    printf("{\"start\":");
    dd2_barrier_export_vector(sweep.start);
    printf(",\"end\":");
    dd2_barrier_export_vector(sweep.end);
    printf(",\"radius\":%.17g,\"half_height\":%.17g,\"found\":%d,\"time\":%.17g,"
           "\"penetration\":%.17g,\"normal\":",
           sweep.radius, sweep.half_height, (int)found, contact.time, contact.penetration);
    dd2_barrier_export_vector(contact.normal);
    printf(",\"point\":");
    dd2_barrier_export_vector(contact.point);
    uint32_t offset = 0;
    unsigned side = 0;
    if (found && dd2_barriers_count(barriers) != 0) {
        const dd2_barrier_segment segment = dd2_barriers_segments(barriers)[contact.barrier];
        offset = dd2_road_strips(road)[segment.strip].source_offset;
        side = segment.side;
    }
    printf(",\"id\":[%u,%u],\"work\":[%zu,%zu]}", offset, side, statistics.bounds_tests,
           statistics.segment_tests);
}

static void dd2_barrier_export_queries(const dd2_barriers *barriers, const dd2_barrier_world *world,
                                       const dd2_road *road) {
    printf(",\"queries\":[");
    const dd2_barrier_segment *segments = dd2_barriers_segments(barriers);
    const size_t count = dd2_barriers_count(barriers);
    for (size_t index = 0; index < count; ++index) {
        const dd2_barrier_segment segment = segments[index];
        const double delta_x = (double)segment.end.x - (double)segment.start.x;
        const double delta_z = (double)segment.end.z - (double)segment.start.z;
        const double length = hypot(delta_x, delta_z);
        const double normal_x = length > 0 ? delta_z / length : 1;
        const double normal_z = length > 0 ? -delta_x / length : 0;
        const dd2_vehicle_vector center = {
            .x = ((double)segment.start.x + (double)segment.end.x) / 2,
            .y =
                (((double)segment.start.y + (double)segment.end.y) / 2) + dd2_barrier_export_height,
            .z = ((double)segment.start.z + (double)segment.end.z) / 2};
        for (unsigned mode = 0; mode < 2; ++mode) {
            printf("%s", index == 0 && mode == 0 ? "" : ",");
            const double height = mode == 0 ? 0 : dd2_barrier_export_bridge;
            dd2_barrier_export_query(
                world,
                (dd2_barrier_sweep){
                    .start = {.x = center.x + (normal_x * dd2_barrier_export_distance),
                              .y = center.y + height,
                              .z = center.z + (normal_z * dd2_barrier_export_distance)},
                    .end = {.x = center.x - (normal_x * dd2_barrier_export_distance),
                            .y = center.y + height,
                            .z = center.z - (normal_z * dd2_barrier_export_distance)},
                    .radius = dd2_barrier_export_radius,
                    .half_height = dd2_barrier_export_radius},
                barriers, road);
        }
    }
    if (count == 0) {
        const double radius = dd2_barriers_radius(barriers);
        for (unsigned angle = 0; angle < DD2_BARRIER_EXPORT_ANGLES; ++angle) {
            printf("%s", angle == 0 ? "" : ",");
            const double radians =
                dd2_barrier_export_turn * (double)angle / (double)DD2_BARRIER_EXPORT_ANGLES;
            const double start = radius - dd2_barrier_export_distance;
            const double end = radius + dd2_barrier_export_distance;
            dd2_barrier_export_query(world,
                                     (dd2_barrier_sweep){.start = {.x = start * cos(radians),
                                                                   .y = dd2_barrier_export_bridge,
                                                                   .z = start * sin(radians)},
                                                         .end = {.x = end * cos(radians),
                                                                 .y = dd2_barrier_export_bridge,
                                                                 .z = end * sin(radians)},
                                                         .radius = dd2_barrier_export_radius,
                                                         .half_height = dd2_barrier_export_radius},
                                     barriers, road);
        }
    }
    printf("]}");
}

static bool dd2_barrier_export_level(const dd2_archive *archive, unsigned number) {
    const char codes[] = "0123456789AB";
    char name[] = "LEV0\\LEVEL.DAT";
    name[3] = codes[number];
    dd2_level_data level = {0};
    if (!dd2_level_decode(dd2_find_view(archive, name), &level)) {
        return false;
    }
    dd2_road *road = dd2_road_create(&level, number <= DD2_BARRIER_EXPORT_RACING ? DD2_ROAD_RACING
                                                                                 : DD2_ROAD_ARENA);
    dd2_barriers *barriers = dd2_barriers_create(road, number);
    dd2_barrier_world *world = dd2_barrier_world_create(barriers);
    if (world == NULL) {
        dd2_barriers_destroy(barriers);
        dd2_road_destroy(road);
        return false;
    }
    printf("%s{\"level\":\"%c\",\"radius\":%.17g,\"segments\":[", number == 1 ? "" : ",",
           codes[number], dd2_barriers_radius(barriers));
    const dd2_barrier_segment *segments = dd2_barriers_segments(barriers);
    for (size_t index = 0; index < dd2_barriers_count(barriers); ++index) {
        const dd2_barrier_segment segment = segments[index];
        printf("%s[%u,%u,%d,%d,%d,%d,%d,%d]", index == 0 ? "" : ",",
               dd2_road_strips(road)[segment.strip].source_offset, segment.side, segment.start.x,
               segment.start.y, segment.start.z, segment.end.x, segment.end.y, segment.end.z);
    }
    printf("]");
    dd2_barrier_export_queries(barriers, world, road);
    dd2_barrier_world_destroy(world);
    dd2_barriers_destroy(barriers);
    dd2_road_destroy(road);
    return true;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        return EXIT_FAILURE;
    }
    dd2_archive_fixture fixture = {0};
    if (!dd2_archive_fixture_open(argv[1], &fixture)) {
        return EXIT_FAILURE;
    }
    bool passed = true;
    printf("[");
    for (unsigned number = 1; number <= DD2_BARRIER_EXPORT_LEVELS && passed; ++number) {
        passed = dd2_barrier_export_level(fixture.archive, number);
    }
    printf("]\n");
    dd2_archive_fixture_close(&fixture);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
