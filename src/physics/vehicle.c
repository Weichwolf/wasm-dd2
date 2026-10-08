#include "physics/vehicle.h"

#include "assets/road.h"
#include "physics/body_surface.h"
#include "physics/numeric.h"
#include "physics/road_contact.h"
#include "physics/road_surface.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Wheel XZ positions come from the original wheel rig (FUN_00440bf4).
 * Spring/damper and drive values are rewrite tuning in world units and seconds;
 * the original visual suspension recurrence is not a rigid-body spring. */
static const double dd2_vehicle_half_width = 186;
static const double dd2_vehicle_half_length = 450;
static const double dd2_vehicle_mount_height = -60;
static const double dd2_vehicle_radius = 60;
static const double dd2_vehicle_travel = 70;
static const double dd2_vehicle_gravity = 2500;
static const double dd2_vehicle_spring = 90;
static const double dd2_vehicle_damper = 10;
static const double dd2_vehicle_tire_grip = 1.3;
static const double dd2_vehicle_drive = 1800;
static const double dd2_vehicle_forward_limit = 9000;
static const double dd2_vehicle_reverse_limit = 4500;
static const double dd2_vehicle_lateral_rate = 8;
static const double dd2_vehicle_brake_rate = 12;
static const double dd2_vehicle_rolling_rate = 0.1;
static const double dd2_vehicle_air_drag = 0.1;
static const double dd2_vehicle_angular_drag = 0.1;
static const double dd2_vehicle_steer_limit = 0.39269908169872415481;
static const double dd2_vehicle_steer_rate = 2.5;
static const double dd2_vehicle_min_up = 0.2;
static const double dd2_vehicle_bump_window = 15;
static const double dd2_vehicle_body_grip = 0.5;
static const double dd2_vehicle_loose_grip = 0.5;
static const double dd2_vehicle_position_limit = 2147483648.0;
static const double dd2_vehicle_velocity_limit = 1000000;
static const double dd2_vehicle_angular_limit = 64;
static const double dd2_vehicle_yaw_limit = 1000000;
static const double dd2_vehicle_rotation_tolerance = 1e-6;
static const dd2_vehicle_vector dd2_vehicle_inertia = {
    /* Rectangular body inertia / mass: lengths 900 x 372 x 260. */
    .x = 73133.333333333333,
    .y = 79032,
    .z = 17165.333333333333};

static dd2_vehicle_vector dd2_vehicle_add(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return (dd2_vehicle_vector){
        .x = first.x + second.x, .y = first.y + second.y, .z = first.z + second.z};
}

static dd2_vehicle_vector dd2_vehicle_scale(dd2_vehicle_vector vector, double factor) {
    return (dd2_vehicle_vector){
        .x = vector.x * factor, .y = vector.y * factor, .z = vector.z * factor};
}

static double dd2_vehicle_dot(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return (first.x * second.x) + (first.y * second.y) + (first.z * second.z);
}

static dd2_vehicle_vector dd2_vehicle_cross(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return (dd2_vehicle_vector){.x = (first.y * second.z) - (first.z * second.y),
                                .y = (first.z * second.x) - (first.x * second.z),
                                .z = (first.x * second.y) - (first.y * second.x)};
}

dd2_vehicle_vector dd2_vehicle_rotate(dd2_vehicle_rotation rotation, dd2_vehicle_vector vector) {
    const dd2_vehicle_vector axis = {.x = rotation.x, .y = rotation.y, .z = rotation.z};
    const dd2_vehicle_vector cross = dd2_vehicle_scale(dd2_vehicle_cross(axis, vector), 2);
    return dd2_vehicle_add(vector, dd2_vehicle_add(dd2_vehicle_scale(cross, rotation.w),
                                                   dd2_vehicle_cross(axis, cross)));
}

static dd2_vehicle_rotation dd2_vehicle_inverse(dd2_vehicle_rotation rotation) {
    return (dd2_vehicle_rotation){
        .x = -rotation.x, .y = -rotation.y, .z = -rotation.z, .w = rotation.w};
}

