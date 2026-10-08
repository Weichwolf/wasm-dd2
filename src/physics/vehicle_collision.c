#include "physics/vehicle_collision.h"

#include "physics/barrier_world.h"
#include "physics/body_geometry.h"
#include "physics/car_contact.h"
#include "physics/collision_math.h"
#include "physics/contact_group.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

struct dd2_vehicle_collision_storage {
    dd2_vehicle_contact contacts[DD2_VEHICLE_REPORT_LIMIT];
};

dd2_vehicle_collision_storage *dd2_vehicle_collision_storage_create(void) {
    return malloc(sizeof(dd2_vehicle_collision_storage));
}

void dd2_vehicle_collision_storage_destroy(dd2_vehicle_collision_storage *storage) {
    free(storage);
}

/* Metadata-only queries use the same response clock without storing entries. */
typedef struct {
    dd2_vehicle_collision_report report;
    dd2_vehicle_contact *contacts;
    double last_time;
} dd2_fleet_recording;

/* Overlapping rounded body lobes fit the 372 x 900 footprint. They avoid
 * snagging a rectangular corner on a strip seam; they are rewrite tuning. */
enum {
    DD2_COLLISION_PROBES = 5,
    DD2_COLLISION_ITERATIONS = DD2_VEHICLE_EVENT_LIMIT,
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
    {.x = DD2_BODY_HALF_WIDTH, .y = -DD2_BODY_HALF_HEIGHT, .z = DD2_BODY_HALF_LENGTH},
    {.x = DD2_BODY_HALF_WIDTH, .y = -DD2_BODY_HALF_HEIGHT, .z = -DD2_BODY_HALF_LENGTH},
    {.x = -DD2_BODY_HALF_WIDTH, .y = -DD2_BODY_HALF_HEIGHT, .z = DD2_BODY_HALF_LENGTH},
    {.x = -DD2_BODY_HALF_WIDTH, .y = -DD2_BODY_HALF_HEIGHT, .z = -DD2_BODY_HALF_LENGTH},
    {.x = DD2_BODY_HALF_WIDTH, .y = DD2_BODY_HALF_HEIGHT, .z = DD2_BODY_HALF_LENGTH},
    {.x = DD2_BODY_HALF_WIDTH, .y = DD2_BODY_HALF_HEIGHT, .z = -DD2_BODY_HALF_LENGTH},
    {.x = -DD2_BODY_HALF_WIDTH, .y = DD2_BODY_HALF_HEIGHT, .z = DD2_BODY_HALF_LENGTH},
    {.x = -DD2_BODY_HALF_WIDTH, .y = DD2_BODY_HALF_HEIGHT, .z = -DD2_BODY_HALF_LENGTH}};

dd2_vehicle_vector dd2_vehicle_body_corner(unsigned corner) {
    return corner < DD2_VEHICLE_BODY_CORNERS ? dd2_collision_corners[corner]
                                             : (dd2_vehicle_vector){0};
}

static bool dd2_collision_sweep(const dd2_vehicle *start, const dd2_vehicle *end,
                                const dd2_barrier_world *world, dd2_barrier_contact *contact,
                                unsigned *support) {
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
            if (support != NULL) {
                *support = probe;
            }
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
                                       dd2_barrier_contact *contact, unsigned *support) {
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
            if (support != NULL) {
                *support = corner;
            }
            break;
        }
    }
    return found;
}

typedef struct {
    double speed;
    double impulse;
} dd2_collision_response_result;

