#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "game/driving.h"
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
    DD2_GROUND_PROBE_RACING = 7,
    DD2_GROUND_PROBE_SEEDS = 4,
    DD2_GROUND_PROBE_STEPS = 600,
    DD2_GROUND_PROBE_SAMPLE = 20
};
static const double dd2_ground_probe_short = 25;
static const double dd2_ground_probe_long = 2000;
static const double dd2_ground_probe_drop = 1000;
static const double dd2_ground_probe_half = 0.7071067811865475244;
static const double dd2_ground_probe_slide = 1000;
static const double dd2_ground_probe_spin = 2;

static dd2_road *dd2_ground_probe_load(const char *path, char code) {
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
        road = dd2_road_create(&level, code <= '0' + DD2_GROUND_PROBE_RACING ? DD2_ROAD_RACING
                                                                             : DD2_ROAD_ARENA);
    }
    dd2_archive_close(archive);
    dd2_file_release(&file);
    return road;
}

static void dd2_ground_probe_key(const dd2_road *road, size_t cell) {
    const dd2_road_cell geometry = dd2_road_cells(road)[cell];
    const uint32_t source = geometry.strip == DD2_ROAD_NO_STRIP
                                ? DD2_ROAD_NO_STRIP
                                : dd2_road_strips(road)[geometry.strip].source_offset;
    printf("[%u,%u]", source, geometry.lane);
}

static void dd2_ground_probe_sweeps(const dd2_road *road, const dd2_road_surface *surface) {
    static const size_t corners[2][3] = {{0, 1, 3}, {2, 3, 1}};
    for (size_t cell = 0; cell < dd2_road_cell_count(road); ++cell) {
        const dd2_road_cell geometry = dd2_road_cells(road)[cell];
        for (unsigned triangle = 0; triangle < 2; ++triangle) {
            dd2_road_position center = {0};
            for (size_t corner = 0; corner < 3; ++corner) {
                const dd2_track_vertex vertex =
                    dd2_road_vertices(road)[geometry.vertices[corners[triangle][corner]]];
                center.x += (double)vertex.x / 3;
                center.y += (double)vertex.y / 3;
                center.z += (double)vertex.z / 3;
            }
            for (unsigned window = 0; window < 2; ++window) {
                const double half = window == 0 ? dd2_ground_probe_short : dd2_ground_probe_long;
                const dd2_surface_sweep sweep = {
                    .start = {.x = center.x, .y = center.y + half, .z = center.z},
                    .end = {.x = center.x, .y = center.y - half, .z = center.z}};
                dd2_surface_hit hit = {0};
                dd2_surface_statistics stats = {0};
                const bool found = dd2_road_surface_sweep(surface, sweep, &hit, &stats);
                printf("{\"kind\":\"query\",\"source\":");
                dd2_ground_probe_key(road, cell);
                printf(",\"triangle\":%u,\"window\":%u,\"found\":%d,\"key\":", triangle, window,
                       (int)found);
                dd2_ground_probe_key(road, hit.road.cell);
                printf(",\"hit_triangle\":%u,\"time\":%.17g,\"height\":%.17g,\"normal\":[%.17g,%."
                       "17g,%.17g],\"tests\":%zu}\n",
                       hit.road.triangle, hit.time, hit.road.height, hit.road.normal[0],
                       hit.road.normal[1], hit.road.normal[2], stats.cell_tests);
            }
        }
    }
}

static bool dd2_ground_probe_dynamics(const dd2_road *road, const dd2_road_surface *surface,
                                      unsigned level) {
    dd2_driving *driving = dd2_driving_create(road, level);
    if (driving == NULL) {
        return false;
    }
    const dd2_vehicle settled = *dd2_driving_vehicle(driving);
    dd2_driving_destroy(driving);
    for (unsigned seed = 0; seed < DD2_GROUND_PROBE_SEEDS; ++seed) {
        dd2_vehicle vehicle = settled;
        const dd2_vehicle_rotation rotation = vehicle.rotation;
        if (seed == 1 || seed == 2) {
            vehicle.rotation = (dd2_vehicle_rotation){
                .x = rotation.y, .y = -rotation.x, .z = rotation.w, .w = -rotation.z};
        } else if (seed != 0) {
            vehicle.rotation =
                (dd2_vehicle_rotation){.x = (rotation.x + rotation.y) * dd2_ground_probe_half,
                                       .y = (rotation.y - rotation.x) * dd2_ground_probe_half,
                                       .z = (rotation.z + rotation.w) * dd2_ground_probe_half,
                                       .w = (rotation.w - rotation.z) * dd2_ground_probe_half};
            vehicle.angular_velocity.x = dd2_ground_probe_spin;
            vehicle.angular_velocity.z = dd2_ground_probe_spin;
        }
        vehicle.position.y += dd2_ground_probe_drop;
        if (seed == 2) {
            vehicle.velocity.x = dd2_ground_probe_slide;
        }
        unsigned total = 0;
        for (unsigned step = 0; step < DD2_GROUND_PROBE_STEPS; ++step) {
            const dd2_vehicle previous = vehicle;
            dd2_vehicle_impact impact = {0};
            if (!dd2_vehicle_step(&vehicle, road, surface, (dd2_vehicle_control){0}) ||
                !dd2_vehicle_collide_world(&vehicle, &previous, surface, NULL, &impact)) {
                printf("Body contact rejected level %u seed %u step %u\n", level, seed, step);
                return false;
            }
            total += impact.contacts;
            if ((step + 1) % DD2_GROUND_PROBE_SAMPLE == 0) {
                printf(
                    "{\"kind\":\"state\",\"seed\":%u,\"step\":%u,\"contacts\":%u,\"values\":[%.17g,"
                    "%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g]}\n",
                    seed, step + 1, total, vehicle.position.x, vehicle.position.y,
                    vehicle.position.z, vehicle.velocity.x, vehicle.velocity.y, vehicle.velocity.z,
                    vehicle.rotation.x, vehicle.rotation.y, vehicle.rotation.z, vehicle.rotation.w,
                    vehicle.angular_velocity.x, vehicle.angular_velocity.y,
                    vehicle.angular_velocity.z);
            }
        }
        if (seed != 0 && total == 0) {
            return false;
        }
    }
    return true;
}

int main(int argc, char **argv) {
    if (argc != 4 || strlen(argv[2]) != 1 || strchr("123456789AB", argv[2][0]) == NULL ||
        (strcmp(argv[3], "sweep") != 0 && strcmp(argv[3], "drive") != 0)) {
        return EXIT_FAILURE;
    }
    const char code = argv[2][0];
    const unsigned level = code <= '9' ? (unsigned)(code - '0') : (unsigned)(code - 'A') + 10;
    dd2_road *road = dd2_ground_probe_load(argv[1], code);
    dd2_road_surface *surface = dd2_road_surface_create(road);
    bool valid = surface != NULL;
    if (valid && strcmp(argv[3], "sweep") == 0) {
        for (unsigned corner = 0; corner < DD2_VEHICLE_BODY_CORNERS; ++corner) {
            const dd2_vehicle_vector point = dd2_vehicle_body_corner(corner);
            printf("{\"kind\":\"corner\",\"values\":[%.17g,%.17g,%.17g]}\n", point.x, point.y,
                   point.z);
        }
        dd2_ground_probe_sweeps(road, surface);
    } else if (valid) {
        valid = dd2_ground_probe_dynamics(road, surface, level);
    }
    dd2_road_surface_destroy(surface);
    dd2_road_destroy(road);
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
