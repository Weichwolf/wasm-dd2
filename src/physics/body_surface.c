#include "physics/body_surface.h"

#include "physics/body_geometry.h"
#include "physics/collision_math.h"
#include "physics/numeric.h"
#include "physics/road_contact.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stddef.h>

static const double dd2_body_surface_min_up = 0.2;
static const double dd2_body_surface_radius = 504;
static const double dd2_body_surface_axis_tolerance = 1e-10;
static const double dd2_body_surface_unit_tolerance = 1e-6;

bool dd2_body_surface_prepare(const dd2_vehicle *vehicles, unsigned count,
                              dd2_body_surface *field) {
    if (field == NULL) {
        return false;
    }
    *field = (dd2_body_surface){0};
    if (vehicles == NULL || count == 0 || count > DD2_VEHICLE_FLEET_LIMIT) {
        return false;
    }
    for (unsigned body = 0; body < count; ++body) {
        if (!dd2_vehicle_valid(&vehicles[body])) {
            *field = (dd2_body_surface){0};
            return false;
        }
        const dd2_vehicle *vehicle = &vehicles[body];
        const dd2_vehicle_rotation rotation = dd2_collision_normalize(vehicle->rotation);
        field->boxes[body] = (dd2_body_surface_box){
            .center = vehicle->position,
            .axes = {dd2_vehicle_rotate(rotation, (dd2_vehicle_vector){.x = 1}),
                     dd2_vehicle_rotate(rotation, (dd2_vehicle_vector){.y = 1}),
                     dd2_vehicle_rotate(rotation, (dd2_vehicle_vector){.z = 1})},
            .velocity = vehicle->velocity,
            .angular_velocity = vehicle->angular_velocity};
    }
    field->count = count;
    return true;
}

static unsigned dd2_body_surface_box_sample(const dd2_body_surface_box *box,
                                            const dd2_surface_query *query,
                                            dd2_body_surface_contact *faces) {
    const double half[] = {DD2_BODY_HALF_WIDTH, DD2_BODY_HALF_HEIGHT, DD2_BODY_HALF_LENGTH};
    const double xpos = query->point.x - box->center.x;
    const double zpos = query->point.z - box->center.z;
    if ((xpos * xpos) + (zpos * zpos) > dd2_body_surface_radius * dd2_body_surface_radius ||
        query->min_height > box->center.y + dd2_body_surface_radius ||
        query->max_height < box->center.y - dd2_body_surface_radius) {
        return 0;
    }
    unsigned count = 0;
    for (unsigned axis = 0; axis < DD2_BODY_SURFACE_AXES; ++axis) {
        const double sign = box->axes[axis].y < 0 ? -1 : 1;
        const dd2_vehicle_vector normal = dd2_collision_scale(box->axes[axis], sign);
        if (normal.y < dd2_body_surface_min_up) {
            continue;
        }
        const double height =
            box->center.y + ((half[axis] - (normal.x * xpos) - (normal.z * zpos)) / normal.y);
        if (height < query->min_height || height > query->max_height) {
            continue;
        }
        const dd2_vehicle_vector arm = {.x = xpos, .y = height - box->center.y, .z = zpos};
        bool inside = true;
        for (unsigned side = 0; side < DD2_BODY_SURFACE_AXES; ++side) {
            if (side != axis && fabs(dd2_collision_dot(arm, box->axes[side])) >
                                    half[side] + DD2_ROAD_EDGE_TOLERANCE) {
                inside = false;
            }
        }
        if (!inside) {
            continue;
        }
        faces[count] = (dd2_body_surface_contact){
            .point = {.x = query->point.x, .y = height, .z = query->point.z},
            .normal = normal,
            .velocity =
                dd2_collision_add(box->velocity, dd2_collision_cross(box->angular_velocity, arm))};
        ++count;
    }
    return count;
}

bool dd2_body_surface_sample(const dd2_body_surface *field, const dd2_surface_query *query,
                             unsigned exclude, dd2_body_surface_contact *contact) {
    if (contact == NULL) {
        return false;
    }
    *contact = (dd2_body_surface_contact){0};
    if (field == NULL || field->count == 0 || field->count > DD2_VEHICLE_FLEET_LIMIT ||
        exclude >= field->count || query == NULL || !dd2_numeric_finite(&query->point.x) ||
        !dd2_numeric_finite(&query->point.z) || !dd2_numeric_finite(&query->min_height) ||
        !dd2_numeric_finite(&query->max_height) || query->min_height > query->max_height) {
        return false;
    }
    dd2_body_surface_contact candidates[DD2_VEHICLE_FLEET_LIMIT * DD2_BODY_SURFACE_AXES];
    unsigned count = 0;
    double highest = 0;
    for (unsigned body = 0; body < field->count; ++body) {
        if (body == exclude) {
            continue;
        }
        const unsigned faces =
            dd2_body_surface_box_sample(&field->boxes[body], query, &candidates[count]);
        for (unsigned face = 0; face < faces; ++face) {
            candidates[count].body = body;
            highest =
                count == 0 ? candidates[count].point.y : fmax(highest, candidates[count].point.y);
            ++count;
        }
    }
    for (unsigned candidate = 0; candidate < count; ++candidate) {
        if (candidates[candidate].point.y >= highest - DD2_ROAD_EDGE_TOLERANCE) {
            *contact = candidates[candidate];
            contact->maximum_height = highest;
            return true;
        }
    }
    return false;
}

