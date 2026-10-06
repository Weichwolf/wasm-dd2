#include "game/driving.h"

#include "ai/driver.h"
#include "assets/barriers.h"
#include "assets/road.h"
#include "game/starting_grid.h"
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

enum { DD2_DRIVING_RACING_LEVELS = 7, DD2_DRIVING_LEVELS = 11, DD2_DRIVING_SETTLE_STEPS = 200 };
static const double dd2_driving_full_turn = 6.28318530717958647693;
static const double dd2_driving_ride_height = 190;
static const double dd2_driving_wheel_radius = 60;
static const double dd2_driving_max_frame = 0.25;
static const double dd2_driving_time_tolerance = 1e-12;

struct dd2_driving {
    const dd2_road *road;
    dd2_road_surface *surface;
    dd2_barriers *barriers;
    dd2_barrier_world *barrier_world;
    uint64_t collisions;
    dd2_grid_start starts[DD2_VEHICLE_FLEET_LIMIT];
    dd2_vehicle vehicles[DD2_VEHICLE_FLEET_LIMIT];
    dd2_ai_driver drivers[DD2_VEHICLE_FLEET_LIMIT];
    bool opponents;
    uint64_t pair_collisions;
    double accumulator;
    double wheel_rolls[DD2_VEHICLE_FLEET_LIMIT];
};

