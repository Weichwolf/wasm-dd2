#include "assets/archive.h"
#include "assets/barriers.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "game/recovery.h"
#include "game/starting_grid.h"
#include "physics/barrier_world.h"
#include "physics/damage.h"
#include "physics/road_contact.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"
#include "platform/file.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_RECOVERY_PROBE_RACING = 7,
    DD2_RECOVERY_PROBE_STEPS = 2000,
    DD2_RECOVERY_PROBE_DRIVE = 100
};
static const double dd2_recovery_probe_drop = 500;

static dd2_road *dd2_recovery_probe_load(const char *path, char code) {
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
        road = dd2_road_create(&level, code <= '0' + DD2_RECOVERY_PROBE_RACING ? DD2_ROAD_RACING
                                                                               : DD2_ROAD_ARENA);
    }
    dd2_archive_close(archive);
    dd2_file_release(&file);
    return road;
}

static void dd2_recovery_probe_vector(dd2_vehicle_vector vector) {
    printf("[%.17g,%.17g,%.17g]", vector.x, vector.y, vector.z);
}

static void dd2_recovery_probe_pose(const dd2_vehicle *vehicle) {
    printf("{\"position\":");
    dd2_recovery_probe_vector(vehicle->position);
    printf(",\"velocity\":");
    dd2_recovery_probe_vector(vehicle->velocity);
    printf(",\"rotation\":[%.17g,%.17g,%.17g,%.17g],\"angular\":", vehicle->rotation.x,
           vehicle->rotation.y, vehicle->rotation.z, vehicle->rotation.w);
    dd2_recovery_probe_vector(vehicle->angular_velocity);
    printf(",\"steps\":%llu}", (unsigned long long)vehicle->steps);
}

static void dd2_recovery_probe_wheels(const dd2_vehicle *vehicle, const dd2_road *road) {
    printf(",\"wheels\":[");
    for (unsigned index = 0; index < DD2_VEHICLE_WHEELS; ++index) {
        const dd2_vehicle_wheel wheel = vehicle->wheels[index];
        printf("%s{\"grounded\":%d,\"mount\":", index == 0 ? "" : ",", (int)wheel.grounded);
        dd2_recovery_probe_vector(wheel.mount);
        printf(",\"center\":");
        dd2_recovery_probe_vector(wheel.center);
        if (wheel.grounded) {
            const dd2_road_cell cell = dd2_road_cells(road)[wheel.contact.cell];
            const unsigned source = cell.strip == DD2_ROAD_NO_STRIP
                                        ? DD2_ROAD_NO_STRIP
                                        : dd2_road_strips(road)[cell.strip].source_offset;
            printf(",\"key\":[%u,%u],\"triangle\":%u,\"height\":%.17g,\"normal\":[%.17g,%."
                   "17g,%.17g]",
                   source, cell.lane, wheel.contact.triangle, wheel.contact.height,
                   wheel.contact.normal[0], wheel.contact.normal[1], wheel.contact.normal[2]);
        }
        printf("}");
    }
    printf("]");
}

static bool dd2_recovery_probe_slot(dd2_grid_start start, unsigned slot, dd2_recovery_frame frame) {
    dd2_vehicle vehicle = {0};
    if (!dd2_vehicle_reset(&vehicle, start.spawn)) {
        return false;
    }
    /* A controlled roof-down drop at each original grid position; all settling,
     * support detection, timing and righting thereafter use production physics. */
    vehicle.position.y += dd2_recovery_probe_drop;
    vehicle.rotation =
        (dd2_vehicle_rotation){.x = sin(start.spawn.yaw / 2), .z = cos(start.spawn.yaw / 2)};
    dd2_recovery_driver state = {0};
    unsigned restarts = 0;
    for (unsigned tick = 1; tick <= DD2_RECOVERY_PROBE_STEPS; ++tick) {
        const dd2_vehicle previous = vehicle;
        if (!dd2_vehicle_step(&vehicle, frame.road, frame.surface, (dd2_vehicle_control){0}) ||
            !dd2_vehicle_collide_world(&vehicle, &previous, frame.surface, frame.world, NULL)) {
            return false;
        }
        const dd2_vehicle before = vehicle;
        const unsigned rest = state.rest_steps;
        if (!dd2_recovery_step(&state, &vehicle, frame) || vehicle.steps != tick) {
            return false;
        }
        restarts += (unsigned)(rest != 0 && state.rest_steps == 0 && state.recoveries == 0);
        if (state.recoveries == 0) {
            continue;
        }
        printf("{\"slot\":%u,\"tick\":%u,\"prior_rest\":%u,\"restarts\":%u,\"state\":[%u,%llu,%d],"
               "\"spawn\":[%.17g,%.17g,%.17g,%.17g],\"before\":",
               slot, tick, rest, restarts, state.rest_steps, (unsigned long long)state.recoveries,
               (int)state.overturned, start.spawn.position.x, start.spawn.position.y,
               start.spawn.position.z, start.spawn.yaw);
        dd2_recovery_probe_pose(&before);
        printf(",\"landed\":");
        dd2_recovery_probe_pose(&vehicle);
        dd2_recovery_probe_wheels(&vehicle, frame.road);
        for (unsigned step = 0; step < DD2_RECOVERY_PROBE_DRIVE; ++step) {
            const dd2_vehicle landed = vehicle;
            if (!dd2_vehicle_step(&vehicle, frame.road, frame.surface,
                                  (dd2_vehicle_control){.throttle = 1}) ||
                !dd2_vehicle_collide_world(&vehicle, &landed, frame.surface, frame.world, NULL) ||
                !dd2_recovery_step(&state, &vehicle, frame)) {
                return false;
            }
        }
        printf(",\"driven\":");
        dd2_recovery_probe_pose(&vehicle);
        puts("}");
        return state.recoveries == 1 && !state.overturned;
    }
    printf("No supported recovery: slot=%u rest=%u pos=(%.17g,%.17g,%.17g)\n", slot,
           state.rest_steps, vehicle.position.x, vehicle.position.y, vehicle.position.z);
    return false;
}

int main(int argc, char **argv) {
    if (argc != 3 || strlen(argv[2]) != 1 || strchr("123456789AB", argv[2][0]) == NULL) {
        return EXIT_FAILURE;
    }
    const char code = argv[2][0];
    const unsigned level = code >= 'A' ? (unsigned)(code - 'A') + 10U : (unsigned)(code - '0');
    dd2_road *road = dd2_recovery_probe_load(argv[1], code);
    dd2_road_surface *surface = dd2_road_surface_create(road);
    dd2_barriers *barriers = dd2_barriers_create(road, level);
    dd2_barrier_world *world = dd2_barrier_world_create(barriers);
    dd2_grid_start starts[DD2_VEHICLE_FLEET_LIMIT] = {0};
    const dd2_vehicle_damage damage = {.regions = {0.5}};
    const dd2_recovery_frame frame = {
        .road = road, .surface = surface, .world = world, .damage = &damage, .count = 1};
    bool passed = surface != NULL && world != NULL &&
                  dd2_starting_grid(road, surface, level, DD2_VEHICLE_FLEET_LIMIT, starts);
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT && passed; ++slot) {
        passed = dd2_recovery_probe_slot(starts[slot], slot, frame);
    }
    dd2_barrier_world_destroy(world);
    dd2_barriers_destroy(barriers);
    dd2_road_surface_destroy(surface);
    dd2_road_destroy(road);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