static double dd2_vehicle_rotation_length(dd2_vehicle_rotation rotation) {
    return (rotation.x * rotation.x) + (rotation.y * rotation.y) + (rotation.z * rotation.z) +
           (rotation.w * rotation.w);
}

static bool dd2_vehicle_number_valid(const double *number, double limit) {
    return dd2_numeric_finite(number) && fabs(*number) <= limit;
}

static bool dd2_vehicle_vector_valid(const dd2_vehicle_vector *vector, double limit) {
    return dd2_vehicle_number_valid(&vector->x, limit) &&
           dd2_vehicle_number_valid(&vector->y, limit) &&
           dd2_vehicle_number_valid(&vector->z, limit);
}

bool dd2_vehicle_valid(const dd2_vehicle *vehicle) {
    if (vehicle == NULL) {
        return false;
    }
    const dd2_vehicle_rotation *rotation = &vehicle->rotation;
    return dd2_vehicle_vector_valid(&vehicle->position, dd2_vehicle_position_limit) &&
           dd2_vehicle_vector_valid(&vehicle->velocity, dd2_vehicle_velocity_limit) &&
           dd2_vehicle_vector_valid(&vehicle->angular_velocity, dd2_vehicle_angular_limit) &&
           dd2_vehicle_number_valid(&vehicle->steering, dd2_vehicle_steer_limit) &&
           dd2_vehicle_number_valid(&rotation->x, 1) && dd2_vehicle_number_valid(&rotation->y, 1) &&
           dd2_vehicle_number_valid(&rotation->z, 1) && dd2_vehicle_number_valid(&rotation->w, 1) &&
           fabs(dd2_vehicle_rotation_length(*rotation) - 1) <= dd2_vehicle_rotation_tolerance;
}

bool dd2_vehicle_reset(dd2_vehicle *vehicle, dd2_vehicle_spawn spawn) {
    if (vehicle == NULL || !dd2_vehicle_vector_valid(&spawn.position, dd2_vehicle_position_limit) ||
        !dd2_vehicle_number_valid(&spawn.yaw, dd2_vehicle_yaw_limit)) {
        return false;
    }
    *vehicle = (dd2_vehicle){.position = spawn.position,
                             .rotation = {.y = sin(spawn.yaw / 2), .w = cos(spawn.yaw / 2)}};
    for (size_t index = 0; index < DD2_VEHICLE_WHEELS; ++index) {
        vehicle->wheels[index].contact.cell = DD2_ROAD_NO_STRIP;
    }
    return true;
}

static dd2_vehicle_vector dd2_vehicle_mount(size_t wheel) {
    return (dd2_vehicle_vector){.x = wheel < 2 ? dd2_vehicle_half_width : -dd2_vehicle_half_width,
                                .y = dd2_vehicle_mount_height,
                                .z = (wheel & 1U) == 0 ? dd2_vehicle_half_length
                                                       : -dd2_vehicle_half_length};
}

static dd2_vehicle_vector dd2_vehicle_point_velocity(const dd2_vehicle *vehicle,
                                                     dd2_vehicle_vector arm) {
    return dd2_vehicle_add(vehicle->velocity, dd2_vehicle_cross(vehicle->angular_velocity, arm));
}

static double dd2_vehicle_move_toward(double value, double target, double change) {
    return value + fmax(-change, fmin(change, target - value));
}

typedef struct {
    dd2_vehicle_vector linear;
    dd2_vehicle_vector angular;
} dd2_vehicle_force;
typedef struct {
    const dd2_body_surface *field;
    dd2_vehicle_force *reactions;
    unsigned body;
} dd2_vehicle_body_support;

