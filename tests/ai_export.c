#include "ai/driver.h"
#include "ai/path.h"
#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "game/driving.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"
#include "platform/file.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_AI_PROBE_RACING = 7, DD2_AI_PROBE_FRAMES = 2400, DD2_AI_PROBE_SAMPLE = 200 };
static const double dd2_ai_probe_seconds = 0.025;

static dd2_road *dd2_ai_probe_load(const char *path, char code) {
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
        road = dd2_road_create(&level, code <= '0' + DD2_AI_PROBE_RACING ? DD2_ROAD_RACING
                                                                         : DD2_ROAD_ARENA);
    }
    dd2_archive_close(archive);
    dd2_file_release(&file);
    return road;
}

static void dd2_ai_probe_paths(const dd2_road *road) {
    static const double lanes[] = {0.25, 0.5, 0.75};
    static const double distances[] = {0, 1000, 5000};
    for (size_t strip = 0; strip < dd2_road_strip_count(road); ++strip) {
        const dd2_road_strip geometry = dd2_road_strips(road)[strip];
        const dd2_road_cell cell = dd2_road_cells(road)[geometry.first_cell];
        dd2_vehicle_vector position = {0};
        for (unsigned corner = 0; corner < DD2_ROAD_CORNERS; ++corner) {
            const dd2_track_vertex vertex = dd2_road_vertices(road)[cell.vertices[corner]];
            position.x += (double)vertex.x / (double)DD2_ROAD_CORNERS;
            position.y += (double)vertex.y / (double)DD2_ROAD_CORNERS;
            position.z += (double)vertex.z / (double)DD2_ROAD_CORNERS;
        }
        for (unsigned lane = 0; lane < 3; ++lane) {
            for (unsigned distance = 0; distance < 3; ++distance) {
                dd2_ai_path_sample sample = {0};
                const bool found = dd2_ai_path(road,
                                               (dd2_ai_path_query){.cell = geometry.first_cell,
                                                                   .lane = lanes[lane],
                                                                   .distance = distances[distance],
                                                                   .position = position},
                                               &sample);
                const uint32_t source =
                    found ? dd2_road_strips(road)[sample.strip].source_offset : DD2_ROAD_NO_STRIP;
                printf("{\"source\":%u,\"lane\":%.17g,\"distance\":%.17g,\"position\":[%.17g,%.17g,"
                       "%.17g],"
                       "\"found\":%d,\"hit\":%u,\"point\":[%.17g,%.17g,%.17g],\"direction\":[%.17g,"
                       "%.17g],"
                       "\"width\":%.17g,\"curvature\":%.17g}\n",
                       geometry.source_offset, lanes[lane], distances[distance], position.x,
                       position.y, position.z, (int)found, source, sample.point.x, sample.point.y,
                       sample.point.z, sample.direction.x, sample.direction.z, sample.width,
                       sample.curvature);
            }
        }
    }
}

static void dd2_ai_probe_state(const dd2_driving *driving, unsigned frame) {
    const dd2_vehicle *vehicles = dd2_driving_vehicles(driving);
    const dd2_ai_driver *drivers = dd2_driving_drivers(driving);
    for (unsigned slot = 0; slot < dd2_driving_vehicle_count(driving); ++slot) {
        const dd2_vehicle *vehicle = &vehicles[slot];
        const dd2_ai_driver *driver = &drivers[slot];
        printf("{\"slot\":%u,\"frame\":%u,\"position\":[%.17g,%.17g,%.17g],\"velocity\":[%.17g,%."
               "17g,%.17g],"
               "\"cell\":%u,\"lane\":%.17g,\"target\":%u,\"reverse\":%u,\"decisions\":%llu}\n",
               slot, frame, vehicle->position.x, vehicle->position.y, vehicle->position.z,
               vehicle->velocity.x, vehicle->velocity.y, vehicle->velocity.z, driver->cell,
               driver->lane, driver->target, driver->reverse_steps,
               (unsigned long long)driver->steps);
    }
}

static bool dd2_ai_probe_drive(const dd2_road *road, unsigned level) {
    dd2_driving *driving = dd2_driving_create(road, level);
    dd2_driving_set_damage(driving, false);
    if (driving == NULL || !dd2_driving_opponents(driving)) {
        dd2_driving_destroy(driving);
        return false;
    }
    double travel[DD2_VEHICLE_FLEET_LIMIT] = {0};
    unsigned supported[DD2_VEHICLE_FLEET_LIMIT] = {0};
    unsigned reverse[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_ai_probe_state(driving, 0);
    bool valid = true;
    for (unsigned frame = 0; frame < DD2_AI_PROBE_FRAMES && valid; ++frame) {
        dd2_vehicle_vector previous[DD2_VEHICLE_FLEET_LIMIT] = {0};
        for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
            previous[slot] = dd2_driving_vehicles(driving)[slot].position;
        }
        valid = dd2_driving_advance(
            driving, (dd2_driving_frame){.seconds = dd2_ai_probe_seconds, .control = {.brake = 1}});
        if (!valid) {
            printf("Rejected AI frame level=%u frame=%u\n", level, frame);
            break;
        }
        for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
            const dd2_vehicle *vehicle = &dd2_driving_vehicles(driving)[slot];
            travel[slot] += hypot(vehicle->position.x - previous[slot].x,
                                  vehicle->position.z - previous[slot].z);
            bool contact = false;
            for (unsigned wheel = 0; wheel < DD2_VEHICLE_WHEELS; ++wheel) {
                contact = contact || vehicle->wheels[wheel].grounded;
            }
            supported[slot] += (unsigned)contact;
            reverse[slot] += (unsigned)(dd2_driving_drivers(driving)[slot].reverse_steps != 0);
        }
        if ((frame + 1) % DD2_AI_PROBE_SAMPLE == 0) {
            dd2_ai_probe_state(driving, frame + 1);
        }
    }
    if (valid) {
        for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
            printf(
                "{\"summary\":true,\"slot\":%u,\"travel\":%.17g,\"supported\":%u,\"reverse\":%u}\n",
                slot, travel[slot], supported[slot], reverse[slot]);
        }
        valid = dd2_driving_reset(driving);
        if (valid) {
            dd2_ai_probe_state(driving, 0);
        }
    }
    dd2_driving_destroy(driving);
    return valid;
}

int main(int argc, char **argv) {
    if (argc != 4 || strlen(argv[2]) != 1 || strchr("123456789AB", argv[2][0]) == NULL ||
        (strcmp(argv[3], "path") != 0 && strcmp(argv[3], "drive") != 0)) {
        return EXIT_FAILURE;
    }
    const char code = argv[2][0];
    const unsigned level = code <= '9' ? (unsigned)(code - '0') : (unsigned)(code - 'A') + 10;
    dd2_road *road = dd2_ai_probe_load(argv[1], code);
    bool valid = road != NULL;
    if (valid && strcmp(argv[3], "path") == 0) {
        dd2_ai_probe_paths(road);
    } else if (valid) {
        valid = dd2_ai_probe_drive(road, level);
    }
    dd2_road_destroy(road);
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