static bool dd2_driving_align(dd2_vehicle *vehicle, const dd2_driving *driving, unsigned slot) {
    dd2_road_contact contact = {0};
    if (!dd2_road_surface_sample(
            driving->surface,
            (dd2_surface_query){.point = {.x = driving->starts[slot].spawn.position.x,
                                          .z = driving->starts[slot].spawn.position.z},
                                .min_height = driving->starts[slot].spawn.position.y -
                                              (2 * dd2_driving_ride_height),
                                .max_height = driving->starts[slot].spawn.position.y,
                                .preferred_cell = driving->starts[slot].cell},
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
    dd2_vehicle vehicles[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        if (!dd2_vehicle_reset(&vehicles[slot], driving->starts[slot].spawn) ||
            !dd2_driving_align(&vehicles[slot], driving, slot)) {
            return false;
        }
    }
    for (unsigned step = 0; step < DD2_DRIVING_SETTLE_STEPS; ++step) {
        dd2_vehicle previous[DD2_VEHICLE_FLEET_LIMIT] = {0};
        for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
            previous[slot] = vehicles[slot];
            if (!dd2_vehicle_step(&vehicles[slot], driving->road, driving->surface,
                                  (dd2_vehicle_control){.brake = 1})) {
                return false;
            }
        }
        if (!dd2_vehicle_collide_fleet(vehicles, previous, DD2_VEHICLE_FLEET_LIMIT,
                                       driving->surface, driving->barrier_world, NULL, NULL)) {
            return false;
        }
    }
    dd2_ai_driver drivers[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        if (!dd2_ai_driver_reset(&drivers[slot],
                                 (dd2_ai_start){.road = driving->road,
                                                .cell = driving->starts[slot].cell,
                                                .slot = slot,
                                                .count = DD2_VEHICLE_FLEET_LIMIT})) {
            return false;
        }
    }
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        driving->vehicles[slot] = vehicles[slot];
        driving->drivers[slot] = drivers[slot];
        driving->wheel_rolls[slot] = 0;
    }
    driving->accumulator = 0;
    driving->collisions = 0;
    driving->pair_collisions = 0;
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
    driving->opponents = true;
    driving->surface = dd2_road_surface_create(road);
    driving->barriers = dd2_barriers_create(road, level);
    driving->barrier_world = dd2_barrier_world_create(driving->barriers);
    if (driving->surface == NULL || driving->barrier_world == NULL ||
        !dd2_starting_grid(road, driving->surface, level, DD2_VEHICLE_FLEET_LIMIT,
                           driving->starts) ||
        !dd2_driving_reset(driving)) {
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

static bool dd2_driving_step(const dd2_driving *driving, dd2_vehicle *vehicles,
                             dd2_ai_driver *drivers, dd2_vehicle_control player,
                             dd2_vehicle_impact *impacts) {
    dd2_vehicle previous[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_vehicle_control controls[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        previous[slot] = vehicles[slot];
        controls[slot] = (dd2_vehicle_control){.brake = 1};
    }
    controls[0] = player;
    for (unsigned slot = 1; slot < DD2_VEHICLE_FLEET_LIMIT && driving->opponents; ++slot) {
        const dd2_ai_observation observation = {.road = driving->road,
                                                .surface = driving->surface,
                                                .vehicles = previous,
                                                .count = DD2_VEHICLE_FLEET_LIMIT,
                                                .slot = slot};
        if (!dd2_ai_driver_step(&drivers[slot], &observation, &controls[slot])) {
            return false;
        }
    }
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        if (!dd2_vehicle_step(&vehicles[slot], driving->road, driving->surface, controls[slot])) {
            return false;
        }
    }
    return dd2_vehicle_collide_fleet(vehicles, previous, DD2_VEHICLE_FLEET_LIMIT, driving->surface,
                                     driving->barrier_world, impacts, NULL);
}

bool dd2_driving_advance(dd2_driving *driving, dd2_driving_frame frame) {
    if (driving == NULL || !dd2_numeric_finite(&frame.seconds) || frame.seconds < 0 ||
        frame.seconds > dd2_driving_max_frame || !dd2_numeric_finite(&frame.control.throttle) ||
        fabs(frame.control.throttle) > 1 || !dd2_numeric_finite(&frame.control.brake) ||
        frame.control.brake < 0 || frame.control.brake > 1 ||
        !dd2_numeric_finite(&frame.control.steer) || fabs(frame.control.steer) > 1) {
        return false;
    }
    dd2_vehicle vehicles[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_ai_driver drivers[DD2_VEHICLE_FLEET_LIMIT] = {0};
    double rolls[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        vehicles[slot] = driving->vehicles[slot];
        drivers[slot] = driving->drivers[slot];
        rolls[slot] = driving->wheel_rolls[slot];
    }
    double accumulator = driving->accumulator + frame.seconds;
    uint64_t pairs = driving->pair_collisions;
    uint64_t collisions = driving->collisions;
    const unsigned steps =
        (unsigned)floor((accumulator + dd2_driving_time_tolerance) / DD2_VEHICLE_STEP_SECONDS);
    for (unsigned step = 0; step < steps; ++step) {
        dd2_vehicle_impact impacts[DD2_VEHICLE_FLEET_LIMIT] = {0};
        if (!dd2_driving_step(driving, vehicles, drivers, frame.control, impacts) ||
            UINT64_MAX - collisions < impacts[0].contacts ||
            UINT64_MAX - pairs < impacts[0].pair_contacts) {
            return false;
        }
        collisions += impacts[0].contacts;
        pairs += impacts[0].pair_contacts;
        for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
            const dd2_vehicle *vehicle = &vehicles[slot];
            const dd2_vehicle_vector forward =
                dd2_vehicle_rotate(vehicle->rotation, (dd2_vehicle_vector){.z = 1});
            const double speed = (vehicle->velocity.x * forward.x) +
                                 (vehicle->velocity.y * forward.y) +
                                 (vehicle->velocity.z * forward.z);
            rolls[slot] =
                fmod(rolls[slot] + (speed * DD2_VEHICLE_STEP_SECONDS / dd2_driving_wheel_radius),
                     dd2_driving_full_turn);
        }
    }
    accumulator = fmax(0, accumulator - ((double)steps * DD2_VEHICLE_STEP_SECONDS));
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        driving->vehicles[slot] = vehicles[slot];
        driving->drivers[slot] = drivers[slot];
        driving->wheel_rolls[slot] = rolls[slot];
    }
    driving->pair_collisions = pairs;
    driving->accumulator = accumulator;
    driving->collisions = collisions;
    return true;
}

void dd2_driving_suspend(dd2_driving *driving) {
    if (driving != NULL) {
        driving->accumulator = 0;
    }
}

void dd2_driving_set_opponents(dd2_driving *driving, bool enabled) {
    if (driving != NULL) {
        driving->opponents = enabled;
        driving->accumulator = 0;
    }
}
bool dd2_driving_opponents(const dd2_driving *driving) {
    return driving != NULL && driving->opponents;
}
const dd2_ai_driver *dd2_driving_drivers(const dd2_driving *driving) {
    return driving != NULL ? driving->drivers : NULL;
}

const dd2_vehicle *dd2_driving_vehicle(const dd2_driving *driving) {
    return driving != NULL ? &driving->vehicles[0] : NULL;
}

double dd2_driving_wheel_roll(const dd2_driving *driving) {
    return driving != NULL ? driving->wheel_rolls[0] : 0;
}

const dd2_vehicle_spawn *dd2_driving_start(const dd2_driving *driving) {
    return driving != NULL ? &driving->starts[0].spawn : NULL;
}

uint64_t dd2_driving_collisions(const dd2_driving *driving) {
    return driving != NULL ? driving->collisions : 0;
}

unsigned dd2_driving_vehicle_count(const dd2_driving *driving) {
    return driving != NULL ? DD2_VEHICLE_FLEET_LIMIT : 0;
}
const dd2_vehicle *dd2_driving_vehicles(const dd2_driving *driving) {
    return driving != NULL ? driving->vehicles : NULL;
}
const double *dd2_driving_wheel_rolls(const dd2_driving *driving) {
    return driving != NULL ? driving->wheel_rolls : NULL;
}
uint64_t dd2_driving_pair_collisions(const dd2_driving *driving) {
    return driving != NULL ? driving->pair_collisions : 0;
}
const dd2_vehicle_spawn *dd2_driving_grid_start(const dd2_driving *driving, unsigned slot) {
    return driving != NULL && slot < DD2_VEHICLE_FLEET_LIMIT ? &driving->starts[slot].spawn : NULL;
}