static dd2_vehicle_vector dd2_vehicle_tire_force(const dd2_vehicle *vehicle, const dd2_road *road,
                                                 const dd2_vehicle_wheel *wheel, size_t index,
                                                 dd2_vehicle_control control,
                                                 dd2_vehicle_vector surface_velocity) {
    const dd2_vehicle_vector normal = {.x = wheel->contact.normal[0],
                                       .y = wheel->contact.normal[1],
                                       .z = wheel->contact.normal[2]};
    const double angle = (index & 1U) == 0 ? vehicle->steering : 0;
    dd2_vehicle_vector forward = dd2_vehicle_rotate(
        vehicle->rotation, (dd2_vehicle_vector){.x = sin(angle), .z = cos(angle)});
    forward =
        dd2_vehicle_add(forward, dd2_vehicle_scale(normal, -dd2_vehicle_dot(forward, normal)));
    const double length = sqrt(dd2_vehicle_dot(forward, forward));
    if (length < dd2_vehicle_min_up) {
        return (dd2_vehicle_vector){0};
    }
    forward = dd2_vehicle_scale(forward, 1 / length);
    const dd2_vehicle_vector right = dd2_vehicle_cross(normal, forward);
    const dd2_vehicle_vector arm =
        dd2_vehicle_add(wheel->support == DD2_VEHICLE_WHEEL_BODY ? wheel->point : wheel->mount,
                        dd2_vehicle_scale(vehicle->position, -1));
    const dd2_vehicle_vector point_velocity = dd2_vehicle_point_velocity(vehicle, arm);
    const dd2_vehicle_vector velocity =
        wheel->support == DD2_VEHICLE_WHEEL_BODY
            ? dd2_vehicle_add(point_velocity, dd2_vehicle_scale(surface_velocity, -1))
            : point_velocity;
    const double speed = dd2_vehicle_dot(velocity, forward);
    const double maximum =
        control.throttle >= 0 ? dd2_vehicle_forward_limit : dd2_vehicle_reverse_limit;
    /* Opposing throttle remains available while changing direction. */
    const double ratio = speed * control.throttle >= 0 ? speed / maximum : 0;
    double longitudinal =
        (control.throttle * dd2_vehicle_drive * fmax(0, 1 - (ratio * ratio)) /
         (double)DD2_VEHICLE_WHEELS) -
        (speed * ((control.brake * dd2_vehicle_brake_rate) + dd2_vehicle_rolling_rate) /
         (double)DD2_VEHICLE_WHEELS);
    double lateral =
        -dd2_vehicle_dot(velocity, right) * dd2_vehicle_lateral_rate / (double)DD2_VEHICLE_WHEELS;
    double surface_scale = dd2_vehicle_body_grip;
    if (wheel->support == DD2_VEHICLE_WHEEL_ROAD) {
        const uint8_t flags = dd2_road_cells(road)[wheel->contact.cell].surface_flags;
        surface_scale = (flags & 2U) != 0 ? dd2_vehicle_loose_grip : 1;
    }
    const double limit = wheel->load * dd2_vehicle_tire_grip * surface_scale;
    const double demand = hypot(longitudinal, lateral);
    if (demand > limit && demand > 0) {
        longitudinal *= limit / demand;
        lateral *= limit / demand;
    }
    return dd2_vehicle_add(dd2_vehicle_scale(forward, longitudinal),
                           dd2_vehicle_scale(right, lateral));
}