static bool dd2_body_surface_wheel_valid(const dd2_body_wheel_query *query) {
    if (query == NULL || !dd2_numeric_finite(&query->mount.x) ||
        !dd2_numeric_finite(&query->mount.y) || !dd2_numeric_finite(&query->mount.z) ||
        !dd2_numeric_finite(&query->axis.x) || !dd2_numeric_finite(&query->axis.y) ||
        !dd2_numeric_finite(&query->axis.z) || !dd2_numeric_finite(&query->radius) ||
        !dd2_numeric_finite(&query->min_displacement) ||
        !dd2_numeric_finite(&query->max_displacement) || query->radius <= 0 ||
        query->min_displacement > query->max_displacement ||
        fabs(query->axis.x) > 1 + dd2_body_surface_unit_tolerance ||
        fabs(query->axis.y) > 1 + dd2_body_surface_unit_tolerance ||
        fabs(query->axis.z) > 1 + dd2_body_surface_unit_tolerance) {
        return false;
    }
    return fabs(dd2_collision_dot(query->axis, query->axis) - 1) <= dd2_body_surface_unit_tolerance;
}

static unsigned dd2_body_surface_wheel_faces(const dd2_body_surface_box *box,
                                             const dd2_body_wheel_query *query, double reach,
                                             dd2_body_surface_contact *faces) {
    const double half[] = {DD2_BODY_HALF_WIDTH, DD2_BODY_HALF_HEIGHT, DD2_BODY_HALF_LENGTH};
    const dd2_vehicle_vector mount_arm =
        dd2_collision_add(query->mount, dd2_collision_scale(box->center, -1));
    if (fabs(mount_arm.x) > reach || fabs(mount_arm.y) > reach || fabs(mount_arm.z) > reach) {
        return 0;
    }
    unsigned count = 0;
    for (unsigned axis = 0; axis < DD2_BODY_SURFACE_AXES; ++axis) {
        const double sign = box->axes[axis].y < 0 ? -1 : 1;
        const dd2_vehicle_vector normal = dd2_collision_scale(box->axes[axis], sign);
        const double alignment = dd2_collision_dot(normal, query->axis);
        if (normal.y < dd2_body_surface_min_up || alignment <= dd2_body_surface_axis_tolerance) {
            continue;
        }
        const double numerator = dd2_collision_dot(normal, mount_arm) - half[axis] - query->radius;
        const double displacement = numerator / alignment;
        if (!dd2_numeric_finite(&displacement) || displacement < query->min_displacement ||
            displacement > query->max_displacement) {
            continue;
        }
        const dd2_vehicle_vector center_arm =
            dd2_collision_add(mount_arm, dd2_collision_scale(query->axis, -fmax(0, displacement)));
        const dd2_vehicle_vector arm = dd2_collision_add(
            center_arm,
            dd2_collision_scale(normal, half[axis] - dd2_collision_dot(normal, center_arm)));
        bool inside =
            dd2_numeric_finite(&arm.x) && dd2_numeric_finite(&arm.y) && dd2_numeric_finite(&arm.z);
        for (unsigned side = 0; side < DD2_BODY_SURFACE_AXES; ++side) {
            inside = inside && fabs(dd2_collision_dot(arm, box->axes[side])) <=
                                   half[side] + DD2_ROAD_EDGE_TOLERANCE;
        }
        if (!inside) {
            continue;
        }
        faces[count] = (dd2_body_surface_contact){
            .point = dd2_collision_add(box->center, arm),
            .normal = normal,
            .velocity =
                dd2_collision_add(box->velocity, dd2_collision_cross(box->angular_velocity, arm)),
            .displacement = displacement};
        ++count;
    }
    return count;
}

bool dd2_body_surface_wheel_sample(const dd2_body_surface *field, const dd2_body_wheel_query *query,
                                   unsigned exclude, dd2_body_surface_contact *contact) {
    if (contact == NULL) {
        return false;
    }
    *contact = (dd2_body_surface_contact){0};
    if (field == NULL || field->count == 0 || field->count > DD2_VEHICLE_FLEET_LIMIT ||
        exclude >= field->count || !dd2_body_surface_wheel_valid(query)) {
        return false;
    }
    const double reach = dd2_body_surface_radius + query->radius +
                         fmax(fabs(query->min_displacement), fabs(query->max_displacement));
    if (!dd2_numeric_finite(&reach)) {
        return false;
    }
    dd2_body_surface_contact candidates[DD2_VEHICLE_FLEET_LIMIT * DD2_BODY_SURFACE_AXES];
    unsigned count = 0;
    double closest = 0;
    for (unsigned body = 0; body < field->count; ++body) {
        if (body == exclude) {
            continue;
        }
        const unsigned faces =
            dd2_body_surface_wheel_faces(&field->boxes[body], query, reach, &candidates[count]);
        for (unsigned face = 0; face < faces; ++face) {
            candidates[count].body = body;
            closest = count == 0 ? candidates[count].displacement
                                 : fmin(closest, candidates[count].displacement);
            ++count;
        }
    }
    for (unsigned candidate = 0; candidate < count; ++candidate) {
        if (candidates[candidate].displacement <= closest + DD2_ROAD_EDGE_TOLERANCE) {
            *contact = candidates[candidate];
            contact->minimum_displacement = closest;
            return true;
        }
    }
    return false;
}
