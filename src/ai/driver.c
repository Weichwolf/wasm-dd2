#include "ai/driver.h"

#include "ai/path.h"
#include "assets/road.h"
#include "physics/numeric.h"
#include "physics/road_contact.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

enum { DD2_AI_STUCK_STEPS = 300, DD2_AI_REVERSE_STEPS = 300, DD2_AI_RETARGET_STEPS = 400 };
static const double dd2_ai_wheelbase = 900;
static const double dd2_ai_steer_limit = 0.39269908169872415481;
static const double dd2_ai_lookahead = 1000;
static const double dd2_ai_look_seconds = 0.6;
static const double dd2_ai_preview_seconds = 0.8;
static const double dd2_ai_racing_speed = 3200;
static const double dd2_ai_arena_speed = 2000;
static const double dd2_ai_corner_load = 650;
static const double dd2_ai_control_speed = 600;
static const double dd2_ai_stuck_speed = 80;
static const double dd2_ai_reverse_speed = 500;
/* A car must leave this radius within 1.5 seconds to count as making progress.
 * Solver velocities alone can remain nonzero when the contact budget limits travel. */
static const double dd2_ai_progress_distance = 120;
static const double dd2_ai_body_height = 400;
static const double dd2_ai_side_clearance = 450;
static const double dd2_ai_follow_distance = 1100;
static const double dd2_ai_lane_shift = 0.18;
static const double dd2_ai_lane_rate = 0.3;
static const double dd2_ai_surface_window = 500;
static const double dd2_ai_min_axis = 0.2;
static const double dd2_ai_yaw_damping = 0.2;
static const double dd2_ai_lead_seconds = 0.5;

typedef struct {
    dd2_vehicle_vector point;
    double speed;
    bool found;
} dd2_ai_goal;

static double dd2_ai_clamp(double value) {
    return fmax(-1, fmin(1, value));
}
static dd2_vehicle_vector dd2_ai_relative(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return (dd2_vehicle_vector){
        .x = first.x - second.x, .y = first.y - second.y, .z = first.z - second.z};
}
static double dd2_ai_horizontal_dot(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return (first.x * second.x) + (first.z * second.z);
}

bool dd2_ai_driver_reset(dd2_ai_driver *driver, dd2_ai_start start) {
    if (driver == NULL || start.road == NULL || start.cell >= dd2_road_cell_count(start.road) ||
        start.count == 0 || start.count > DD2_VEHICLE_FLEET_LIMIT || start.slot >= start.count) {
        return false;
    }
    const dd2_road_cell cell = dd2_road_cells(start.road)[start.cell];
    const double lane = cell.strip == DD2_ROAD_NO_STRIP
                            ? (1.0 / 2)
                            : ((double)cell.lane + (1.0 / 2)) /
                                  (double)dd2_road_strips(start.road)[cell.strip].lanes;
    *driver = (dd2_ai_driver){.cell = start.cell,
                              .lane = lane,
                              .base_lane = lane,
                              .target = (start.slot + (start.count / 2)) % start.count};
    return true;
}

static bool dd2_ai_valid(const dd2_ai_driver *driver, const dd2_ai_observation *observation) {
    if (driver == NULL || observation == NULL || observation->road == NULL ||
        observation->surface == NULL || observation->vehicles == NULL || observation->count == 0 ||
        observation->count > DD2_VEHICLE_FLEET_LIMIT || observation->slot >= observation->count ||
        (observation->pursue_player &&
         (observation->slot == 0 || dd2_road_strip_count(observation->road) != 0)) ||
        driver->cell >= dd2_road_cell_count(observation->road) ||
        driver->target >= observation->count || driver->steps == UINT64_MAX ||
        driver->stuck_steps > DD2_AI_STUCK_STEPS || driver->reverse_steps > DD2_AI_REVERSE_STEPS ||
        driver->progress_steps > DD2_AI_STUCK_STEPS ||
        !dd2_numeric_finite(&driver->progress_origin.x) ||
        !dd2_numeric_finite(&driver->progress_origin.y) ||
        !dd2_numeric_finite(&driver->progress_origin.z) || !dd2_numeric_finite(&driver->lane) ||
        driver->lane < 0 || driver->lane > 1 || !dd2_numeric_finite(&driver->base_lane) ||
        driver->base_lane < 0 || driver->base_lane > 1) {
        return false;
    }
    for (unsigned slot = 0; slot < observation->count; ++slot) {
        if (!dd2_vehicle_valid(&observation->vehicles[slot])) {
            return false;
        }
    }
    return true;
}

