#include "physics/vehicle_collision.h"

#include "physics/barrier_world.h"
#include "physics/car_contact.h"
#include "physics/collision_math.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"

#include <math.h>
#include <stddef.h>

/* Overlapping rounded body lobes fit the 372 x 900 footprint. They avoid
 * snagging a rectangular corner on a strip seam; they are rewrite tuning. */
enum {
    DD2_COLLISION_PROBES = 5,
    DD2_COLLISION_ITERATIONS = 64,
    DD2_COLLISION_BARRIER_ITERATIONS = 16,
    DD2_COLLISION_ROTATION_SEGMENTS = 16
};
static const double dd2_collision_radius = 186;
static const double dd2_collision_half_height = 130;
static const double dd2_collision_half_length = 264;
static const double dd2_collision_clearance = 1e-4;
static const double dd2_collision_restitution = 0.2;
static const double dd2_collision_friction = 0.25;
static const double dd2_collision_motion_tolerance = 1e-8;
static const double dd2_collision_ground_friction = 0.8;
static const double dd2_collision_ground_recovery = 25;
static const double dd2_collision_angular_chord = 0.02;
static const double dd2_collision_time_tolerance = 1e-10;

/* Get_Corner_Positions uses this box, separate from the rendered mesh and
 * visual body offset. The lower four points match the source wheel rig. */
static const dd2_vehicle_vector dd2_collision_corners[DD2_VEHICLE_BODY_CORNERS] = {
    {.x = 186, .y = -130, .z = 450},  {.x = 186, .y = -130, .z = -450},
    {.x = -186, .y = -130, .z = 450}, {.x = -186, .y = -130, .z = -450},
    {.x = 186, .y = 130, .z = 450},   {.x = 186, .y = 130, .z = -450},
    {.x = -186, .y = 130, .z = 450},  {.x = -186, .y = 130, .z = -450}};

