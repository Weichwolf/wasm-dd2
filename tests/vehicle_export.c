#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "platform/file.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_VEHICLE_PROBE_RACING = 7,
    DD2_VEHICLE_PROBE_SEEDS = 24,
    DD2_VEHICLE_PROBE_STEPS = 1000,
    DD2_VEHICLE_PROBE_SAMPLE = 20,
    DD2_VEHICLE_PROBE_SETTLE = 200,
    DD2_VEHICLE_PROBE_DRIVE_END = 600,
    DD2_VEHICLE_PROBE_BRAKE_END = 800,
    DD2_VEHICLE_PROBE_RIDE_HEIGHT = 190,
    DD2_VEHICLE_PROBE_DROP_HEIGHT = 400,
    DD2_VEHICLE_PROBE_SURFACE_TYPES = 4,
    DD2_VEHICLE_PROBE_FLAG_MASK = 7
};
static const double dd2_vehicle_probe_steer = 0.3;
static const double dd2_vehicle_probe_rotation_tolerance = 1e-10;

static dd2_road *dd2_vehicle_probe_load(const char *path, char code) {
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
        road = dd2_road_create(&level, code <= '0' + DD2_VEHICLE_PROBE_RACING ? DD2_ROAD_RACING
                                                                              : DD2_ROAD_ARENA);
    }
    dd2_archive_close(archive);
    dd2_file_release(&file);
    return road;
}

static dd2_vehicle_spawn dd2_vehicle_probe_spawn(const dd2_road *road, size_t index, bool drop) {
    const dd2_road_cell *cell = &dd2_road_cells(road)[index];
    const dd2_track_vertex *vertices = dd2_road_vertices(road);
    dd2_vehicle_spawn spawn = {0};
    for (size_t corner = 0; corner < DD2_ROAD_CORNERS; ++corner) {
        const dd2_track_vertex vertex = vertices[cell->vertices[corner]];
        spawn.position.x += (double)vertex.x / (double)DD2_ROAD_CORNERS;
        spawn.position.y += (double)vertex.y / (double)DD2_ROAD_CORNERS;
        spawn.position.z += (double)vertex.z / (double)DD2_ROAD_CORNERS;
    }
    const dd2_track_vertex first = vertices[cell->vertices[0]];
    const dd2_track_vertex last = vertices[cell->vertices[3]];
    spawn.yaw = atan2((double)last.x - (double)first.x, (double)last.z - (double)first.z);
    spawn.position.y += DD2_VEHICLE_PROBE_RIDE_HEIGHT + (drop ? DD2_VEHICLE_PROBE_DROP_HEIGHT : 0);
    return spawn;
}

static dd2_vehicle_control dd2_vehicle_probe_control(unsigned step) {
    if (step < DD2_VEHICLE_PROBE_SETTLE) {
        return (dd2_vehicle_control){0};
    }
    if (step < DD2_VEHICLE_PROBE_DRIVE_END) {
        return (dd2_vehicle_control){
            .throttle = 1,
            .steer = step < (DD2_VEHICLE_PROBE_SETTLE + DD2_VEHICLE_PROBE_DRIVE_END) / 2
                         ? dd2_vehicle_probe_steer
                         : -dd2_vehicle_probe_steer};
    }
    if (step < DD2_VEHICLE_PROBE_BRAKE_END) {
        return (dd2_vehicle_control){.brake = 1};
    }
    return (dd2_vehicle_control){.throttle = -1, .steer = dd2_vehicle_probe_steer};
}

