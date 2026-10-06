#include "assets/archive.h"
#include "assets/barriers.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "game/driving.h"
#include "physics/barrier_world.h"
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
    DD2_FLEET_PROBE_RACING = 7,
    DD2_FLEET_PROBE_CARS = 20,
    DD2_FLEET_PROBE_FRAMES = 120,
    DD2_FLEET_PROBE_SAMPLE = 20,
    DD2_FLEET_PROBE_PARTS = 5,
    DD2_FLEET_PROBE_WORLD_STEPS = 64
};
static const double dd2_fleet_probe_arena_steer = 0.4;
static const double dd2_fleet_probe_drop = 500;
static const double dd2_fleet_probe_hit_speed = 5000;
static const double dd2_fleet_probe_world_height = 190;
static const double dd2_fleet_probe_arena_height = 1000;

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

static void dd2_fleet_probe_contacts(const dd2_vehicle_collision_report *report, unsigned step,
                                     bool world) {
    if (report->count == 0) {
        return;
    }
    printf("{\"contact_report\":true,\"world\":%d,\"step\":%u,\"pairs\":%u,\"impacts\":[",
           (int)world, step, report->pair_contacts);
    for (unsigned slot = 0; slot < DD2_FLEET_PROBE_CARS; ++slot) {
        const dd2_vehicle_impact impact = report->impacts[slot];
        printf("%s[%u,%u,%.17g,%.17g,%.17g,%.17g,%.17g]", slot == 0 ? "" : ",", impact.contacts,
               impact.pair_contacts, impact.normal_speed, impact.impulse, impact.point.x,
               impact.point.y, impact.point.z);
    }
    printf("],\"events\":[");
    for (unsigned index = 0; index < report->count; ++index) {
        const dd2_vehicle_contact contact = report->contacts[index];
        printf("%s[%u,%u,%u,%u,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,"
               "%.17g,%.17g,%.17g,%.17g,%.17g,%.17g]",
               index == 0 ? "" : ",", (unsigned)contact.kind, contact.first, contact.second,
               contact.obstacle, contact.time, contact.normal_speed, contact.impulse,
               contact.point.x, contact.point.y, contact.point.z, contact.normal.x,
               contact.normal.y, contact.normal.z, contact.local_points[0].x,
               contact.local_points[0].y, contact.local_points[0].z, contact.local_points[1].x,
               contact.local_points[1].y, contact.local_points[1].z);
    }
    puts("]}");
}

static bool dd2_fleet_probe_world_hit(dd2_vehicle previous, const dd2_road_surface *surface,
                                      const dd2_barrier_world *world, unsigned sample) {
    for (unsigned step = 0; step < DD2_FLEET_PROBE_WORLD_STEPS; ++step) {
        dd2_vehicle next = previous;
        next.position.x += previous.velocity.x * DD2_VEHICLE_STEP_SECONDS;
        next.position.y += previous.velocity.y * DD2_VEHICLE_STEP_SECONDS;
        next.position.z += previous.velocity.z * DD2_VEHICLE_STEP_SECONDS;
        dd2_vehicle_collision_report report = {0};
        if (!dd2_vehicle_collide_fleet_report(&next, &previous, 1, surface, world, &report)) {
            return false;
        }
        if (report.impacts[0].normal_speed > 0) {
            dd2_fleet_probe_contacts(&report, sample, true);
            return true;
        }
        previous = next;
    }
    return false;
}

static bool dd2_fleet_probe_landing(const dd2_driving *driving, const dd2_road_surface *surface) {
    dd2_vehicle previous = {0};
    if (!dd2_vehicle_reset(&previous, *dd2_driving_grid_start(driving, 0))) {
        return false;
    }
    previous.position.y += dd2_fleet_probe_drop;
    previous.rotation = (dd2_vehicle_rotation){.x = previous.rotation.y, .z = previous.rotation.w};
    previous.velocity.y = -dd2_fleet_probe_hit_speed;
    return dd2_fleet_probe_world_hit(previous, surface, NULL, 1);
}

