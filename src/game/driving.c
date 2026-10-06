#include "game/driving.h"

#include "assets/barriers.h"
#include "assets/level.h"
#include "assets/road.h"
#include "physics/barrier_world.h"
#include "physics/numeric.h"
#include "physics/road_contact.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    DD2_DRIVING_RACING_LEVELS = 7,
    DD2_DRIVING_LEVELS = 11,
    DD2_DRIVING_SPLIT = 8,
    DD2_DRIVING_MERGE = 9,
    DD2_DRIVING_HEADING_ZERO = 192,
    DD2_DRIVING_HEADING_COUNT = 256,
    DD2_DRIVING_SETTLE_STEPS = 200
};
static const double dd2_driving_full_turn = 6.28318530717958647693;
static const double dd2_driving_ride_height = 190;
static const double dd2_driving_arena_start = -12000;
static const double dd2_driving_height_limit = 2147483648.0;
static const double dd2_driving_wheel_radius = 60;
static const double dd2_driving_max_frame = 0.25;
static const double dd2_driving_time_tolerance = 1e-12;

struct dd2_driving {
    const dd2_road *road;
    dd2_road_surface *surface;
    dd2_barriers *barriers;
    dd2_barrier_world *barrier_world;
    uint64_t collisions;
    dd2_vehicle_spawn spawn;
    dd2_vehicle vehicle;
    double accumulator;
    double wheel_roll;
};

/* FUN_00426be4 numbers the main loop first, then each alternate branch in
 * encounter order. The stored source_number has a different lap-progress role. */
static uint32_t dd2_driving_strip(const dd2_road *road, uint32_t number) {
    const dd2_road_strip *strips = dd2_road_strips(road);
    const size_t count = dd2_road_strip_count(road);
    for (size_t index = 0; index < count; ++index) {
        if (strips[index].main_order == number) {
            return (uint32_t)index;
        }
    }
    uint32_t current = 0;
    uint32_t generated = (uint32_t)dd2_road_main_count(road);
    for (size_t visited = 0; visited < count; ++visited) {
        if (strips[current].kind == DD2_DRIVING_SPLIT) {
            current = strips[current].branch;
            for (size_t branch = 0; branch < count; ++branch) {
                if (strips[current].kind == DD2_DRIVING_MERGE) {
                    break;
                }
                if (generated++ == number) {
                    return current;
                }
                current = strips[current].next;
            }
        }
        current = strips[current].next;
        if (current == 0) {
            break;
        }
    }
    return DD2_ROAD_NO_STRIP;
}

static bool dd2_driving_spawn(dd2_driving *driving, unsigned level) {
    static const uint32_t numbers[] = {266, 656, 535, 614, 22, 242, 449};
    static const unsigned lanes[] = {2, 1, 3, 2, 2, 2, 1};
    dd2_vehicle_spawn spawn = {0};
    if (level <= DD2_DRIVING_RACING_LEVELS) {
        const uint32_t index = dd2_driving_strip(driving->road, numbers[level - 1]);
        if (index == DD2_ROAD_NO_STRIP) {
            return false;
        }
        const dd2_road_strip *strip = &dd2_road_strips(driving->road)[index];
        const unsigned lane = lanes[level - 1];
        if (lane >= strip->lanes) {
            return false;
        }
        const dd2_road_cell *cell = &dd2_road_cells(driving->road)[strip->first_cell + lane];
        const dd2_track_vertex *vertices = dd2_road_vertices(driving->road);
        for (size_t corner = 0; corner < DD2_ROAD_CORNERS; ++corner) {
            const dd2_track_vertex vertex = vertices[cell->vertices[corner]];
            spawn.position.x += (double)vertex.x / (double)DD2_ROAD_CORNERS;
            spawn.position.y += (double)vertex.y / (double)DD2_ROAD_CORNERS;
            spawn.position.z += (double)vertex.z / (double)DD2_ROAD_CORNERS;
        }
        const uint8_t heading = (uint8_t)(DD2_DRIVING_HEADING_ZERO - strip->heading);
        spawn.yaw = (double)heading * dd2_driving_full_turn / (double)DD2_DRIVING_HEADING_COUNT;
    } else {
        if (level == DD2_DRIVING_LEVELS) {
            spawn.position.x = dd2_driving_arena_start;
            spawn.yaw = dd2_driving_full_turn / 4;
        } else {
            spawn.position.z = dd2_driving_arena_start;
        }
        dd2_road_contact contact = {0};
        if (!dd2_road_surface_sample(
                driving->surface,
                (dd2_surface_query){.point = {.x = spawn.position.x, .z = spawn.position.z},
                                    .min_height = -dd2_driving_height_limit,
                                    .max_height = dd2_driving_height_limit,
                                    .preferred_cell = DD2_ROAD_NO_STRIP},
                &contact, NULL)) {
            return false;
        }
        spawn.position.y = contact.height;
    }
    spawn.position.y += dd2_driving_ride_height;
    driving->spawn = spawn;
    return true;
}

static bool dd2_driving_align(dd2_vehicle *vehicle, const dd2_driving *driving) {
    dd2_road_contact contact = {0};
    if (!dd2_road_surface_sample(
            driving->surface,
            (dd2_surface_query){
                .point = {.x = driving->spawn.position.x, .z = driving->spawn.position.z},
                .min_height = driving->spawn.position.y - (2 * dd2_driving_ride_height),
                .max_height = driving->spawn.position.y,
                .preferred_cell = DD2_ROAD_NO_STRIP},
            &contact, NULL)) {
        return false;
    }
    /* Shortest rotation from world up to the source contact normal, followed
     * by the configured yaw. Keep the typed original spawn as reset evidence. */
    const double length = sqrt(2 * (1 + contact.normal[1]));
    const dd2_vehicle_rotation tilt = {.x = contact.normal[2] / length,
                                       .z = -contact.normal[0] / length,
                                       .w = (1 + contact.normal[1]) / length};
    const dd2_vehicle_rotation yaw = vehicle->rotation;
    vehicle->rotation = (dd2_vehicle_rotation){.x = (tilt.x * yaw.w) - (tilt.z * yaw.y),
                                               .y = tilt.w * yaw.y,
                                               .z = (tilt.x * yaw.y) + (tilt.z * yaw.w),
                                               .w = tilt.w * yaw.w};
    vehicle->position.y = contact.height + (dd2_driving_ride_height / contact.normal[1]);
    return true;
}