dd2_vehicle_vector dd2_vehicle_body_corner(unsigned corner) {
    return corner < DD2_VEHICLE_BODY_CORNERS ? dd2_collision_corners[corner]
                                             : (dd2_vehicle_vector){0};
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

static dd2_vehicle_vector dd2_collision_corner(const dd2_vehicle *start, const dd2_vehicle *end,
                                               unsigned corner, double time) {
    const dd2_vehicle_vector position = dd2_collision_add(
        start->position,
        dd2_collision_scale(
            dd2_collision_add(end->position, dd2_collision_scale(start->position, -1)), time));
    return dd2_collision_add(
        position, dd2_vehicle_rotate(dd2_collision_rotation(start->rotation, end->rotation, time),
                                     dd2_collision_corners[corner]));
}

static bool dd2_collision_ground_sweep(const dd2_vehicle *start, const dd2_vehicle *end,
                                       const dd2_road_surface *surface,
                                       dd2_barrier_contact *contact) {
    const double cosine = fmin(1, fabs(dd2_collision_rotation_dot(start->rotation, end->rotation)));
    const double angle = 2 * acos(cosine);
    const unsigned segments = (unsigned)fmin((double)DD2_COLLISION_ROTATION_SEGMENTS,
                                             fmax(1, ceil(angle / dd2_collision_angular_chord)));
    dd2_barrier_contact candidates[DD2_VEHICLE_BODY_CORNERS] = {0};
    bool eligible[DD2_VEHICLE_BODY_CORNERS] = {false};
    double earliest = 1;
    bool found = false;
    for (unsigned segment = 0; segment < segments; ++segment) {
        const double begin = (double)segment / (double)segments;
        const double finish = (double)(segment + 1) / (double)segments;
        if (found && begin > earliest + dd2_collision_time_tolerance) {
            break;
        }
        for (unsigned corner = 0; corner < DD2_VEHICLE_BODY_CORNERS; ++corner) {
            const dd2_vehicle_vector first = dd2_collision_corner(start, end, corner, begin);
            const dd2_vehicle_vector last = dd2_collision_corner(start, end, corner, finish);
            const dd2_surface_sweep sweep = {.start = {.x = first.x, .y = first.y, .z = first.z},
                                             .end = {.x = last.x, .y = last.y, .z = last.z},
                                             .recovery = dd2_collision_ground_recovery};
            dd2_surface_hit hit = {0};
            if (!dd2_road_surface_sweep(surface, sweep, &hit, NULL)) {
                continue;
            }
            const double time = begin + (hit.time / (double)segments);
            if (!eligible[corner] || time < candidates[corner].time) {
                candidates[corner] = (dd2_barrier_contact){
                    .time = time,
                    .penetration = hit.penetration,
                    .point = {.x = hit.point.x, .y = hit.point.y, .z = hit.point.z},
                    .normal = {.x = hit.road.normal[0],
                               .y = hit.road.normal[1],
                               .z = hit.road.normal[2]},
                    .barrier = hit.road.cell};
                found = true;
                eligible[corner] = true;
                earliest = fmin(earliest, time);
            }
        }
    }
    /* Anchor all ties to the global earliest time, then prefer the source
     * corner order. Nearly simultaneous supports must not change impulse order
     * because native and WASM round one distance differently. */
    for (unsigned corner = 0; corner < DD2_VEHICLE_BODY_CORNERS; ++corner) {
        if (eligible[corner] &&
            candidates[corner].time <= earliest + dd2_collision_time_tolerance) {
            *contact = candidates[corner];
            break;
        }
    }
    return found;
}

static void dd2_collision_response(dd2_vehicle *vehicle, dd2_barrier_contact contact, bool ground,
                                   dd2_vehicle_impact *impact) {
    const dd2_vehicle_vector arm =
        dd2_collision_add(contact.point, dd2_collision_scale(vehicle->position, -1));
    const double speed =
        dd2_collision_dot(dd2_collision_point_velocity(vehicle, arm), contact.normal);
    if (speed >= 0) {
        return;
    }
    const double restitution = ground ? 0 : dd2_collision_restitution;
    const double coefficient = ground ? dd2_collision_ground_friction : dd2_collision_friction;
    const double impulse =
        -(1 + restitution) * speed / dd2_collision_effective_mass(vehicle, arm, contact.normal);
    dd2_collision_impulse(vehicle, arm, dd2_collision_scale(contact.normal, impulse));
    const dd2_vehicle_vector velocity = dd2_collision_point_velocity(vehicle, arm);
    dd2_vehicle_vector tangent = dd2_collision_add(
        velocity,
        dd2_collision_scale(contact.normal, -dd2_collision_dot(velocity, contact.normal)));
    const double length = sqrt(dd2_collision_dot(tangent, tangent));
    if (length > dd2_collision_motion_tolerance) {
        tangent = dd2_collision_scale(tangent, 1 / length);
        const double friction = fmin(coefficient * impulse,
                                     length / dd2_collision_effective_mass(vehicle, arm, tangent));
        dd2_collision_impulse(vehicle, arm, dd2_collision_scale(tangent, -friction));
    }
    if (-speed > impact->normal_speed) {
        impact->normal_speed = -speed;
        impact->impulse = impulse;
        impact->point = contact.point;
    }
}

static bool dd2_collision_resolve(dd2_vehicle *vehicle, const dd2_vehicle *previous,
                                  const dd2_road_surface *surface, const dd2_barrier_world *world,
                                  dd2_vehicle_impact *impact) {
    if (impact != NULL) {
        *impact = (dd2_vehicle_impact){0};
    }
    if ((world == NULL && surface == NULL) || !dd2_vehicle_valid(vehicle) ||
        !dd2_vehicle_valid(previous)) {
        return false;
    }
    dd2_vehicle next = *vehicle;
    dd2_vehicle start = *previous;
    dd2_vehicle_impact events = {0};
    double remaining = DD2_VEHICLE_STEP_SECONDS;
    const unsigned limit =
        surface != NULL ? DD2_COLLISION_ITERATIONS : DD2_COLLISION_BARRIER_ITERATIONS;
    for (unsigned iteration = 0; iteration < limit; ++iteration) {
        dd2_barrier_contact contact = {0};
        bool found = world != NULL && dd2_collision_sweep(&start, &next, world, &contact);
        dd2_barrier_contact floor = {0};
        const bool ground = surface != NULL &&
                            dd2_collision_ground_sweep(&start, &next, surface, &floor) &&
                            (!found || floor.time <= contact.time);
        if (ground) {
            contact = floor;
            found = true;
        }
        if (!found) {
            break;
        }
        next.position = dd2_collision_add(
            start.position,
            dd2_collision_scale(
                dd2_collision_add(next.position, dd2_collision_scale(start.position, -1)),
                contact.time));
        next.rotation = dd2_collision_rotation(start.rotation, next.rotation, contact.time);
        dd2_collision_response(&next, contact, ground, &events);
        ++events.contacts;
        next.position = dd2_collision_add(
            next.position,
            dd2_collision_scale(contact.normal, contact.penetration + dd2_collision_clearance));
        remaining *= 1 - contact.time;
        start = next;
        next.position =
            dd2_collision_add(start.position, dd2_collision_scale(next.velocity, remaining));
        next.rotation = dd2_collision_turn(&next, remaining);
        if (iteration + 1 == limit) {
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

bool dd2_vehicle_collide_barriers(dd2_vehicle *vehicle, const dd2_vehicle *previous,
                                  const dd2_barrier_world *world, dd2_vehicle_impact *impact) {
    return dd2_collision_resolve(vehicle, previous, NULL, world, impact);
}

bool dd2_vehicle_collide_world(dd2_vehicle *vehicle, const dd2_vehicle *previous,
                               const dd2_road_surface *surface, const dd2_barrier_world *world,
                               dd2_vehicle_impact *impact) {
    if (surface == NULL) {
        if (impact != NULL) {
            *impact = (dd2_vehicle_impact){0};
        }
        return false;
    }
    return dd2_collision_resolve(vehicle, previous, surface, world, impact);
}

typedef struct {
    dd2_barrier_contact contact;
    unsigned first;
    unsigned second;
    bool ground;
    bool pair;
} dd2_fleet_event;
enum {
    DD2_FLEET_EVENT_LIMIT =
        DD2_VEHICLE_FLEET_LIMIT + (DD2_VEHICLE_FLEET_LIMIT * (DD2_VEHICLE_FLEET_LIMIT - 1) / 2)
};

static bool dd2_fleet_contact(const dd2_vehicle *start, const dd2_vehicle *end, unsigned count,
                              const dd2_road_surface *surface, const dd2_barrier_world *world,
                              dd2_fleet_event *event) {
    dd2_fleet_event events[DD2_FLEET_EVENT_LIMIT] = {0};
    unsigned found = 0;
    double earliest = 1;
    for (unsigned body = 0; body < count; ++body) {
        dd2_barrier_contact contact = {0};
        bool touching =
            world != NULL && dd2_collision_sweep(&start[body], &end[body], world, &contact);
        dd2_barrier_contact floor = {0};
        const bool ground = surface != NULL &&
                            dd2_collision_ground_sweep(&start[body], &end[body], surface, &floor) &&
                            (!touching || floor.time <= contact.time);
        if (ground) {
            contact = floor;
            touching = true;
        }
        if (touching) {
            events[found++] =
                (dd2_fleet_event){.contact = contact, .first = body, .ground = ground};
            earliest = fmin(earliest, contact.time);
        }
    }
    for (unsigned first = 0; first < count; ++first) {
        for (unsigned second = first + 1; second < count; ++second) {
            dd2_car_contact contact = {0};
            if (dd2_car_contact_sweep(&start[first], &end[first], &start[second], &end[second],
                                      &contact)) {
                events[found++] = (dd2_fleet_event){.first = first,
                                                    .second = second,
                                                    .pair = true,
                                                    .contact = {.time = contact.time,
                                                                .penetration = contact.penetration,
                                                                .normal = contact.normal,
                                                                .point = contact.point}};
                earliest = fmin(earliest, contact.time);
            }
        }
    }
    /* Stable world/body order before pair lexicographic order, anchored globally
     * to earliest time. A tolerance chain must not depend on visitation order. */
    for (unsigned index = 0; index < found; ++index) {
        if (events[index].contact.time <= earliest + dd2_collision_time_tolerance) {
            *event = events[index];
            return true;
        }
    }
    return false;
}

static void dd2_fleet_pair_impulse(dd2_vehicle *first, dd2_vehicle *second,
                                   dd2_vehicle_vector first_arm, dd2_vehicle_vector second_arm,
                                   dd2_vehicle_vector impulse) {
    dd2_collision_impulse(first, first_arm, impulse);
    dd2_collision_impulse(second, second_arm, dd2_collision_scale(impulse, -1));
}
static dd2_vehicle_vector dd2_fleet_relative(const dd2_vehicle *first, const dd2_vehicle *second,
                                             dd2_vehicle_vector first_arm,
                                             dd2_vehicle_vector second_arm) {
    return dd2_collision_add(
        dd2_collision_point_velocity(first, first_arm),
        dd2_collision_scale(dd2_collision_point_velocity(second, second_arm), -1));
}

static void dd2_fleet_pair_response(dd2_vehicle *first, dd2_vehicle *second,
                                    dd2_barrier_contact contact, dd2_vehicle_impact *first_impact,
                                    dd2_vehicle_impact *second_impact) {
    const dd2_vehicle_vector first_arm =
        dd2_collision_add(contact.point, dd2_collision_scale(first->position, -1));
    const dd2_vehicle_vector second_arm =
        dd2_collision_add(contact.point, dd2_collision_scale(second->position, -1));
    const double speed =
        dd2_collision_dot(dd2_fleet_relative(first, second, first_arm, second_arm), contact.normal);
    if (speed >= 0) {
        return;
    }
    const double mass = dd2_collision_effective_mass(first, first_arm, contact.normal) +
                        dd2_collision_effective_mass(second, second_arm, contact.normal);
    const double impulse = -(1 + dd2_collision_restitution) * speed / mass;
    dd2_fleet_pair_impulse(first, second, first_arm, second_arm,
                           dd2_collision_scale(contact.normal, impulse));
    const dd2_vehicle_vector relative = dd2_fleet_relative(first, second, first_arm, second_arm);
    dd2_vehicle_vector tangent = dd2_collision_add(
        relative,
        dd2_collision_scale(contact.normal, -dd2_collision_dot(relative, contact.normal)));
    const double length = sqrt(dd2_collision_dot(tangent, tangent));
    if (length > dd2_collision_motion_tolerance) {
        tangent = dd2_collision_scale(tangent, 1 / length);
        const double friction_mass = dd2_collision_effective_mass(first, first_arm, tangent) +
                                     dd2_collision_effective_mass(second, second_arm, tangent);
        const double friction = fmin(dd2_collision_friction * impulse, length / friction_mass);
        dd2_fleet_pair_impulse(first, second, first_arm, second_arm,
                               dd2_collision_scale(tangent, -friction));
    }
    dd2_vehicle_impact *outputs[] = {first_impact, second_impact};
    for (unsigned index = 0; index < 2; ++index) {
        if (-speed > outputs[index]->normal_speed) {
            outputs[index]->normal_speed = -speed;
            outputs[index]->impulse = impulse;
            outputs[index]->point = contact.point;
        }
    }
}

static bool dd2_fleet_validate(dd2_vehicle *vehicles, const dd2_vehicle *previous, unsigned count,
                               dd2_vehicle_impact *impacts, unsigned *pair_contacts) {
    if (pair_contacts != NULL) {
        *pair_contacts = 0;
    }
    if (count == 0 || count > DD2_VEHICLE_FLEET_LIMIT) {
        return false;
    }
    if (impacts != NULL) {
        for (unsigned body = 0; body < count; ++body) {
            impacts[body] = (dd2_vehicle_impact){0};
        }
    }
    if (vehicles == NULL || previous == NULL) {
        return false;
    }
    for (unsigned body = 0; body < count; ++body) {
        if (!dd2_vehicle_valid(&vehicles[body]) || !dd2_vehicle_valid(&previous[body])) {
            return false;
        }
    }
    return true;
}

static void dd2_fleet_response(dd2_vehicle *next, dd2_fleet_event event,
                               dd2_vehicle_impact *events) {
    if (event.pair) {
        dd2_fleet_pair_response(&next[event.first], &next[event.second], event.contact,
                                &events[event.first], &events[event.second]);
        ++events[event.second].contacts;
        ++events[event.first].pair_contacts;
        ++events[event.second].pair_contacts;
        const dd2_vehicle_vector correction = dd2_collision_scale(
            event.contact.normal, (event.contact.penetration / 2) + dd2_collision_clearance);
        next[event.first].position = dd2_collision_add(next[event.first].position, correction);
        next[event.second].position =
            dd2_collision_add(next[event.second].position, dd2_collision_scale(correction, -1));
    } else {
        dd2_collision_response(&next[event.first], event.contact, event.ground,
                               &events[event.first]);
        next[event.first].position = dd2_collision_add(
            next[event.first].position,
            dd2_collision_scale(event.contact.normal,
                                event.contact.penetration + dd2_collision_clearance));
    }
    ++events[event.first].contacts;
}

typedef struct {
    double seconds;
    bool stop;
} dd2_fleet_remaining;
static void dd2_fleet_remainder(dd2_vehicle *start, dd2_vehicle *next, unsigned count,
                                dd2_fleet_remaining remaining) {
    for (unsigned body = 0; body < count; ++body) {
        start[body] = next[body];
        if (!remaining.stop) {
            next[body].position = dd2_collision_add(
                start[body].position, dd2_collision_scale(next[body].velocity, remaining.seconds));
            next[body].rotation = dd2_collision_turn(&next[body], remaining.seconds);
        }
    }
}

bool dd2_vehicle_collide_fleet(dd2_vehicle *vehicles, const dd2_vehicle *previous, unsigned count,
                               const dd2_road_surface *surface, const dd2_barrier_world *world,
                               dd2_vehicle_impact *impacts, unsigned *pair_contacts) {
    if (!dd2_fleet_validate(vehicles, previous, count, impacts, pair_contacts)) {
        return false;
    }
    dd2_vehicle start[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_vehicle next[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_vehicle_impact events[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned body = 0; body < count; ++body) {
        start[body] = previous[body];
        next[body] = vehicles[body];
    }
    double remaining = DD2_VEHICLE_STEP_SECONDS;
    unsigned pairs = 0;
    for (unsigned iteration = 0; iteration < DD2_COLLISION_ITERATIONS; ++iteration) {
        dd2_fleet_event event = {0};
        if (!dd2_fleet_contact(start, next, count, surface, world, &event)) {
            break;
        }
        for (unsigned body = 0; body < count; ++body) {
            next[body].position = dd2_collision_position(start[body].position, next[body].position,
                                                         event.contact.time);
            next[body].rotation = dd2_collision_rotation(start[body].rotation, next[body].rotation,
                                                         event.contact.time);
        }
        dd2_fleet_response(next, event, events);
        pairs += (unsigned)event.pair;
        remaining *= 1 - event.contact.time;
        /* Exhaustion retains every body's last checked pose, never unchecked residual motion. */
        dd2_fleet_remainder(
            start, next, count,
            (dd2_fleet_remaining){.seconds = remaining,
                                  .stop = iteration + 1 == DD2_COLLISION_ITERATIONS});
    }
    for (unsigned body = 0; body < count; ++body) {
        if (!dd2_vehicle_valid(&next[body])) {
            return false;
        }
    }
    for (unsigned body = 0; body < count; ++body) {
        vehicles[body] = next[body];
        if (impacts != NULL) {
            impacts[body] = events[body];
        }
    }
    if (pair_contacts != NULL) {
        *pair_contacts = pairs;
    }
    return true;
}