static dd2_vehicle_vector dd2_vehicle_wheel_force(dd2_vehicle *vehicle, const dd2_road *road,
                                                  const dd2_road_surface *surface, size_t index,
                                                  dd2_vehicle_control control,
                                                  const dd2_vehicle_body_support *support) {
    dd2_vehicle_wheel *wheel = &vehicle->wheels[index];
    const uint32_t preferred = wheel->contact.cell;
    const dd2_vehicle_vector arm = dd2_vehicle_rotate(vehicle->rotation, dd2_vehicle_mount(index));
    const dd2_vehicle_vector up_axis =
        dd2_vehicle_rotate(vehicle->rotation, (dd2_vehicle_vector){.y = 1});
    *wheel = (dd2_vehicle_wheel){.mount = dd2_vehicle_add(vehicle->position, arm),
                                 .contact = {.cell = DD2_ROAD_NO_STRIP}};
    wheel->center = dd2_vehicle_add(wheel->mount, dd2_vehicle_scale(up_axis, -dd2_vehicle_travel));
    if (up_axis.y < dd2_vehicle_min_up) {
        return (dd2_vehicle_vector){0};
    }
    const dd2_surface_query query = {.point = {.x = wheel->center.x, .z = wheel->center.z},
                                     .min_height = wheel->center.y - dd2_vehicle_radius,
                                     .max_height = wheel->mount.y - dd2_vehicle_radius +
                                                   dd2_vehicle_bump_window,
                                     .preferred_cell = preferred};
    dd2_body_surface_contact body_contact = {0};
    const dd2_body_wheel_query body_query = {.mount = wheel->mount,
                                             .axis = up_axis,
                                             .radius = dd2_vehicle_radius,
                                             .min_displacement = -dd2_vehicle_bump_window,
                                             .max_displacement = dd2_vehicle_travel};
    const bool body_found =
        support != NULL &&
        dd2_body_surface_wheel_sample(support->field, &body_query, support->body, &body_contact);
    dd2_surface_query road_query = query;
    if (body_found) {
        const double threshold =
            wheel->mount.y - dd2_vehicle_radius -
            ((body_contact.minimum_displacement + DD2_ROAD_EDGE_TOLERANCE) * up_axis.y);
        road_query.min_height = fmax(query.min_height, threshold);
    }
    const bool road_found = dd2_road_surface_sample(surface, road_query, &wheel->contact, NULL) &&
                            wheel->contact.normal[1] >= dd2_vehicle_min_up;
    dd2_vehicle_vector surface_velocity = {0};
    if (body_found && !road_found) {
        wheel->support = DD2_VEHICLE_WHEEL_BODY;
        wheel->body = body_contact.body;
        wheel->point = body_contact.point;
        surface_velocity = body_contact.velocity;
        wheel->contact = (dd2_road_contact){
            .height = body_contact.point.y,
            .normal = {body_contact.normal.x, body_contact.normal.y, body_contact.normal.z},
            .cell = DD2_ROAD_NO_STRIP};
    } else if (road_found) {
        wheel->support = DD2_VEHICLE_WHEEL_ROAD;
        wheel->point = (dd2_vehicle_vector){
            .x = query.point.x, .y = wheel->contact.height, .z = query.point.z};
    } else {
        wheel->contact.cell = DD2_ROAD_NO_STRIP;
        return (dd2_vehicle_vector){0};
    }
    wheel->grounded = true;
    const double displacement =
        wheel->support == DD2_VEHICLE_WHEEL_BODY
            ? body_contact.displacement
            : (wheel->mount.y - dd2_vehicle_radius - wheel->contact.height) / up_axis.y;
    wheel->compression = fmax(0, fmin(dd2_vehicle_travel, dd2_vehicle_travel - displacement));
    wheel->center = dd2_vehicle_add(
        wheel->mount, dd2_vehicle_scale(up_axis, wheel->compression - dd2_vehicle_travel));
    const dd2_vehicle_vector normal = {.x = wheel->contact.normal[0],
                                       .y = wheel->contact.normal[1],
                                       .z = wheel->contact.normal[2]};
    const dd2_vehicle_vector force_arm =
        wheel->support == DD2_VEHICLE_WHEEL_BODY
            ? dd2_vehicle_add(wheel->point, dd2_vehicle_scale(vehicle->position, -1))
            : arm;
    const dd2_vehicle_vector point_velocity = dd2_vehicle_point_velocity(vehicle, force_arm);
    const dd2_vehicle_vector velocity =
        wheel->support == DD2_VEHICLE_WHEEL_BODY
            ? dd2_vehicle_add(point_velocity, dd2_vehicle_scale(surface_velocity, -1))
            : point_velocity;
    wheel->load = fmax(0, (wheel->compression * dd2_vehicle_spring) -
                              (dd2_vehicle_dot(velocity, normal) * dd2_vehicle_damper));
    const dd2_vehicle_vector force = dd2_vehicle_add(
        dd2_vehicle_scale(normal, wheel->load),
        dd2_vehicle_tire_force(vehicle, road, wheel, index, control, surface_velocity));
    if (wheel->support == DD2_VEHICLE_WHEEL_BODY) {
        dd2_vehicle_force *reaction = &support->reactions[wheel->body];
        reaction->linear = dd2_vehicle_add(reaction->linear, dd2_vehicle_scale(force, -1));
        const dd2_vehicle_vector other_arm = dd2_vehicle_add(
            wheel->point, dd2_vehicle_scale(support->field->boxes[wheel->body].center, -1));
        reaction->angular = dd2_vehicle_add(
            reaction->angular, dd2_vehicle_scale(dd2_vehicle_cross(other_arm, force), -1));
    }
    return force;
}

