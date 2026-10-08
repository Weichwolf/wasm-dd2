#include "game/recovery.h"

#include "assets/road.h"
#include "physics/collision_math.h"
#include "physics/road_contact.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

static const double dd2_recovery_min_up = 0.2;
static const double dd2_recovery_support_gap = 2;
static const double dd2_recovery_angular_limit = 0.5;
static const double dd2_recovery_vertical_limit = 50;
static const double dd2_recovery_ride_height = 190;
static const double dd2_recovery_wheel_radius = 60;
static const double dd2_recovery_npc_distance = 8192;
static const double dd2_recovery_heading_tolerance = 1e-10;

typedef struct {
    dd2_road_contact road;
    dd2_vehicle_vector point;
} dd2_recovery_support;

static bool dd2_recovery_rest(const dd2_vehicle *vehicle, const dd2_road_surface *surface,
                              dd2_recovery_support *support) {
    const dd2_vehicle_vector upward =
        dd2_vehicle_rotate(vehicle->rotation, (dd2_vehicle_vector){.y = 1});
    if (upward.y >= dd2_recovery_min_up ||
        dd2_collision_dot(vehicle->angular_velocity, vehicle->angular_velocity) >
            dd2_recovery_angular_limit * dd2_recovery_angular_limit) {
        return false;
    }
    bool found = false;
    double best_alignment = 0;
    for (unsigned corner = 0; corner < DD2_VEHICLE_BODY_CORNERS; ++corner) {
        const dd2_vehicle_vector point = dd2_collision_add(
            vehicle->position,
            dd2_vehicle_rotate(vehicle->rotation, dd2_vehicle_body_corner(corner)));
        dd2_road_contact road = {0};
        if (!dd2_road_surface_sample(
                surface,
                (dd2_surface_query){.point = {.x = point.x, .z = point.z},
                                    .min_height = point.y - dd2_recovery_support_gap,
                                    .max_height = point.y + dd2_recovery_support_gap,
                                    .preferred_cell = DD2_ROAD_NO_STRIP},
                &road, NULL) ||
            road.normal[1] < dd2_recovery_min_up) {
            continue;
        }
        const dd2_vehicle_vector normal = {
            .x = road.normal[0], .y = road.normal[1], .z = road.normal[2]};
        if (fabs(dd2_collision_dot(vehicle->velocity, normal)) <= dd2_recovery_vertical_limit) {
            /* A corner can touch the opposite bank of a valley while the roof
             * rests on this bank. Prefer the road facing the roof; extrapolating
             * the first corner's plane can miss the actual center road. Equal
             * alignments retain source-corner order. */
            const double alignment = -dd2_collision_dot(upward, normal);
            if (!found || alignment > best_alignment) {
                *support = (dd2_recovery_support){.road = road, .point = point};
                best_alignment = alignment;
                found = true;
            }
        }
    }
    return found;
}

static double dd2_recovery_yaw(dd2_vehicle_rotation rotation) {
    const dd2_vehicle_vector forward = dd2_vehicle_rotate(rotation, (dd2_vehicle_vector){.z = 1});
    if (hypot(forward.x, forward.z) > dd2_recovery_heading_tolerance) {
        return atan2(forward.x, forward.z);
    }
    const dd2_vehicle_vector right = dd2_vehicle_rotate(rotation, (dd2_vehicle_vector){.x = 1});
    return atan2(-right.z, right.x);
}