typedef struct {
    double distance;
    double speed;
    bool found;
} dd2_ai_traffic;
static dd2_ai_traffic dd2_ai_obstacle(const dd2_ai_observation *observation,
                                      dd2_vehicle_vector forward) {
    const dd2_vehicle *vehicle = &observation->vehicles[observation->slot];
    const dd2_vehicle_vector right = {.x = forward.z, .z = -forward.x};
    dd2_ai_traffic nearest = {.distance = dd2_ai_follow_distance + dd2_ai_racing_speed};
    for (unsigned slot = 0; slot < observation->count; ++slot) {
        if (slot == observation->slot) {
            continue;
        }
        const dd2_vehicle *other = &observation->vehicles[slot];
        const dd2_vehicle_vector relative = dd2_ai_relative(other->position, vehicle->position);
        const double distance = dd2_ai_horizontal_dot(relative, forward);
        if (distance > 0 && distance < nearest.distance && fabs(relative.y) < dd2_ai_body_height &&
            fabs(dd2_ai_horizontal_dot(relative, right)) < dd2_ai_side_clearance) {
            nearest = (dd2_ai_traffic){.distance = distance,
                                       .speed = dd2_ai_horizontal_dot(other->velocity, forward),
                                       .found = true};
        }
    }
    return nearest;
}

static void dd2_ai_locate(dd2_ai_driver *driver, const dd2_ai_observation *observation) {
    const dd2_vehicle *vehicle = &observation->vehicles[observation->slot];
    dd2_road_contact contact = {0};
    if (dd2_road_surface_sample(
            observation->surface,
            (dd2_surface_query){.point = {.x = vehicle->position.x, .z = vehicle->position.z},
                                .min_height = vehicle->position.y - dd2_ai_surface_window,
                                .max_height = vehicle->position.y,
                                .preferred_cell = driver->cell},
            &contact, NULL)) {
        driver->cell = contact.cell;
    }
}

static dd2_vehicle_vector dd2_ai_toward(dd2_vehicle_vector point, dd2_vehicle_vector position) {
    const dd2_vehicle_vector relative = dd2_ai_relative(point, position);
    const double length = hypot(relative.x, relative.z);
    return length > dd2_ai_min_axis
               ? (dd2_vehicle_vector){.x = relative.x / length, .z = relative.z / length}
               : (dd2_vehicle_vector){0};
}

static double dd2_ai_passing_lane(const dd2_ai_driver *driver,
                                  const dd2_ai_observation *observation, dd2_ai_path_query query) {
    double lane = driver->base_lane;
    double best = -1;
    const double preference =
        (observation->slot & 1U) == 0 ? dd2_ai_lane_shift : -dd2_ai_lane_shift;
    const double shifts[] = {preference, -preference};
    for (unsigned candidate = 0; candidate < 2; ++candidate) {
        query.lane = fmax(0, fmin(1, driver->base_lane + shifts[candidate]));
        dd2_ai_path_sample aim = {0};
        if (!dd2_ai_path(observation->road, query, &aim)) {
            continue;
        }
        const dd2_vehicle_vector direction = dd2_ai_toward(aim.point, query.position);
        const dd2_ai_traffic traffic = dd2_ai_obstacle(observation, direction);
        if (traffic.distance > best) {
            best = traffic.distance;
            lane = query.lane;
        }
    }
    return lane;
}