bool dd2_vehicle_refresh_wheels(dd2_vehicle *vehicle, const dd2_road *road,
                                const dd2_road_surface *surface) {
    if (road == NULL || surface == NULL || !dd2_vehicle_valid(vehicle)) {
        return false;
    }
    dd2_vehicle next = *vehicle;
    for (size_t index = 0; index < DD2_VEHICLE_WHEELS; ++index) {
        (void)dd2_vehicle_wheel_force(&next, road, surface, index, (dd2_vehicle_control){0}, NULL);
    }
    *vehicle = next;
    return true;
}

dd2_vehicle_vector dd2_vehicle_angular_response(dd2_vehicle_rotation rotation,
                                                dd2_vehicle_vector torque) {
    const dd2_vehicle_vector local = dd2_vehicle_rotate(dd2_vehicle_inverse(rotation), torque);
    return dd2_vehicle_rotate(rotation, (dd2_vehicle_vector){.x = local.x / dd2_vehicle_inertia.x,
                                                             .y = local.y / dd2_vehicle_inertia.y,
                                                             .z = local.z / dd2_vehicle_inertia.z});
}

static void dd2_vehicle_integrate_rotation(dd2_vehicle *vehicle, dd2_vehicle_vector torque) {
    const dd2_vehicle_rotation rotation = vehicle->rotation;
    const dd2_vehicle_rotation inverse = dd2_vehicle_inverse(rotation);
    const dd2_vehicle_vector local_torque = dd2_vehicle_rotate(inverse, torque);
    const dd2_vehicle_vector local_velocity =
        dd2_vehicle_rotate(inverse, vehicle->angular_velocity);
    const dd2_vehicle_vector momentum = {.x = local_velocity.x * dd2_vehicle_inertia.x,
                                         .y = local_velocity.y * dd2_vehicle_inertia.y,
                                         .z = local_velocity.z * dd2_vehicle_inertia.z};
    const dd2_vehicle_vector corrected = dd2_vehicle_add(
        local_torque, dd2_vehicle_scale(dd2_vehicle_cross(local_velocity, momentum), -1));
    const dd2_vehicle_vector acceleration = dd2_vehicle_rotate(
        rotation, (dd2_vehicle_vector){.x = corrected.x / dd2_vehicle_inertia.x,
                                       .y = corrected.y / dd2_vehicle_inertia.y,
                                       .z = corrected.z / dd2_vehicle_inertia.z});
    vehicle->angular_velocity = dd2_vehicle_scale(
        dd2_vehicle_add(vehicle->angular_velocity,
                        dd2_vehicle_scale(acceleration, DD2_VEHICLE_STEP_SECONDS)),
        1 / (1 + (dd2_vehicle_angular_drag * DD2_VEHICLE_STEP_SECONDS)));
    const dd2_vehicle_vector half =
        dd2_vehicle_scale(vehicle->angular_velocity, DD2_VEHICLE_STEP_SECONDS / 2);
    dd2_vehicle_rotation next = {
        .x = rotation.x + (half.x * rotation.w) + (half.y * rotation.z) - (half.z * rotation.y),
        .y = rotation.y + (half.y * rotation.w) + (half.z * rotation.x) - (half.x * rotation.z),
        .z = rotation.z + (half.z * rotation.w) + (half.x * rotation.y) - (half.y * rotation.x),
        .w = rotation.w - (half.x * rotation.x) - (half.y * rotation.y) - (half.z * rotation.z)};
    const double factor = 1 / sqrt(dd2_vehicle_rotation_length(next));
    next.x *= factor;
    next.y *= factor;
    next.z *= factor;
    next.w *= factor;
    vehicle->rotation = next;
}