static void dd2_vehicle_probe_sample(const dd2_vehicle *vehicle, unsigned seed, unsigned step) {
    printf("{\"seed\":%u,\"step\":%u,\"state\":[%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,"
           "%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g],\"wheels\":[",
           seed, step, vehicle->position.x, vehicle->position.y, vehicle->position.z,
           vehicle->velocity.x, vehicle->velocity.y, vehicle->velocity.z, vehicle->rotation.x,
           vehicle->rotation.y, vehicle->rotation.z, vehicle->rotation.w,
           vehicle->angular_velocity.x, vehicle->angular_velocity.y, vehicle->angular_velocity.z,
           vehicle->steering);
    for (size_t index = 0; index < DD2_VEHICLE_WHEELS; ++index) {
        const dd2_vehicle_wheel *wheel = &vehicle->wheels[index];
        printf("%s[%u,%u,%d,%.17g,%.17g]", index == 0 ? "" : ",", wheel->contact.cell,
               wheel->contact.triangle, (int)wheel->grounded, wheel->compression, wheel->load);
    }
    puts("]}");
}

static bool dd2_vehicle_probe_run(const dd2_road *road, const dd2_road_surface *surface) {
    unsigned grounded = 0;
    unsigned airborne = 0;
    for (unsigned seed = 0; seed < DD2_VEHICLE_PROBE_SEEDS; ++seed) {
        const size_t cell = ((size_t)seed * dd2_road_cell_count(road)) / DD2_VEHICLE_PROBE_SEEDS;
        dd2_vehicle vehicle = {0};
        if (!dd2_vehicle_reset(&vehicle, dd2_vehicle_probe_spawn(road, cell, (seed & 1U) != 0))) {
            return false;
        }
        for (unsigned step = 0; step < DD2_VEHICLE_PROBE_STEPS; ++step) {
            if (!dd2_vehicle_step(&vehicle, road, surface, dd2_vehicle_probe_control(step))) {
                printf("Vehicle rejected original road seed %u cell %zu step %u\n", seed, cell,
                       step);
                return false;
            }
            const dd2_vehicle_rotation rotation = vehicle.rotation;
            const double length = (rotation.x * rotation.x) + (rotation.y * rotation.y) +
                                  (rotation.z * rotation.z) + (rotation.w * rotation.w);
            if (fabs(length - 1) > dd2_vehicle_probe_rotation_tolerance) {
                return false;
            }
            unsigned contacts = 0;
            for (size_t index = 0; index < DD2_VEHICLE_WHEELS; ++index) {
                contacts += (unsigned)vehicle.wheels[index].grounded;
            }
            grounded += (unsigned)(contacts != 0);
            airborne += (unsigned)(contacts == 0);
            if ((step + 1) % DD2_VEHICLE_PROBE_SAMPLE == 0) {
                dd2_vehicle_probe_sample(&vehicle, seed, step + 1);
            }
        }
    }
    unsigned types[DD2_VEHICLE_PROBE_SURFACE_TYPES] = {0};
    for (size_t cell = 0; cell < dd2_road_cell_count(road); ++cell) {
        ++types[(dd2_road_cells(road)[cell].surface_flags & DD2_VEHICLE_PROBE_FLAG_MASK) >> 1U];
    }
    printf("{\"summary\":true,\"seeds\":%u,\"steps\":%u,\"grounded\":%u,\"airborne\":%u,"
           "\"surface_types\":[%u,%u,%u,%u]}\n",
           DD2_VEHICLE_PROBE_SEEDS, DD2_VEHICLE_PROBE_SEEDS * DD2_VEHICLE_PROBE_STEPS, grounded,
           airborne, types[0], types[1], types[2], types[3]);
    return true;
}

int main(int argc, char **argv) {
    if (argc != 3 || strlen(argv[2]) != 1 || strchr("123456789AB", argv[2][0]) == NULL) {
        return EXIT_FAILURE;
    }
    dd2_road *road = dd2_vehicle_probe_load(argv[1], argv[2][0]);
    dd2_road_surface *surface = dd2_road_surface_create(road);
    const bool valid = surface != NULL && dd2_vehicle_probe_run(road, surface);
    dd2_road_surface_destroy(surface);
    dd2_road_destroy(road);
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
