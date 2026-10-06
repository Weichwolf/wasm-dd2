#include "physics/vehicle_collision.h"

#include "physics/barrier_world.h"
#include "physics/vehicle.h"

#include <math.h>
#include <stddef.h>

/* Overlapping rounded body lobes fit the 372 x 900 footprint. They avoid
 * snagging a rectangular corner on a strip seam; they are rewrite tuning. */
enum { DD2_COLLISION_PROBES = 5, DD2_COLLISION_ITERATIONS = 16 };
static const double dd2_collision_radius = 186;
static const double dd2_collision_half_height = 130;
static const double dd2_collision_half_length = 264;
static const double dd2_collision_clearance = 1e-4;
static const double dd2_collision_restitution = 0.2;
static const double dd2_collision_friction = 0.25;
static const double dd2_collision_motion_tolerance = 1e-8;

static dd2_vehicle_vector dd2_collision_add(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return (dd2_vehicle_vector){
        .x = first.x + second.x, .y = first.y + second.y, .z = first.z + second.z};
}
static dd2_vehicle_vector dd2_collision_scale(dd2_vehicle_vector vector, double factor) {
    return (dd2_vehicle_vector){
        .x = vector.x * factor, .y = vector.y * factor, .z = vector.z * factor};
}
static double dd2_collision_dot(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return (first.x * second.x) + (first.y * second.y) + (first.z * second.z);
}
static dd2_vehicle_vector dd2_collision_cross(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return (dd2_vehicle_vector){.x = (first.y * second.z) - (first.z * second.y),
                                .y = (first.z * second.x) - (first.x * second.z),
                                .z = (first.x * second.y) - (first.y * second.x)};
}
static double dd2_collision_rotation_dot(dd2_vehicle_rotation first, dd2_vehicle_rotation second) {
    return (first.x * second.x) + (first.y * second.y) + (first.z * second.z) +
           (first.w * second.w);
}
static dd2_vehicle_rotation dd2_collision_normalize(dd2_vehicle_rotation rotation) {
    const double scale = 1 / sqrt(dd2_collision_rotation_dot(rotation, rotation));
    return (dd2_vehicle_rotation){.x = rotation.x * scale,
                                  .y = rotation.y * scale,
                                  .z = rotation.z * scale,
                                  .w = rotation.w * scale};
}
static dd2_vehicle_rotation dd2_collision_rotation(dd2_vehicle_rotation first,
                                                   dd2_vehicle_rotation second, double time) {
    const double sign = dd2_collision_rotation_dot(first, second) < 0 ? -1 : 1;
    return dd2_collision_normalize(
        (dd2_vehicle_rotation){.x = first.x + (((sign * second.x) - first.x) * time),
                               .y = first.y + (((sign * second.y) - first.y) * time),
                               .z = first.z + (((sign * second.z) - first.z) * time),
                               .w = first.w + (((sign * second.w) - first.w) * time)});
}
static dd2_vehicle_rotation dd2_collision_turn(const dd2_vehicle *vehicle, double seconds) {
    const dd2_vehicle_vector half = dd2_collision_scale(vehicle->angular_velocity, seconds / 2);
    const dd2_vehicle_rotation rotation = vehicle->rotation;
    return dd2_collision_normalize((dd2_vehicle_rotation){
        .x = rotation.x + (half.x * rotation.w) + (half.y * rotation.z) - (half.z * rotation.y),
        .y = rotation.y + (half.y * rotation.w) + (half.z * rotation.x) - (half.x * rotation.z),
        .z = rotation.z + (half.z * rotation.w) + (half.x * rotation.y) - (half.y * rotation.x),
        .w = rotation.w - (half.x * rotation.x) - (half.y * rotation.y) - (half.z * rotation.z)});
}

static bool dd2_collision_sweep(const dd2_vehicle *start, const dd2_vehicle *end,
                                const dd2_barrier_world *world, dd2_barrier_contact *contact) {
    /* A rotated point's arc is at most radius*(1-cos(angle/2)) from its chord.
     * Quaternions give cos(angle/2) directly. Inflate footprint and height. */
    const double padding =
        dd2_collision_half_length *
        fmax(0, 1 - fabs(dd2_collision_rotation_dot(start->rotation, end->rotation)));
    bool found = false;
    for (unsigned probe = 0; probe < DD2_COLLISION_PROBES; ++probe) {
        const double local_z =
            -dd2_collision_half_length +
            ((double)probe * 2 * dd2_collision_half_length / (double)(DD2_COLLISION_PROBES - 1));
        const dd2_vehicle_vector local = {.z = local_z};
        const dd2_barrier_sweep sweep = {
            .start = dd2_collision_add(start->position, dd2_vehicle_rotate(start->rotation, local)),
            .end = dd2_collision_add(end->position, dd2_vehicle_rotate(end->rotation, local)),
            .radius = dd2_collision_radius + padding,
            .half_height = dd2_collision_half_height + padding};
        dd2_barrier_contact candidate = {0};
        if (dd2_barrier_world_sweep(world, sweep, &candidate, NULL) &&
            (!found || candidate.time < contact->time)) {
            *contact = candidate;
            found = true;
        }
    }
    return found;
}