static void dd2_vehicle_landing(dd2_vehicle *vehicle, const dd2_vehicle *previous,
                                const dd2_road_surface *surface) {
    const dd2_vehicle_vector up_axis =
        dd2_vehicle_rotate(vehicle->rotation, (dd2_vehicle_vector){.y = 1});
    if (up_axis.y < dd2_vehicle_min_up) {
        return;
    }
    double lift = 0;
    for (size_t index = 0; index < DD2_VEHICLE_WHEELS; ++index) {
        const dd2_vehicle_vector mount = dd2_vehicle_add(
            vehicle->position, dd2_vehicle_rotate(vehicle->rotation, dd2_vehicle_mount(index)));
        const dd2_vehicle_vector old_mount = dd2_vehicle_add(
            previous->position, dd2_vehicle_rotate(previous->rotation, dd2_vehicle_mount(index)));
        const dd2_surface_query query = {
            .point = {.x = mount.x, .z = mount.z},
            .min_height = mount.y - dd2_vehicle_radius - dd2_vehicle_travel,
            .max_height = fmax(old_mount.y, mount.y) - dd2_vehicle_radius + dd2_vehicle_bump_window,
            .preferred_cell = previous->wheels[index].contact.cell};
        dd2_road_contact contact = {0};
        if (dd2_road_surface_sample(surface, query, &contact, NULL) &&
            contact.normal[1] >= dd2_vehicle_min_up) {
            lift = fmax(lift, contact.height + dd2_vehicle_radius - mount.y);
        }
    }
    if (lift > 0) {
        /* Swept vertical window catches fast downward travel without selecting
         * a bridge above the previous wheel. Hard stop at full compression. */
        vehicle->position.y += lift;
        vehicle->velocity.y = fmax(0, vehicle->velocity.y);
    }
}

bool dd2_vehicle_step(dd2_vehicle *vehicle, const dd2_road *road, const dd2_road_surface *surface,
                      dd2_vehicle_control control) {
    if (vehicle == NULL || road == NULL || surface == NULL || !dd2_vehicle_valid(vehicle) ||
        vehicle->steps == UINT64_MAX || !dd2_vehicle_number_valid(&control.throttle, 1) ||
        !dd2_vehicle_number_valid(&control.brake, 1) || control.brake < 0 ||
        !dd2_vehicle_number_valid(&control.steer, 1)) {
        return false;
    }
    dd2_vehicle next = *vehicle;
    next.steering = dd2_vehicle_move_toward(next.steering, control.steer * dd2_vehicle_steer_limit,
                                            dd2_vehicle_steer_rate * DD2_VEHICLE_STEP_SECONDS);
    dd2_vehicle_vector acceleration = {.x = -next.velocity.x * dd2_vehicle_air_drag,
                                       .y = -dd2_vehicle_gravity,
                                       .z = -next.velocity.z * dd2_vehicle_air_drag};
    dd2_vehicle_vector torque = {0};
    for (size_t index = 0; index < DD2_VEHICLE_WHEELS; ++index) {
        const dd2_vehicle_vector force =
            dd2_vehicle_wheel_force(&next, road, surface, index, control, NULL);
        const dd2_vehicle_vector arm =
            dd2_vehicle_add(next.wheels[index].mount, dd2_vehicle_scale(next.position, -1));
        acceleration = dd2_vehicle_add(acceleration, force);
        torque = dd2_vehicle_add(torque, dd2_vehicle_cross(arm, force));
    }
    next.velocity =
        dd2_vehicle_add(next.velocity, dd2_vehicle_scale(acceleration, DD2_VEHICLE_STEP_SECONDS));
    next.position =
        dd2_vehicle_add(next.position, dd2_vehicle_scale(next.velocity, DD2_VEHICLE_STEP_SECONDS));
    dd2_vehicle_integrate_rotation(&next, torque);
    dd2_vehicle_landing(&next, vehicle, surface);
    ++next.steps;
    if (!dd2_vehicle_valid(&next)) {
        return false;
    }
    *vehicle = next;
    return true;
}