static dd2_ai_goal dd2_ai_racing(dd2_ai_driver *driver, const dd2_ai_observation *observation,
                                 dd2_vehicle_vector forward) {
    const dd2_vehicle *vehicle = &observation->vehicles[observation->slot];
    const double speed = hypot(vehicle->velocity.x, vehicle->velocity.z);
    dd2_ai_path_query query = {.cell = driver->cell,
                               .lane = driver->lane,
                               .position = vehicle->position,
                               .distance = dd2_ai_lookahead + (speed * dd2_ai_look_seconds)};
    const double target_lane = dd2_ai_obstacle(observation, forward).found
                                   ? dd2_ai_passing_lane(driver, observation, query)
                                   : driver->base_lane;
    driver->lane +=
        fmax(-dd2_ai_lane_rate * DD2_VEHICLE_STEP_SECONDS,
             fmin(dd2_ai_lane_rate * DD2_VEHICLE_STEP_SECONDS, target_lane - driver->lane));
    dd2_ai_path_sample aim = {0};
    dd2_ai_path_sample preview = {0};
    query.lane = driver->lane;
    if (!dd2_ai_path(observation->road, query, &aim)) {
        return (dd2_ai_goal){0};
    }
    query.distance += speed * dd2_ai_preview_seconds;
    if (!dd2_ai_path(observation->road, query, &preview)) {
        return (dd2_ai_goal){0};
    }
    double wanted = dd2_ai_racing_speed;
    if (preview.curvature > 0) {
        wanted = fmin(wanted, sqrt(dd2_ai_corner_load / preview.curvature));
    }
    const dd2_ai_traffic traffic =
        dd2_ai_obstacle(observation, dd2_ai_toward(aim.point, vehicle->position));
    if (traffic.found) {
        wanted = fmin(wanted, fmax(0, traffic.speed + ((traffic.distance - dd2_ai_follow_distance) *
                                                       dd2_ai_lead_seconds)));
    }
    return (dd2_ai_goal){.point = aim.point, .speed = wanted, .found = true};
}

static unsigned dd2_ai_target(const dd2_ai_observation *observation, dd2_vehicle_vector forward) {
    const dd2_vehicle *vehicle = &observation->vehicles[observation->slot];
    unsigned target = observation->slot;
    double nearest = dd2_ai_racing_speed * dd2_ai_racing_speed;
    nearest *= nearest;
    for (unsigned slot = 0; slot < observation->count; ++slot) {
        const dd2_vehicle_vector relative =
            dd2_ai_relative(observation->vehicles[slot].position, vehicle->position);
        if (slot == observation->slot || fabs(relative.y) > dd2_ai_body_height) {
            continue;
        }
        double distance = dd2_ai_horizontal_dot(relative, relative);
        if (dd2_ai_horizontal_dot(relative, forward) < 0) {
            distance *= 2;
        }
        if (distance < nearest) {
            nearest = distance;
            target = slot;
        }
    }
    return target;
}

static dd2_ai_goal dd2_ai_arena(dd2_ai_driver *driver, const dd2_ai_observation *observation,
                                dd2_vehicle_vector forward) {
    if (!observation->pursue_player && driver->steps != 0 &&
        driver->steps % DD2_AI_RETARGET_STEPS == 0) {
        driver->target = dd2_ai_target(observation, forward);
    }
    if (driver->target == observation->slot) {
        return (dd2_ai_goal){0};
    }
    const dd2_vehicle *target = &observation->vehicles[driver->target];
    return (dd2_ai_goal){
        .point = {.x = target->position.x + (target->velocity.x * dd2_ai_lead_seconds),
                  .y = target->position.y,
                  .z = target->position.z + (target->velocity.z * dd2_ai_lead_seconds)},
        .speed = dd2_ai_arena_speed,
        .found = true};
}

static bool dd2_ai_progress_stalled(dd2_ai_driver *driver, dd2_vehicle_vector position) {
    const double distance =
        hypot(position.x - driver->progress_origin.x, position.z - driver->progress_origin.z);
    if (driver->progress_steps == 0 || driver->reverse_steps != 0 ||
        distance >= dd2_ai_progress_distance) {
        driver->progress_origin = position;
        driver->progress_steps = 0;
    }
    if (driver->reverse_steps != 0) {
        return false;
    }
    if (driver->progress_steps < DD2_AI_STUCK_STEPS) {
        ++driver->progress_steps;
    }
    return driver->progress_steps == DD2_AI_STUCK_STEPS;
}

