#ifndef DD2_PHYSICS_COLLISION_MATH_H
#define DD2_PHYSICS_COLLISION_MATH_H

#include "physics/vehicle.h"

#include <math.h>

/* Internal rigid-body arithmetic. Callers validate state before using it. */
static inline dd2_vehicle_vector dd2_collision_add(dd2_vehicle_vector first,
                                                   dd2_vehicle_vector second) {
    return (dd2_vehicle_vector){
        .x = first.x + second.x, .y = first.y + second.y, .z = first.z + second.z};
}
static inline dd2_vehicle_vector dd2_collision_scale(dd2_vehicle_vector vector, double factor) {
    return (dd2_vehicle_vector){
        .x = vector.x * factor, .y = vector.y * factor, .z = vector.z * factor};
}
static inline double dd2_collision_dot(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return (first.x * second.x) + (first.y * second.y) + (first.z * second.z);
}
static inline dd2_vehicle_vector dd2_collision_cross(dd2_vehicle_vector first,
                                                     dd2_vehicle_vector second) {
    return (dd2_vehicle_vector){.x = (first.y * second.z) - (first.z * second.y),
                                .y = (first.z * second.x) - (first.x * second.z),
                                .z = (first.x * second.y) - (first.y * second.x)};
}
static inline double dd2_collision_rotation_dot(dd2_vehicle_rotation first,
                                                dd2_vehicle_rotation second) {
    return (first.x * second.x) + (first.y * second.y) + (first.z * second.z) +
           (first.w * second.w);
}
static inline dd2_vehicle_rotation dd2_collision_normalize(dd2_vehicle_rotation rotation) {
    const double scale = 1 / sqrt(dd2_collision_rotation_dot(rotation, rotation));
    return (dd2_vehicle_rotation){.x = rotation.x * scale,
                                  .y = rotation.y * scale,
                                  .z = rotation.z * scale,
                                  .w = rotation.w * scale};
}
static inline dd2_vehicle_rotation
dd2_collision_rotation(dd2_vehicle_rotation first, dd2_vehicle_rotation second, double time) {
    const double sign = dd2_collision_rotation_dot(first, second) < 0 ? -1 : 1;
    return dd2_collision_normalize(
        (dd2_vehicle_rotation){.x = first.x + (((sign * second.x) - first.x) * time),
                               .y = first.y + (((sign * second.y) - first.y) * time),
                               .z = first.z + (((sign * second.z) - first.z) * time),
                               .w = first.w + (((sign * second.w) - first.w) * time)});
}
static inline dd2_vehicle_rotation dd2_collision_turn(const dd2_vehicle *vehicle, double seconds) {
    const dd2_vehicle_vector half = dd2_collision_scale(vehicle->angular_velocity, seconds / 2);
    const dd2_vehicle_rotation rotation = vehicle->rotation;
    return dd2_collision_normalize((dd2_vehicle_rotation){
        .x = rotation.x + (half.x * rotation.w) + (half.y * rotation.z) - (half.z * rotation.y),
        .y = rotation.y + (half.y * rotation.w) + (half.z * rotation.x) - (half.x * rotation.z),
        .z = rotation.z + (half.z * rotation.w) + (half.x * rotation.y) - (half.y * rotation.x),
        .w = rotation.w - (half.x * rotation.x) - (half.y * rotation.y) - (half.z * rotation.z)});
}
static inline dd2_vehicle_vector dd2_collision_position(dd2_vehicle_vector first,
                                                        dd2_vehicle_vector second, double time) {
    return dd2_collision_add(
        first,
        dd2_collision_scale(dd2_collision_add(second, dd2_collision_scale(first, -1)), time));
}
static inline double dd2_collision_effective_mass(const dd2_vehicle *vehicle,
                                                  dd2_vehicle_vector arm, dd2_vehicle_vector axis) {
    const dd2_vehicle_vector response =
        dd2_vehicle_angular_response(vehicle->rotation, dd2_collision_cross(arm, axis));
    return 1 + dd2_collision_dot(axis, dd2_collision_cross(response, arm));
}
static inline void dd2_collision_impulse(dd2_vehicle *vehicle, dd2_vehicle_vector arm,
                                         dd2_vehicle_vector impulse) {
    vehicle->velocity = dd2_collision_add(vehicle->velocity, impulse);
    vehicle->angular_velocity = dd2_collision_add(
        vehicle->angular_velocity,
        dd2_vehicle_angular_response(vehicle->rotation, dd2_collision_cross(arm, impulse)));
}
static inline dd2_vehicle_vector dd2_collision_point_velocity(const dd2_vehicle *vehicle,
                                                              dd2_vehicle_vector arm) {
    return dd2_collision_add(vehicle->velocity,
                             dd2_collision_cross(vehicle->angular_velocity, arm));
}
#endif