static dd2_collision_response_result dd2_collision_response(dd2_vehicle *vehicle,
                                                            dd2_barrier_contact contact,
                                                            bool ground,
                                                            dd2_vehicle_impact *impact) {
    const dd2_vehicle_vector arm =
        dd2_collision_add(contact.point, dd2_collision_scale(vehicle->position, -1));
    const double speed =
        dd2_collision_dot(dd2_collision_point_velocity(vehicle, arm), contact.normal);
    if (speed >= 0) {
        return (dd2_collision_response_result){0};
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
    return (dd2_collision_response_result){.speed = -speed, .impulse = impulse};
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
        bool found = world != NULL && dd2_collision_sweep(&start, &next, world, &contact, NULL);
        dd2_barrier_contact floor = {0};
        const bool ground = surface != NULL &&
                            dd2_collision_ground_sweep(&start, &next, surface, &floor, NULL) &&
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
    /* Simultaneous body supports use the same inelastic joint solve as a
     * one-car field. Serial impulses can leave another touching corner closing. */
    return dd2_vehicle_collide_fleet(vehicle, previous, 1, surface, world, impact, NULL);
}

typedef struct {
    dd2_barrier_contact contact;
    unsigned first;
    unsigned second;
    unsigned support; /* Body corner or rounded barrier probe, independent of
                         chord error. */
    bool ground;
    bool pair;
    bool unresolved;
} dd2_fleet_event;
enum {
    DD2_FLEET_EVENT_LIMIT =
        DD2_VEHICLE_FLEET_LIMIT + (DD2_VEHICLE_FLEET_LIMIT * (DD2_VEHICLE_FLEET_LIMIT - 1) / 2)
};

static bool dd2_fleet_contact(const dd2_vehicle *start, const dd2_vehicle *end, unsigned count,
                              const dd2_road_surface *surface, const dd2_barrier_world *world,
                              dd2_fleet_event *event, bool *valid) {
    /* Only the completely written prefix below found participates in the
     * earliest-event scan. Unused capacity has no contact meaning. */
    dd2_fleet_event events[DD2_FLEET_EVENT_LIMIT];
    unsigned found = 0;
    double earliest = 1;
    double unresolved_time = 2;
    for (unsigned body = 0; body < count; ++body) {
        unsigned support = 0;
        unsigned floor_support = 0;
        dd2_barrier_contact contact = {0};
        bool touching = world != NULL &&
                        dd2_collision_sweep(&start[body], &end[body], world, &contact, &support);
        dd2_barrier_contact floor = {0};
        const bool ground =
            surface != NULL &&
            dd2_collision_ground_sweep(&start[body], &end[body], surface, &floor, &floor_support) &&
            (!touching || floor.time <= contact.time);
        if (ground) {
            contact = floor;
            support = floor_support;
            touching = true;
        }
        if (touching) {
            events[found++] = (dd2_fleet_event){
                .contact = contact, .first = body, .ground = ground, .support = support};
            earliest = fmin(earliest, contact.time);
        }
    }
    dd2_car_pair_contacts pairs;
    if (!dd2_car_contacts_sweep(&(dd2_car_fleet_motion){.start = start, .end = end, .count = count},
                                &pairs)) {
        *valid = false;
        return false;
    }
    for (unsigned index = 0; index < pairs.count; ++index) {
        const dd2_car_pair_contact pair = pairs.pairs[index];
        const dd2_car_contact contact = pair.contact;
        events[found++] = (dd2_fleet_event){.first = pair.first,
                                            .second = pair.second,
                                            .pair = true,
                                            .unresolved = contact.unresolved,
                                            .contact = {.time = contact.time,
                                                        .penetration = contact.penetration,
                                                        .normal = contact.normal,
                                                        .point = contact.point}};
        earliest = fmin(earliest, contact.time);
        if (contact.unresolved) {
            unresolved_time = fmin(unresolved_time, contact.time);
        }
    }
    /* Stable world/body order before pair lexicographic order, anchored globally
     * to earliest time. A tolerance chain must not depend on visitation order. */
    for (unsigned index = 0; index < found; ++index) {
        if (events[index].contact.time <= earliest + dd2_collision_time_tolerance &&
            events[index].contact.time <= unresolved_time) {
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

static dd2_collision_response_result
dd2_fleet_pair_response(dd2_vehicle *first, dd2_vehicle *second, dd2_barrier_contact contact,
                        dd2_vehicle_impact *first_impact, dd2_vehicle_impact *second_impact) {
    const dd2_vehicle_vector first_arm =
        dd2_collision_add(contact.point, dd2_collision_scale(first->position, -1));
    const dd2_vehicle_vector second_arm =
        dd2_collision_add(contact.point, dd2_collision_scale(second->position, -1));
    const double speed =
        dd2_collision_dot(dd2_fleet_relative(first, second, first_arm, second_arm), contact.normal);
    if (speed >= 0) {
        return (dd2_collision_response_result){0};
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
    return (dd2_collision_response_result){.speed = -speed, .impulse = impulse};
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

static dd2_collision_response_result dd2_fleet_response(dd2_vehicle *next, dd2_fleet_event event,
                                                        dd2_vehicle_impact *events) {
    dd2_collision_response_result response = {0};
    if (event.pair) {
        response = dd2_fleet_pair_response(&next[event.first], &next[event.second], event.contact,
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
        response = dd2_collision_response(&next[event.first], event.contact, event.ground,
                                          &events[event.first]);
        next[event.first].position = dd2_collision_add(
            next[event.first].position,
            dd2_collision_scale(event.contact.normal,
                                event.contact.penetration + dd2_collision_clearance));
    }
    ++events[event.first].contacts;
    return response;
}

static dd2_vehicle_vector dd2_fleet_local_point(const dd2_vehicle *vehicle,
                                                dd2_vehicle_vector point) {
    const dd2_vehicle_rotation inverse = {.x = -vehicle->rotation.x,
                                          .y = -vehicle->rotation.y,
                                          .z = -vehicle->rotation.z,
                                          .w = vehicle->rotation.w};
    return dd2_vehicle_rotate(inverse,
                              dd2_collision_add(point, dd2_collision_scale(vehicle->position, -1)));
}

static dd2_vehicle_contact dd2_fleet_record(const dd2_vehicle *vehicles, dd2_fleet_event event,
                                            double remaining) {
    const dd2_vehicle_contact_kind world_kind =
        event.ground ? DD2_VEHICLE_CONTACT_GROUND : DD2_VEHICLE_CONTACT_BARRIER;
    return (dd2_vehicle_contact){
        .point = event.contact.point,
        .normal = event.contact.normal,
        .local_points = {dd2_fleet_local_point(&vehicles[event.first], event.contact.point),
                         event.pair
                             ? dd2_fleet_local_point(&vehicles[event.second], event.contact.point)
                             : (dd2_vehicle_vector){0}},
        .time = (DD2_VEHICLE_STEP_SECONDS - remaining + (remaining * event.contact.time)) /
                DD2_VEHICLE_STEP_SECONDS,
        .first = event.first,
        .second = event.pair ? event.second : DD2_VEHICLE_NO_PARTNER,
        .obstacle = event.pair ? UINT32_MAX : event.contact.barrier,
        .kind = event.pair ? DD2_VEHICLE_CONTACT_PAIR : world_kind};
}

static dd2_vehicle_contact dd2_fleet_append(dd2_fleet_recording *recording,
                                            dd2_vehicle_contact contact,
                                            dd2_collision_response_result response) {
    dd2_vehicle_collision_report *report = &recording->report;
    /* Residual duration can round independently of the previous normalized
     * timestamp, especially for consecutive overlap repairs at time zero.
     * Keep report chronology exact without changing the solver's event clock. */
    contact.time = fmax(recording->last_time, fmin(1, contact.time));
    contact.normal_speed = response.speed;
    contact.impulse = response.impulse;
    if (recording->contacts != NULL) {
        recording->contacts[report->count] = contact;
    }
    ++report->count;
    recording->last_time = contact.time;
    return contact;
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

/* One primary contact plus every body probe/corner and unordered pair.
 * World constraints are queried only for the primary pair-connected component.
 */
enum {
    DD2_FLEET_GROUP_LIMIT =
        1 + (DD2_VEHICLE_FLEET_LIMIT * (DD2_COLLISION_PROBES + DD2_VEHICLE_BODY_CORNERS)) +
        (DD2_VEHICLE_FLEET_LIMIT * (DD2_VEHICLE_FLEET_LIMIT - 1) / 2)
};
static const double dd2_fleet_group_normal_tolerance = 1e-10;

typedef struct {
    dd2_fleet_event events[DD2_FLEET_GROUP_LIMIT];
    unsigned count;
    bool overflow;
} dd2_fleet_neighborhood;

typedef struct {
    dd2_vehicle *bodies;
    const dd2_road_surface *surface;
    const dd2_barrier_world *world;
    dd2_fleet_event primary;
    unsigned count;
} dd2_fleet_group_query;

static bool dd2_fleet_same_support(dd2_fleet_event first, dd2_fleet_event second) {
    if (first.first != second.first || first.second != second.second || first.pair != second.pair ||
        first.ground != second.ground) {
        return false;
    }
    /* A pair has no world obstacle identity. Primary and static queries can
     * carry different unused barrier values without representing two contacts. */
    if (first.pair) {
        return true;
    }
    if (first.contact.barrier != second.contact.barrier ||
        dd2_collision_dot(first.contact.normal, second.contact.normal) <=
            1 - dd2_fleet_group_normal_tolerance) {
        return false;
    }
    /* Swept chord and instantaneous queries can return different points for
     * the same rotating body corner/probe. Physical identity deduplicates them
     * without merging distinct supports at a road/barrier junction. */
    return first.support == second.support;
}

static void dd2_fleet_group_add(dd2_fleet_neighborhood *group, dd2_fleet_event event) {
    for (unsigned index = 0; index < group->count; ++index) {
        if (dd2_fleet_same_support(group->events[index], event)) {
            /* Keep primary impact metadata, but repair the deepest observed
             * gap at its actual interpolated pose. */
            group->events[index].contact.penetration =
                fmax(group->events[index].contact.penetration, event.contact.penetration);
            return;
        }
    }
    if (group->count == DD2_FLEET_GROUP_LIMIT) {
        group->overflow = true;
        return;
    }
    group->events[group->count++] = event;
}

static void dd2_fleet_near_barriers(const dd2_fleet_group_query *query,
                                    dd2_fleet_neighborhood *group, const bool *connected) {
    if (query->world == NULL) {
        return;
    }
    const double margin = 2 * dd2_collision_clearance;
    for (unsigned body = 0; body < query->count; ++body) {
        if (!connected[body]) {
            continue;
        }
        const dd2_vehicle *vehicle = &query->bodies[body];
        for (unsigned probe = 0; probe < DD2_COLLISION_PROBES; ++probe) {
            const double local_z =
                -dd2_collision_half_length + ((double)probe * 2 * dd2_collision_half_length /
                                              (double)(DD2_COLLISION_PROBES - 1));
            const dd2_vehicle_vector center = dd2_collision_add(
                vehicle->position,
                dd2_vehicle_rotate(vehicle->rotation, (dd2_vehicle_vector){.z = local_z}));
            const dd2_barrier_sweep sweep = {.start = center,
                                             .end = center,
                                             .radius = dd2_collision_radius + margin,
                                             .half_height = dd2_collision_half_height};
            dd2_barrier_contact contact = {0};
            if (dd2_barrier_world_sweep(query->world, sweep, &contact, NULL)) {
                contact.penetration -= margin;
                dd2_fleet_group_add(
                    group, (dd2_fleet_event){.first = body, .contact = contact, .support = probe});
            }
        }
    }
}

static void dd2_fleet_near_ground(const dd2_fleet_group_query *query, dd2_fleet_neighborhood *group,
                                  const bool *connected) {
    if (query->surface == NULL) {
        return;
    }
    const double margin = 2 * dd2_collision_clearance;
    for (unsigned body = 0; body < query->count; ++body) {
        if (!connected[body]) {
            continue;
        }
        const dd2_vehicle *vehicle = &query->bodies[body];
        for (unsigned corner = 0; corner < DD2_VEHICLE_BODY_CORNERS; ++corner) {
            const dd2_vehicle_vector point = dd2_collision_add(
                vehicle->position,
                dd2_vehicle_rotate(vehicle->rotation, dd2_collision_corners[corner]));
            const dd2_surface_sweep sweep = {.start = {point.x, point.y - margin, point.z},
                                             .end = {point.x, point.y - margin, point.z},
                                             .recovery = dd2_collision_ground_recovery};
            dd2_surface_hit hit = {0};
            if (dd2_road_surface_sweep(query->surface, sweep, &hit, NULL)) {
                dd2_fleet_group_add(
                    group,
                    (dd2_fleet_event){
                        .first = body,
                        .support = corner,
                        .ground = true,
                        .contact = {
                            .point = {hit.point.x, hit.point.y, hit.point.z},
                            .normal = {hit.road.normal[0], hit.road.normal[1], hit.road.normal[2]},
                            .penetration = hit.penetration - (margin * hit.road.normal[1]),
                            .barrier = hit.road.cell}});
            }
        }
    }
}

static bool dd2_fleet_near_pairs(const dd2_fleet_group_query *query,
                                 dd2_fleet_neighborhood *group) {
    dd2_car_pair_contacts pairs;
    if (!dd2_car_contacts_proximity(
            &(dd2_car_fleet_neighborhood){.vehicles = query->bodies,
                                          .count = query->count,
                                          .margin = 2 * dd2_collision_clearance},
            &pairs)) {
        return false;
    }
    for (unsigned index = 0; index < pairs.count; ++index) {
        const dd2_car_pair_contact pair = pairs.pairs[index];
        const dd2_car_contact contact = pair.contact;
        dd2_fleet_group_add(group, (dd2_fleet_event){.first = pair.first,
                                                     .second = pair.second,
                                                     .pair = true,
                                                     .contact = {.point = contact.point,
                                                                 .normal = contact.normal,
                                                                 .penetration = contact.penetration,
                                                                 .barrier = UINT32_MAX}});
    }
    return true;
}

static void dd2_fleet_group_connections(const dd2_fleet_group_query *query,
                                        const dd2_fleet_neighborhood *pairs, bool *connected) {
    connected[query->primary.first] = true;
    if (query->primary.pair) {
        connected[query->primary.second] = true;
    }
    for (unsigned pass = 0; pass < query->count; ++pass) {
        for (unsigned index = 0; index < pairs->count; ++index) {
            const dd2_fleet_event event = pairs->events[index];
            if (connected[event.first] || connected[event.second]) {
                connected[event.first] = true;
                connected[event.second] = true;
            }
        }
    }
}

/* World contacts cannot connect separate bodies. Find the pair component first,
 * then query only its bodies, keeping primary/world/pair insertion order
 * intact. */
static bool dd2_fleet_collect_group(const dd2_fleet_group_query *query,
                                    dd2_fleet_neighborhood *group) {
    dd2_fleet_neighborhood pairs;
    pairs.count = 0;
    pairs.overflow = false;
    if (!dd2_fleet_near_pairs(query, &pairs)) {
        return false;
    }
    bool connected[DD2_VEHICLE_FLEET_LIMIT] = {false};
    dd2_fleet_group_connections(query, &pairs, connected);
    dd2_fleet_group_add(group, query->primary);
    dd2_fleet_near_barriers(query, group, connected);
    dd2_fleet_near_ground(query, group, connected);
    for (unsigned index = 0; index < pairs.count; ++index) {
        if (connected[pairs.events[index].first]) {
            dd2_fleet_group_add(group, pairs.events[index]);
        }
    }
    group->overflow = group->overflow || pairs.overflow;
    return true;
}

static double dd2_fleet_group_closing(const dd2_vehicle *bodies, dd2_fleet_event event) {
    const dd2_vehicle_vector first_arm = dd2_collision_add(
        event.contact.point, dd2_collision_scale(bodies[event.first].position, -1));
    dd2_vehicle_vector velocity = dd2_collision_point_velocity(&bodies[event.first], first_arm);
    if (event.pair) {
        const dd2_vehicle_vector second_arm = dd2_collision_add(
            event.contact.point, dd2_collision_scale(bodies[event.second].position, -1));
        velocity =
            dd2_fleet_relative(&bodies[event.first], &bodies[event.second], first_arm, second_arm);
    }
    return fmax(0, -dd2_collision_dot(velocity, event.contact.normal));
}

static void dd2_fleet_group_impact(dd2_vehicle_impact *impact, dd2_vehicle_contact contact) {
    ++impact->contacts;
    impact->pair_contacts += (unsigned)(contact.kind == DD2_VEHICLE_CONTACT_PAIR);
    if (contact.normal_speed > impact->normal_speed) {
        impact->normal_speed = contact.normal_speed;
        impact->impulse = contact.impulse;
        impact->point = contact.point;
    }
}

static bool dd2_fleet_group_response(const dd2_fleet_group_query *query,
                                     const dd2_fleet_neighborhood *group,
                                     dd2_fleet_recording *recorded, double remaining) {
    dd2_group_contact constraints[DD2_VEHICLE_CONTACT_LIMIT];
    dd2_vehicle_contact contacts[DD2_VEHICLE_CONTACT_LIMIT];
    double closing[DD2_VEHICLE_CONTACT_LIMIT];
    for (unsigned index = 0; index < group->count; ++index) {
        dd2_fleet_event event = group->events[index];
        event.contact.time = query->primary.contact.time;
        contacts[index] = dd2_fleet_record(query->bodies, event, remaining);
        closing[index] = dd2_fleet_group_closing(query->bodies, event);
        constraints[index] = (dd2_group_contact){
            .first = event.first,
            .second = event.pair ? event.second : DD2_VEHICLE_NO_PARTNER,
            .point = event.contact.point,
            .normal = event.contact.normal,
            .penetration = event.contact.penetration,
            .friction = event.ground ? dd2_collision_ground_friction : dd2_collision_friction};
    }
    const dd2_fleet_event primary = query->primary;
    dd2_vehicle_impact ignored[DD2_VEHICLE_FLEET_LIMIT] = {0};
    const dd2_collision_response_result rebound =
        primary.pair ? dd2_fleet_pair_response(&query->bodies[primary.first],
                                               &query->bodies[primary.second], primary.contact,
                                               &ignored[primary.first], &ignored[primary.second])
                     : dd2_collision_response(&query->bodies[primary.first], primary.contact,
                                              primary.ground, &ignored[primary.first]);
    dd2_group_solution solution = {0};
    const dd2_group_query solve = {.bodies = query->bodies,
                                   .body_count = query->count,
                                   .contacts = constraints,
                                   .contact_count = group->count};
    if (!dd2_contact_group_solve(&solve, &solution)) {
        return false;
    }
    for (unsigned index = 0; index < group->count; ++index) {
        const dd2_group_response support = solution.contacts[index];
        const dd2_collision_response_result primary_response =
            index == 0 ? rebound : (dd2_collision_response_result){0};
        const double impulse = support.normal_impulse + primary_response.impulse;
        /* Coupled support can carry pressure transferred through another body
         * even when this contact was initially stationary. Its inelastic
         * equivalent incident speed is effective normal mass times impulse. */
        const double speed =
            impulse > 0 ? fmax(primary_response.speed,
                               fmax(closing[index], support.normal_mass * support.normal_impulse))
                        : 0;
        const dd2_collision_response_result response = {.speed = speed, .impulse = impulse};
        const dd2_vehicle_contact contact = dd2_fleet_append(recorded, contacts[index], response);
        dd2_fleet_group_impact(&recorded->report.impacts[contact.first], contact);
        if (contact.kind == DD2_VEHICLE_CONTACT_PAIR) {
            dd2_fleet_group_impact(&recorded->report.impacts[contact.second], contact);
            ++recorded->report.pair_contacts;
        }
    }
    return true;
}

static bool dd2_fleet_respond(const dd2_fleet_group_query *query, dd2_fleet_recording *recorded,
                              bool *serial, double remaining) {
    dd2_fleet_neighborhood group;
    group.count = 0;
    group.overflow = false;
    if (!*serial) {
        if (!dd2_fleet_collect_group(query, &group)) {
            return false;
        }
        *serial = group.overflow || group.count > DD2_VEHICLE_CONTACT_LIMIT;
    }
    if (*serial) {
        const dd2_vehicle_contact contact =
            dd2_fleet_record(query->bodies, query->primary, remaining);
        const dd2_collision_response_result response =
            dd2_fleet_response(query->bodies, query->primary, recorded->report.impacts);
        dd2_fleet_append(recorded, contact, response);
        recorded->report.pair_contacts += (unsigned)query->primary.pair;
        return true;
    }
    return dd2_fleet_group_response(query, &group, recorded, remaining);
}

static bool dd2_fleet_resolve(dd2_vehicle *vehicles, const dd2_vehicle *previous, unsigned count,
                              const dd2_road_surface *surface, const dd2_barrier_world *world,
                              dd2_vehicle_impact *impacts, unsigned *pair_contacts,
                              dd2_vehicle_collision_storage *storage,
                              dd2_vehicle_collision_report *report) {
    if (report != NULL) {
        *report = (dd2_vehicle_collision_report){0};
    }
    if (!dd2_fleet_validate(vehicles, previous, count, impacts, pair_contacts) ||
        (report != NULL && storage == NULL)) {
        return false;
    }
    dd2_vehicle start[DD2_VEHICLE_FLEET_LIMIT];
    dd2_vehicle next[DD2_VEHICLE_FLEET_LIMIT];
    dd2_fleet_recording recorded = {.contacts = report != NULL ? storage->contacts : NULL};
    recorded.report.contacts = recorded.contacts;
    for (unsigned body = 0; body < count; ++body) {
        start[body] = previous[body];
        next[body] = vehicles[body];
    }
    double remaining = DD2_VEHICLE_STEP_SECONDS;
    bool serial = false;
    for (unsigned iteration = 0; iteration < DD2_COLLISION_ITERATIONS; ++iteration) {
        dd2_fleet_event event = {0};
        bool valid = true;
        const bool found = dd2_fleet_contact(start, next, count, surface, world, &event, &valid);
        if (!valid) {
            return false;
        }
        if (!found) {
            break;
        }
        for (unsigned body = 0; body < count; ++body) {
            next[body].position = dd2_collision_position(start[body].position, next[body].position,
                                                         event.contact.time);
            next[body].rotation = dd2_collision_rotation(start[body].rotation, next[body].rotation,
                                                         event.contact.time);
        }
        if (event.unresolved) {
            /* A conservative time bound is not a physical collision. Keep
             * checked poses without inventing impulse, damage or attribution. */
            ++recorded.report.unresolved_sweeps;
            break;
        }
        const dd2_fleet_group_query query = {
            .bodies = next, .count = count, .surface = surface, .world = world, .primary = event};
        if (!dd2_fleet_respond(&query, &recorded, &serial, remaining)) {
            return false;
        }
        ++recorded.report.response_events;
        remaining *= 1 - event.contact.time;
        /* Exhaustion retains every body's last checked pose, never unchecked
         * residual motion. */
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
            impacts[body] = recorded.report.impacts[body];
        }
    }
    if (pair_contacts != NULL) {
        *pair_contacts = recorded.report.pair_contacts;
    }
    if (report != NULL) {
        *report = recorded.report;
    }
    return true;
}

bool dd2_vehicle_collide_fleet(dd2_vehicle *vehicles, const dd2_vehicle *previous, unsigned count,
                               const dd2_road_surface *surface, const dd2_barrier_world *world,
                               dd2_vehicle_impact *impacts, unsigned *pair_contacts) {
    return dd2_fleet_resolve(vehicles, previous, count, surface, world, impacts, pair_contacts,
                             NULL, NULL);
}

bool dd2_vehicle_collide_fleet_report(dd2_vehicle *vehicles, const dd2_vehicle *previous,
                                      unsigned count, const dd2_road_surface *surface,
                                      const dd2_barrier_world *world,
                                      dd2_vehicle_collision_storage *storage,
                                      dd2_vehicle_collision_report *report) {
    return dd2_fleet_resolve(vehicles, previous, count, surface, world, NULL, NULL, storage,
                             report);
}