static dd2_vehicle_control dd2_ai_control(dd2_ai_driver *driver, const dd2_vehicle *vehicle,
                                          dd2_vehicle_vector forward, dd2_ai_goal goal) {
    const dd2_vehicle_vector relative = dd2_ai_relative(goal.point, vehicle->position);
    const double distance = hypot(relative.x, relative.z);
    if (!goal.found || distance < dd2_ai_min_axis) {
        driver->progress_steps = 0;
        return (dd2_vehicle_control){.brake = 1};
    }
    const dd2_vehicle_vector right = {.x = forward.z, .z = -forward.x};
    const double sideways = dd2_ai_horizontal_dot(relative, right) / distance;
    const double facing = dd2_ai_horizontal_dot(relative, forward) / distance;
    const double speed = dd2_ai_horizontal_dot(vehicle->velocity, forward);
    const double lookahead = dd2_ai_lookahead + (fabs(speed) * dd2_ai_look_seconds);
    const double steer =
        dd2_ai_clamp((atan2(2 * dd2_ai_wheelbase * sideways, lookahead) / dd2_ai_steer_limit) -
                     (vehicle->angular_velocity.y * dd2_ai_yaw_damping));
    const double wanted = goal.speed * fmax(dd2_ai_min_axis, facing);
    dd2_vehicle_control control = {
        .throttle = fmax(0, dd2_ai_clamp((wanted - speed) / dd2_ai_control_speed)),
        .brake = fmax(0, dd2_ai_clamp((speed - wanted) / dd2_ai_control_speed)),
        .steer = steer};
    if (fabs(speed) < dd2_ai_stuck_speed) {
        if (driver->stuck_steps < DD2_AI_STUCK_STEPS) {
            ++driver->stuck_steps;
        }
    } else {
        driver->stuck_steps = 0;
    }
    const bool progress_stalled = dd2_ai_progress_stalled(driver, vehicle->position);
    if (driver->reverse_steps == 0 &&
        (driver->stuck_steps == DD2_AI_STUCK_STEPS || progress_stalled ||
         (facing < -dd2_ai_lead_seconds && fabs(speed) < dd2_ai_reverse_speed))) {
        driver->reverse_steps = DD2_AI_REVERSE_STEPS;
        driver->stuck_steps = 0;
        driver->progress_steps = 0;
    }
    if (driver->reverse_steps != 0) {
        --driver->reverse_steps;
        control = (dd2_vehicle_control){.throttle = -dd2_ai_look_seconds, .steer = -steer};
    }
    return control;
}

bool dd2_ai_driver_step(dd2_ai_driver *driver, const dd2_ai_observation *observation,
                        dd2_vehicle_control *control) {
    if (control == NULL) {
        return false;
    }
    *control = (dd2_vehicle_control){0};
    if (!dd2_ai_valid(driver, observation)) {
        return false;
    }
    dd2_ai_driver next = *driver;
    if (observation->pursue_player) {
        next.target = 0;
    }
    const dd2_vehicle *vehicle = &observation->vehicles[observation->slot];
    dd2_vehicle_vector forward =
        dd2_vehicle_rotate(vehicle->rotation, (dd2_vehicle_vector){.z = 1});
    const double length = hypot(forward.x, forward.z);
    dd2_vehicle_control decision = {.brake = 1};
    if (length >= dd2_ai_min_axis &&
        dd2_vehicle_rotate(vehicle->rotation, (dd2_vehicle_vector){.y = 1}).y >= dd2_ai_min_axis) {
        forward.x /= length;
        forward.z /= length;
        dd2_ai_locate(&next, observation);
        const dd2_ai_goal goal = dd2_road_strip_count(observation->road) != 0
                                     ? dd2_ai_racing(&next, observation, forward)
                                     : dd2_ai_arena(&next, observation, forward);
        decision = dd2_ai_control(&next, vehicle, forward, goal);
    } else {
        next.progress_steps = 0;
    }
    ++next.steps;
    *driver = next;
    *control = decision;
    return true;
}