static bool dd2_fleet_probe_wall(const dd2_barriers *barriers, const dd2_barrier_world *world) {
    const double radius = dd2_barriers_radius(barriers);
    dd2_vehicle_vector point = {.y = dd2_fleet_probe_arena_height, .z = -radius};
    dd2_vehicle_vector inward = {.z = 1};
    if (radius == 0) {
        const dd2_barrier_segment *segments = dd2_barriers_segments(barriers);
        bool found = false;
        for (size_t index = 0; index < dd2_barriers_count(barriers) && !found; ++index) {
            const dd2_barrier_segment segment = segments[index];
            const double xpos = (double)segment.end.x - segment.start.x;
            const double zpos = (double)segment.end.z - segment.start.z;
            const double length = hypot(xpos, zpos);
            if (length > 0) {
                point = (dd2_vehicle_vector){.x = ((double)segment.start.x + segment.end.x) / 2,
                                             .y = (((double)segment.start.y + segment.end.y) / 2) +
                                                  dd2_fleet_probe_world_height,
                                             .z = ((double)segment.start.z + segment.end.z) / 2};
                inward = (dd2_vehicle_vector){.x = zpos / length, .z = -xpos / length};
                found = true;
            }
        }
        if (!found) {
            return false;
        }
    }
    dd2_vehicle previous = {0};
    if (!dd2_vehicle_reset(
            &previous,
            (dd2_vehicle_spawn){.position = {.x = point.x + (inward.x * dd2_fleet_probe_drop),
                                             .y = point.y,
                                             .z = point.z + (inward.z * dd2_fleet_probe_drop)},
                                .yaw = atan2(-inward.x, -inward.z)})) {
        return false;
    }
    previous.velocity = (dd2_vehicle_vector){.x = -inward.x * dd2_fleet_probe_hit_speed,
                                             .z = -inward.z * dd2_fleet_probe_hit_speed};
    return dd2_fleet_probe_world_hit(previous, NULL, world, 2);
}

static bool dd2_fleet_probe_world(const dd2_road *road, const dd2_driving *driving,
                                  unsigned level) {
    dd2_road_surface *surface = dd2_road_surface_create(road);
    dd2_barriers *barriers = dd2_barriers_create(road, level);
    dd2_barrier_world *world = dd2_barrier_world_create(barriers);
    const bool valid = surface != NULL && world != NULL &&
                       dd2_fleet_probe_landing(driving, surface) &&
                       dd2_fleet_probe_wall(barriers, world);
    dd2_barrier_world_destroy(world);
    dd2_barriers_destroy(barriers);
    dd2_road_surface_destroy(surface);
    return valid;
}

int main(int argc, char **argv) {
    if (argc != 3 || strlen(argv[2]) != 1 || strchr("123456789AB", argv[2][0]) == NULL) {
        return EXIT_FAILURE;
    }
    const char code = argv[2][0];
    const unsigned level = code <= '9' ? (unsigned)(code - '0') : (unsigned)(code - 'A') + 10;
    dd2_road *road = dd2_fleet_probe_load(argv[1], code);
    dd2_driving *driving = dd2_driving_create(road, level);
    dd2_driving_set_opponents(driving, false);
    bool valid = driving != NULL && dd2_driving_vehicle_count(driving) == DD2_FLEET_PROBE_CARS;
    if (valid) {
        dd2_fleet_probe_state(driving, 0);
    }
    for (unsigned frame = 0; frame < DD2_FLEET_PROBE_FRAMES && valid; ++frame) {
        const dd2_vehicle_control control =
            level <= DD2_FLEET_PROBE_RACING
                ? (dd2_vehicle_control){.throttle = -1}
                : (dd2_vehicle_control){.throttle = 1, .steer = dd2_fleet_probe_arena_steer};
        for (unsigned part = 0; part < DD2_FLEET_PROBE_PARTS && valid; ++part) {
            valid = dd2_driving_advance(
                driving,
                (dd2_driving_frame){.seconds = DD2_VEHICLE_STEP_SECONDS, .control = control});
            if (valid) {
                dd2_fleet_probe_contacts(dd2_driving_contact_report(driving),
                                         (frame * DD2_FLEET_PROBE_PARTS) + part + 1, false);
            }
        }
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
                dd2_driving_collisions(driving) == 0 &&
                dd2_driving_contact_report(driving)->count == 0;
        if (valid) {
            dd2_fleet_probe_state(driving, 0);
        }
    }
    valid = valid && dd2_fleet_probe_world(road, driving, level);
    dd2_driving_destroy(driving);
    dd2_road_destroy(road);
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