bool dd2_vehicle_step_field(dd2_vehicle *vehicles, const dd2_vehicle_field_step *step) {
    if (vehicles == NULL || step == NULL || step->road == NULL || step->surface == NULL ||
        step->controls == NULL || step->count == 0 || step->count > DD2_VEHICLE_FLEET_LIMIT) {
        return false;
    }
    dd2_body_surface field = {0};
    if (!dd2_body_surface_prepare(vehicles, step->count, &field)) {
        return false;
    }
    for (unsigned body = 0; body < step->count; ++body) {
        const dd2_vehicle_control *control = &step->controls[body];
        if (vehicles[body].steps == UINT64_MAX ||
            !dd2_vehicle_number_valid(&control->throttle, 1) ||
            !dd2_vehicle_number_valid(&control->brake, 1) || control->brake < 0 ||
            !dd2_vehicle_number_valid(&control->steer, 1)) {
            return false;
        }
    }
    dd2_vehicle next[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_vehicle_force forces[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_vehicle_force reactions[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned body = 0; body < step->count; ++body) {
        next[body] = vehicles[body];
        const dd2_vehicle_control control = step->controls[body];
        next[body].steering =
            dd2_vehicle_move_toward(next[body].steering, control.steer * dd2_vehicle_steer_limit,
                                    dd2_vehicle_steer_rate * DD2_VEHICLE_STEP_SECONDS);
        forces[body].linear =
            (dd2_vehicle_vector){.x = -next[body].velocity.x * dd2_vehicle_air_drag,
                                 .y = -dd2_vehicle_gravity,
                                 .z = -next[body].velocity.z * dd2_vehicle_air_drag};
        const dd2_vehicle_body_support support = {
            .field = &field, .reactions = reactions, .body = body};
        for (size_t wheel = 0; wheel < DD2_VEHICLE_WHEELS; ++wheel) {
            const dd2_vehicle_vector force = dd2_vehicle_wheel_force(
                &next[body], step->road, step->surface, wheel, control, &support);
            const dd2_vehicle_wheel *contact = &next[body].wheels[wheel];
            const dd2_vehicle_vector arm = dd2_vehicle_add(
                contact->support == DD2_VEHICLE_WHEEL_BODY ? contact->point : contact->mount,
                dd2_vehicle_scale(next[body].position, -1));
            forces[body].linear = dd2_vehicle_add(forces[body].linear, force);
            forces[body].angular =
                dd2_vehicle_add(forces[body].angular, dd2_vehicle_cross(arm, force));
        }
    }
    for (unsigned body = 0; body < step->count; ++body) {
        const dd2_vehicle_vector acceleration =
            dd2_vehicle_add(forces[body].linear, reactions[body].linear);
        const dd2_vehicle_vector torque =
            dd2_vehicle_add(forces[body].angular, reactions[body].angular);
        next[body].velocity = dd2_vehicle_add(
            next[body].velocity, dd2_vehicle_scale(acceleration, DD2_VEHICLE_STEP_SECONDS));
        next[body].position = dd2_vehicle_add(
            next[body].position, dd2_vehicle_scale(next[body].velocity, DD2_VEHICLE_STEP_SECONDS));
        dd2_vehicle_integrate_rotation(&next[body], torque);
        dd2_vehicle_landing(&next[body], &vehicles[body], step->surface);
        ++next[body].steps;
        if (!dd2_vehicle_valid(&next[body])) {
            return false;
        }
    }
    for (unsigned body = 0; body < step->count; ++body) {
        vehicles[body] = next[body];
    }
    return true;
}
