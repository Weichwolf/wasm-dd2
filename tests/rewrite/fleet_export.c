#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "game/driving.h"
#include "physics/vehicle.h"
#include "platform/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_FLEET_PROBE_RACING = 7,
    DD2_FLEET_PROBE_CARS = 20,
    DD2_FLEET_PROBE_FRAMES = 120,
    DD2_FLEET_PROBE_SAMPLE = 20
};
static const double dd2_fleet_probe_seconds = 0.025;
static const double dd2_fleet_probe_arena_steer = 0.4;

static dd2_road *dd2_fleet_probe_load(const char *path, char code) {
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
        road = dd2_road_create(&level, code <= '0' + DD2_FLEET_PROBE_RACING ? DD2_ROAD_RACING
                                                                            : DD2_ROAD_ARENA);
    }
    dd2_archive_close(archive);
    dd2_file_release(&file);
    return road;
}

static void dd2_fleet_probe_state(const dd2_driving *driving, unsigned frame) {
    const dd2_vehicle *vehicles = dd2_driving_vehicles(driving);
    const double *rolls = dd2_driving_wheel_rolls(driving);
    for (unsigned slot = 0; slot < dd2_driving_vehicle_count(driving); ++slot) {
        const dd2_vehicle *vehicle = &vehicles[slot];
        const dd2_vehicle_spawn *spawn = dd2_driving_grid_start(driving, slot);
        unsigned grounded = 0;
        for (unsigned wheel = 0; wheel < DD2_VEHICLE_WHEELS; ++wheel) {
            grounded += (unsigned)vehicle->wheels[wheel].grounded;
        }
        printf("{\"slot\":%u,\"frame\":%u,\"spawn\":[%.17g,%.17g,%.17g,%.17g],\"state\":[%.17g,%."
               "17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g],"
               "\"grounded\":%u,\"steps\":%llu}\n",
               slot, frame, spawn->position.x, spawn->position.y, spawn->position.z, spawn->yaw,
               vehicle->position.x, vehicle->position.y, vehicle->position.z, vehicle->velocity.x,
               vehicle->velocity.y, vehicle->velocity.z, vehicle->rotation.x, vehicle->rotation.y,
               vehicle->rotation.z, vehicle->rotation.w, vehicle->angular_velocity.x,
               vehicle->angular_velocity.y, vehicle->angular_velocity.z, rolls[slot], grounded,
               (unsigned long long)vehicle->steps);
    }
}

int main(int argc, char **argv) {
    if (argc != 3 || strlen(argv[2]) != 1 || strchr("123456789AB", argv[2][0]) == NULL) {
        return EXIT_FAILURE;
    }
    const char code = argv[2][0];
    const unsigned level = code <= '9' ? (unsigned)(code - '0') : (unsigned)(code - 'A') + 10;
    dd2_road *road = dd2_fleet_probe_load(argv[1], code);
    dd2_driving *driving = dd2_driving_create(road, level);
    bool valid = driving != NULL && dd2_driving_vehicle_count(driving) == DD2_FLEET_PROBE_CARS;
    if (valid) {
        dd2_fleet_probe_state(driving, 0);
    }
    for (unsigned frame = 0; frame < DD2_FLEET_PROBE_FRAMES && valid; ++frame) {
        const dd2_vehicle_control control =
            level <= DD2_FLEET_PROBE_RACING
                ? (dd2_vehicle_control){.throttle = -1}
                : (dd2_vehicle_control){.throttle = 1, .steer = dd2_fleet_probe_arena_steer};
        valid = dd2_driving_advance(
            driving, (dd2_driving_frame){.seconds = dd2_fleet_probe_seconds, .control = control});
        if (valid && (frame + 1) % DD2_FLEET_PROBE_SAMPLE == 0) {
            dd2_fleet_probe_state(driving, frame + 1);
        }
    }
    if (valid) {
        printf("{\"summary\":true,\"vehicles\":%u,\"pairs\":%llu,\"contacts\":%llu}\n",
               dd2_driving_vehicle_count(driving),
               (unsigned long long)dd2_driving_pair_collisions(driving),
               (unsigned long long)dd2_driving_collisions(driving));
        valid = dd2_driving_reset(driving) && dd2_driving_pair_collisions(driving) == 0 &&
                dd2_driving_collisions(driving) == 0;
        if (valid) {
            dd2_fleet_probe_state(driving, 0);
        }
    }
    dd2_driving_destroy(driving);
    dd2_road_destroy(road);
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