static double dd2_collision_effective_mass(const dd2_vehicle *vehicle, dd2_vehicle_vector arm,
                                           dd2_vehicle_vector axis) {
    const dd2_vehicle_vector response =
        dd2_vehicle_angular_response(vehicle->rotation, dd2_collision_cross(arm, axis));
    return 1 + dd2_collision_dot(axis, dd2_collision_cross(response, arm));
}

static void dd2_collision_impulse(dd2_vehicle *vehicle, dd2_vehicle_vector arm,
                                  dd2_vehicle_vector impulse) {
    vehicle->velocity = dd2_collision_add(vehicle->velocity, impulse);
    vehicle->angular_velocity = dd2_collision_add(
        vehicle->angular_velocity,
        dd2_vehicle_angular_response(vehicle->rotation, dd2_collision_cross(arm, impulse)));
}

static dd2_vehicle_vector dd2_collision_point_velocity(const dd2_vehicle *vehicle,
                                                       dd2_vehicle_vector arm) {
    return dd2_collision_add(vehicle->velocity,
                             dd2_collision_cross(vehicle->angular_velocity, arm));
}

static void dd2_collision_response(dd2_vehicle *vehicle, dd2_barrier_contact contact,
                                   dd2_vehicle_impact *impact) {
    const dd2_vehicle_vector arm =
        dd2_collision_add(contact.point, dd2_collision_scale(vehicle->position, -1));
    const double speed =
        dd2_collision_dot(dd2_collision_point_velocity(vehicle, arm), contact.normal);
    if (speed >= 0) {
        return;
    }
    const double impulse = -(1 + dd2_collision_restitution) * speed /
                           dd2_collision_effective_mass(vehicle, arm, contact.normal);
    dd2_collision_impulse(vehicle, arm, dd2_collision_scale(contact.normal, impulse));
    const dd2_vehicle_vector velocity = dd2_collision_point_velocity(vehicle, arm);
    dd2_vehicle_vector tangent = dd2_collision_add(
        velocity,
        dd2_collision_scale(contact.normal, -dd2_collision_dot(velocity, contact.normal)));
    const double length = sqrt(dd2_collision_dot(tangent, tangent));
    if (length > dd2_collision_motion_tolerance) {
        tangent = dd2_collision_scale(tangent, 1 / length);
        const double friction = fmin(dd2_collision_friction * impulse,
                                     length / dd2_collision_effective_mass(vehicle, arm, tangent));
        dd2_collision_impulse(vehicle, arm, dd2_collision_scale(tangent, -friction));
    }
    if (-speed > impact->normal_speed) {
        impact->normal_speed = -speed;
        impact->impulse = impulse;
        impact->point = contact.point;
    }
}

bool dd2_vehicle_collide_barriers(dd2_vehicle *vehicle, const dd2_vehicle *previous,
                                  const dd2_barrier_world *world, dd2_vehicle_impact *impact) {
    if (impact != NULL) {
        *impact = (dd2_vehicle_impact){0};
    }
    if (world == NULL || !dd2_vehicle_valid(vehicle) || !dd2_vehicle_valid(previous)) {
        return false;
    }
    dd2_vehicle next = *vehicle;
    dd2_vehicle start = *previous;
    dd2_vehicle_impact events = {0};
    double remaining = DD2_VEHICLE_STEP_SECONDS;
    for (unsigned iteration = 0; iteration < DD2_COLLISION_ITERATIONS; ++iteration) {
        dd2_barrier_contact contact = {0};
        if (!dd2_collision_sweep(&start, &next, world, &contact)) {
            break;
        }
        next.position = dd2_collision_add(
            start.position,
            dd2_collision_scale(
                dd2_collision_add(next.position, dd2_collision_scale(start.position, -1)),
                contact.time));
        next.rotation = dd2_collision_rotation(start.rotation, next.rotation, contact.time);
        dd2_collision_response(&next, contact, &events);
        ++events.contacts;
        next.position = dd2_collision_add(
            next.position,
            dd2_collision_scale(contact.normal, contact.penetration + dd2_collision_clearance));
        remaining *= 1 - contact.time;
        start = next;
        next.position =
            dd2_collision_add(start.position, dd2_collision_scale(next.velocity, remaining));
        next.rotation = dd2_collision_turn(&next, remaining);
        if (iteration + 1 == DD2_COLLISION_ITERATIONS) {
            /* Keep the last corrected contact pose if a pathological corner
             * consumes the solver budget. Never accept unchecked residual travel. */
            next.position = start.position;
            next.rotation = start.rotation;
        }
    }
    if (!dd2_vehicle_valid(&next)) {
        return false;
    }
    *vehicle = next;
    if (impact != NULL) {
        *impact = events;
    }
    return true;
}