bool dd2_driving_reset(dd2_driving *driving) {
    if (driving == NULL) {
        return false;
    }
    dd2_vehicle vehicle = {0};
    if (!dd2_vehicle_reset(&vehicle, driving->spawn) || !dd2_driving_align(&vehicle, driving)) {
        return false;
    }
    for (unsigned step = 0; step < DD2_DRIVING_SETTLE_STEPS; ++step) {
        const dd2_vehicle previous = vehicle;
        if (!dd2_vehicle_step(&vehicle, driving->road, driving->surface,
                              (dd2_vehicle_control){.brake = 1}) ||
            !dd2_vehicle_collide_world(&vehicle, &previous, driving->surface,
                                       driving->barrier_world, NULL)) {
            return false;
        }
    }
    driving->vehicle = vehicle;
    driving->accumulator = 0;
    driving->wheel_roll = 0;
    driving->collisions = 0;
    return true;
}

dd2_driving *dd2_driving_create(const dd2_road *road, unsigned level) {
    if (road == NULL || level == 0 || level > DD2_DRIVING_LEVELS ||
        ((level <= DD2_DRIVING_RACING_LEVELS) != (dd2_road_strip_count(road) != 0))) {
        return NULL;
    }
    dd2_driving *driving = calloc(1, sizeof(*driving));
    if (driving == NULL) {
        return NULL;
    }
    driving->road = road;
    driving->surface = dd2_road_surface_create(road);
    driving->barriers = dd2_barriers_create(road, level);
    driving->barrier_world = dd2_barrier_world_create(driving->barriers);
    if (driving->surface == NULL || driving->barrier_world == NULL ||
        !dd2_driving_spawn(driving, level) || !dd2_driving_reset(driving)) {
        dd2_driving_destroy(driving);
        return NULL;
    }
    return driving;
}

void dd2_driving_destroy(dd2_driving *driving) {
    if (driving != NULL) {
        dd2_barrier_world_destroy(driving->barrier_world);
        dd2_barriers_destroy(driving->barriers);
        dd2_road_surface_destroy(driving->surface);
        free(driving);
    }
}

bool dd2_driving_advance(dd2_driving *driving, dd2_driving_frame frame) {
    if (driving == NULL || !dd2_numeric_finite(&frame.seconds) || frame.seconds < 0 ||
        frame.seconds > dd2_driving_max_frame || !dd2_numeric_finite(&frame.control.throttle) ||
        fabs(frame.control.throttle) > 1 || !dd2_numeric_finite(&frame.control.brake) ||
        frame.control.brake < 0 || frame.control.brake > 1 ||
        !dd2_numeric_finite(&frame.control.steer) || fabs(frame.control.steer) > 1) {
        return false;
    }
    dd2_vehicle vehicle = driving->vehicle;
    double accumulator = driving->accumulator + frame.seconds;
    double roll = driving->wheel_roll;
    uint64_t collisions = driving->collisions;
    const unsigned steps =
        (unsigned)floor((accumulator + dd2_driving_time_tolerance) / DD2_VEHICLE_STEP_SECONDS);
    for (unsigned step = 0; step < steps; ++step) {
        const dd2_vehicle previous = vehicle;
        dd2_vehicle_impact impact = {0};
        if (!dd2_vehicle_step(&vehicle, driving->road, driving->surface, frame.control) ||
            !dd2_vehicle_collide_world(&vehicle, &previous, driving->surface,
                                       driving->barrier_world, &impact) ||
            UINT64_MAX - collisions < impact.contacts) {
            return false;
        }
        collisions += impact.contacts;
        const dd2_vehicle_vector forward =
            dd2_vehicle_rotate(vehicle.rotation, (dd2_vehicle_vector){.z = 1});
        const double speed = (vehicle.velocity.x * forward.x) + (vehicle.velocity.y * forward.y) +
                             (vehicle.velocity.z * forward.z);
        roll = fmod(roll + (speed * DD2_VEHICLE_STEP_SECONDS / dd2_driving_wheel_radius),
                    dd2_driving_full_turn);
    }
    accumulator = fmax(0, accumulator - ((double)steps * DD2_VEHICLE_STEP_SECONDS));
    driving->vehicle = vehicle;
    driving->accumulator = accumulator;
    driving->wheel_roll = roll;
    driving->collisions = collisions;
    return true;
}

void dd2_driving_suspend(dd2_driving *driving) {
    if (driving != NULL) {
        driving->accumulator = 0;
    }
}

const dd2_vehicle *dd2_driving_vehicle(const dd2_driving *driving) {
    return driving != NULL ? &driving->vehicle : NULL;
}

double dd2_driving_wheel_roll(const dd2_driving *driving) {
    return driving != NULL ? driving->wheel_roll : 0;
}

const dd2_vehicle_spawn *dd2_driving_start(const dd2_driving *driving) {
    return driving != NULL ? &driving->spawn : NULL;
}

uint64_t dd2_driving_collisions(const dd2_driving *driving) {
    return driving != NULL ? driving->collisions : 0;
}