static bool dd2_recovery_land(dd2_vehicle *vehicle, dd2_recovery_support support,
                              dd2_recovery_frame frame) {
    const dd2_vehicle_vector rest_normal = {
        .x = support.road.normal[0], .y = support.road.normal[1], .z = support.road.normal[2]};
    const double expected =
        support.road.height - (((rest_normal.x * (vehicle->position.x - support.point.x)) +
                                (rest_normal.z * (vehicle->position.z - support.point.z))) /
                               rest_normal.y);
    dd2_road_contact landing = {0};
    /* A supporting roof corner may be across a banking seam. Orient/position
     * the upright car on the actual center triangle, not an extrapolated plane.
     * The local height window preserves the supported bridge level. */
    if (!dd2_road_surface_sample(
            frame.surface,
            (dd2_surface_query){.point = {.x = vehicle->position.x, .z = vehicle->position.z},
                                .min_height = expected - dd2_recovery_ride_height,
                                .max_height = expected + dd2_recovery_ride_height,
                                .preferred_cell = support.road.cell},
            &landing, NULL) ||
        landing.normal[1] < dd2_recovery_min_up) {
        return false;
    }
    const dd2_vehicle_vector normal = {
        .x = landing.normal[0], .y = landing.normal[1], .z = landing.normal[2]};
    const double height = landing.height;
    dd2_vehicle next = {0};
    if (!dd2_vehicle_reset(
            &next, (dd2_vehicle_spawn){
                       .position = {.x = vehicle->position.x,
                                    .y = height +
                                         ((dd2_recovery_ride_height - dd2_recovery_wheel_radius) /
                                          normal.y) +
                                         dd2_recovery_wheel_radius - DD2_ROAD_EDGE_TOLERANCE,
                                    .z = vehicle->position.z},
                       .yaw = dd2_recovery_yaw(vehicle->rotation)})) {
        return false;
    }
    const double length = sqrt(2 * (1 + normal.y));
    const dd2_vehicle_rotation tilt = {
        .x = normal.z / length, .z = -normal.x / length, .w = (1 + normal.y) / length};
    const dd2_vehicle_rotation yaw = next.rotation;
    next.rotation = (dd2_vehicle_rotation){.x = (tilt.x * yaw.w) - (tilt.z * yaw.y),
                                           .y = tilt.w * yaw.y,
                                           .z = (tilt.x * yaw.y) + (tilt.z * yaw.w),
                                           .w = tilt.w * yaw.w};
    /* Correct placement against the actual world with zero kinematic velocity;
     * righting a car must not manufacture a crash impulse or accident credit. */
    if (!dd2_vehicle_collide_world(&next, &next, frame.surface, frame.world, NULL)) {
        return false;
    }
    next.velocity = dd2_collision_add(
        vehicle->velocity,
        dd2_collision_scale(normal, -dd2_collision_dot(vehicle->velocity, normal)));
    next.steering = vehicle->steering;
    next.steps = vehicle->steps;
    if (!dd2_vehicle_refresh_wheels(&next, frame.road, frame.surface)) {
        return false;
    }
    bool grounded = false;
    for (unsigned wheel = 0; wheel < DD2_VEHICLE_WHEELS; ++wheel) {
        grounded = grounded || next.wheels[wheel].grounded;
    }
    if (!grounded) {
        return false;
    }
    *vehicle = next;
    return true;
}

bool dd2_recovery_step(dd2_recovery_driver *drivers, dd2_vehicle *vehicles,
                       dd2_recovery_frame frame) {
    if (drivers == NULL || vehicles == NULL || frame.road == NULL || frame.surface == NULL ||
        frame.damage == NULL || frame.count == 0 || frame.count > DD2_VEHICLE_FLEET_LIMIT) {
        return false;
    }
    dd2_recovery_driver states[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_vehicle next[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned slot = 0; slot < frame.count; ++slot) {
        if (!dd2_vehicle_valid(&vehicles[slot]) ||
            drivers[slot].rest_steps > DD2_RECOVERY_REST_STEPS ||
            drivers[slot].overturned != (drivers[slot].rest_steps != 0)) {
            return false;
        }
        next[slot] = vehicles[slot];
        states[slot] = drivers[slot];
    }
    for (unsigned slot = 0; slot < frame.count; ++slot) {
        dd2_recovery_support support = {0};
        states[slot].overturned = dd2_recovery_rest(&next[slot], frame.surface, &support);
        if (!states[slot].overturned) {
            states[slot].rest_steps = 0;
            continue;
        }
        if (states[slot].rest_steps < DD2_RECOVERY_REST_STEPS) {
            ++states[slot].rest_steps;
        }
        const bool distant = slot == 0 || hypot(next[slot].position.x - next[0].position.x,
                                                next[slot].position.z - next[0].position.z) >
                                              dd2_recovery_npc_distance;
        if (states[slot].rest_steps == DD2_RECOVERY_REST_STEPS && !frame.damage[slot].retired &&
            distant) {
            if (states[slot].recoveries == UINT64_MAX) {
                return false;
            }
            if (dd2_recovery_land(&next[slot], support, frame)) {
                ++states[slot].recoveries;
                states[slot].rest_steps = 0;
                states[slot].overturned = false;
            }
        }
    }
    for (unsigned slot = 0; slot < frame.count; ++slot) {
        vehicles[slot] = next[slot];
        drivers[slot] = states[slot];
    }
    return true;
}
