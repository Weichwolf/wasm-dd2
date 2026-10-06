#include "physics/car_contact.h"

#include "physics/collision_math.h"
#include "physics/vehicle.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>

/* Match Get_Corner_Positions and Check_2D_Car_Collision, retaining full pose. */
enum {
    DD2_CAR_AXES = 3,
    DD2_CAR_SAT_AXES = 15,
    DD2_CAR_POLYGON_POINTS = 16,
    DD2_CAR_ANGULAR_PIECES = 64
};
static const double dd2_car_half[DD2_CAR_AXES] = {186, 130, 450};
static const double dd2_car_radius = 504;
static const double dd2_car_piece_angle = 0.01;
static const double dd2_car_axis_tolerance = 1e-12;
static const double dd2_car_contact_tolerance = 1e-6;

typedef struct {
    dd2_vehicle_vector center;
    dd2_vehicle_vector axes[DD2_CAR_AXES];
    double padding;
} dd2_car_box;
typedef struct {
    double time;
    double depth;
    unsigned axis;
    dd2_vehicle_vector normal;
    bool found;
} dd2_car_interval;
typedef struct {
    dd2_vehicle_vector points[DD2_CAR_POLYGON_POINTS];
    unsigned count;
} dd2_car_polygon;

static double dd2_car_angle(dd2_vehicle_rotation first, dd2_vehicle_rotation last) {
    const double sign = dd2_collision_rotation_dot(first, last) < 0 ? -1 : 1;
    const dd2_vehicle_rotation difference = {.x = first.x - (sign * last.x),
                                             .y = first.y - (sign * last.y),
                                             .z = first.z - (sign * last.z),
                                             .w = first.w - (sign * last.w)};
    const dd2_vehicle_rotation sum = {.x = first.x + (sign * last.x),
                                      .y = first.y + (sign * last.y),
                                      .z = first.z + (sign * last.z),
                                      .w = first.w + (sign * last.w)};
    /* Unlike acos(dot), this remains well-conditioned for tiny rotations. */
    return 4 * atan2(sqrt(dd2_collision_rotation_dot(difference, difference)),
                     sqrt(dd2_collision_rotation_dot(sum, sum)));
}
static dd2_car_box dd2_car_box_at(const dd2_vehicle *start, const dd2_vehicle *end, double time) {
    const dd2_vehicle_rotation rotation =
        dd2_collision_rotation(start->rotation, end->rotation, time);
    return (dd2_car_box){.center = dd2_collision_position(start->position, end->position, time),
                         .axes = {dd2_vehicle_rotate(rotation, (dd2_vehicle_vector){.x = 1}),
                                  dd2_vehicle_rotate(rotation, (dd2_vehicle_vector){.y = 1}),
                                  dd2_vehicle_rotate(rotation, (dd2_vehicle_vector){.z = 1})}};
}
static double dd2_car_projection(const dd2_car_box *box, dd2_vehicle_vector axis) {
    double radius = box->padding;
    for (unsigned index = 0; index < DD2_CAR_AXES; ++index) {
        radius += dd2_car_half[index] * fabs(dd2_collision_dot(axis, box->axes[index]));
    }
    return radius;
}
static dd2_vehicle_vector dd2_car_axis(const dd2_car_box *first, const dd2_car_box *second,
                                       unsigned index) {
    if (index < DD2_CAR_AXES) {
        return first->axes[index];
    }
    if (index < DD2_CAR_AXES * 2) {
        return second->axes[index - DD2_CAR_AXES];
    }
    const unsigned cross = index - (DD2_CAR_AXES * 2);
    return dd2_collision_cross(first->axes[cross / DD2_CAR_AXES],
                               second->axes[cross % DD2_CAR_AXES]);
}

typedef struct {
    double enter;
    double leave;
    double depth;
    dd2_vehicle_vector overlap_normal;
    dd2_vehicle_vector entry_normal;
    bool valid;
    bool separated;
} dd2_car_axis_interval;
static const double dd2_car_time_tolerance = 1e-10;

static dd2_car_axis_interval dd2_car_axis_window(const dd2_car_box *first,
                                                 const dd2_car_box *second, unsigned index,
                                                 dd2_vehicle_vector begin,
                                                 dd2_vehicle_vector finish) {
    dd2_vehicle_vector axis = dd2_car_axis(first, second, index);
    const double squared = dd2_collision_dot(axis, axis);
    if (squared <= dd2_car_axis_tolerance * dd2_car_axis_tolerance) {
        return (dd2_car_axis_interval){0};
    }
    axis = dd2_collision_scale(axis, 1 / sqrt(squared));
    const double radius = dd2_car_projection(first, axis) + dd2_car_projection(second, axis);
    const double distance = dd2_collision_dot(begin, axis);
    const double velocity =
        dd2_collision_dot(dd2_collision_add(finish, dd2_collision_scale(begin, -1)), axis);
    const double depth = radius - fabs(distance);
    double sign = distance > 0 ? 1 : -1;
    if (distance == 0) {
        sign = velocity > 0 ? -1 : 1;
    }
    dd2_car_axis_interval interval = {.enter = -1,
                                      .leave = 2,
                                      .depth = depth,
                                      .overlap_normal = dd2_collision_scale(axis, sign),
                                      .valid = true};
    if (velocity == 0) {
        interval.separated = depth < 0;
        return interval;
    }
    const double first_time = (-radius - distance) / velocity;
    const double last_time = (radius - distance) / velocity;
    interval.enter = fmin(first_time, last_time);
    interval.leave = fmax(first_time, last_time);
    interval.entry_normal = dd2_collision_scale(axis, velocity > 0 ? -1 : 1);
    return interval;
}

static dd2_car_interval dd2_car_translation(const dd2_car_box *first, const dd2_car_box *second,
                                            dd2_vehicle_vector begin, dd2_vehicle_vector finish) {
    double enter = 0;
    double leave = 1;
    double overlap = dd2_car_radius * 4;
    dd2_car_axis_interval windows[DD2_CAR_SAT_AXES] = {0};
    for (unsigned index = 0; index < DD2_CAR_SAT_AXES; ++index) {
        windows[index] = dd2_car_axis_window(first, second, index, begin, finish);
        const dd2_car_axis_interval window = windows[index];
        if (window.separated) {
            return (dd2_car_interval){0};
        }
        if (!window.valid) {
            continue;
        }
        enter = fmax(enter, window.enter);
        leave = fmin(leave, window.leave);
        overlap = fmin(overlap, window.depth);
    }
    if (enter > leave || enter > 1 || leave < 0) {
        return (dd2_car_interval){0};
    }
    for (unsigned index = 0; index < DD2_CAR_SAT_AXES; ++index) {
        const dd2_car_axis_interval window = windows[index];
        if (!window.valid) {
            continue;
        }
        if (enter > 0 && window.enter >= enter - dd2_car_time_tolerance) {
            return (dd2_car_interval){
                .time = enter, .axis = index, .normal = window.entry_normal, .found = true};
        }
        if (enter == 0 && window.depth <= overlap + dd2_car_contact_tolerance) {
            const dd2_vehicle_vector motion =
                dd2_collision_add(finish, dd2_collision_scale(begin, -1));
            if (overlap <= dd2_car_contact_tolerance &&
                dd2_collision_dot(motion, window.overlap_normal) >= 0) {
                return (dd2_car_interval){0};
            }
            return (dd2_car_interval){.depth = fmax(0, overlap),
                                      .axis = index,
                                      .normal = window.overlap_normal,
                                      .found = true};
        }
    }
    return (dd2_car_interval){0};
}

static dd2_car_polygon dd2_car_clip(dd2_car_polygon polygon, dd2_vehicle_vector center,
                                    dd2_vehicle_vector axis, double extent) {
    dd2_car_polygon output = {0};
    if (polygon.count == 0) {
        return output;
    }
    dd2_vehicle_vector previous = polygon.points[polygon.count - 1];
    double previous_distance =
        dd2_collision_dot(dd2_collision_add(previous, dd2_collision_scale(center, -1)), axis) -
        extent;
    for (unsigned index = 0; index < polygon.count; ++index) {
        const dd2_vehicle_vector point = polygon.points[index];
        const double distance =
            dd2_collision_dot(dd2_collision_add(point, dd2_collision_scale(center, -1)), axis) -
            extent;
        if ((distance <= 0) != (previous_distance <= 0)) {
            const double fraction = previous_distance / (previous_distance - distance);
            if (output.count < DD2_CAR_POLYGON_POINTS) {
                output.points[output.count++] = dd2_collision_position(previous, point, fraction);
            }
        }
        if (distance <= 0 && output.count < DD2_CAR_POLYGON_POINTS) {
            output.points[output.count++] = point;
        }
        previous = point;
        previous_distance = distance;
    }
    return output;
}

static dd2_vehicle_vector dd2_car_centroid(const dd2_car_polygon *polygon,
                                           dd2_vehicle_vector normal) {
    dd2_vehicle_vector sum = {0};
    double area = 0;
    const dd2_vehicle_vector origin = polygon->points[0];
    for (unsigned index = 1; index + 1 < polygon->count; ++index) {
        const dd2_vehicle_vector first = polygon->points[index];
        const dd2_vehicle_vector second = polygon->points[index + 1];
        const double weight = fabs(dd2_collision_dot(
            dd2_collision_cross(dd2_collision_add(first, dd2_collision_scale(origin, -1)),
                                dd2_collision_add(second, dd2_collision_scale(origin, -1))),
            normal));
        sum = dd2_collision_add(
            sum, dd2_collision_scale(dd2_collision_add(origin, dd2_collision_add(first, second)),
                                     weight / 3));
        area += weight;
    }
    if (area > dd2_car_axis_tolerance) {
        return dd2_collision_scale(sum, 1 / area);
    }
    sum = (dd2_vehicle_vector){0};
    for (unsigned index = 0; index < polygon->count; ++index) {
        sum = dd2_collision_add(sum, polygon->points[index]);
    }
    return dd2_collision_scale(sum, 1 / (double)polygon->count);
}

static dd2_vehicle_vector dd2_car_face_contact(const dd2_car_box *reference,
                                               const dd2_car_box *incident, unsigned face,
                                               dd2_vehicle_vector outward) {
    const dd2_vehicle_vector center =
        dd2_collision_add(reference->center, dd2_collision_scale(outward, dd2_car_half[face]));
    unsigned incident_axis = 0;
    double largest = 0;
    for (unsigned axis = 0; axis < DD2_CAR_AXES; ++axis) {
        const double alignment = fabs(dd2_collision_dot(outward, incident->axes[axis]));
        if (alignment > largest) {
            largest = alignment;
            incident_axis = axis;
        }
    }
    const double sign = dd2_collision_dot(outward, incident->axes[incident_axis]) > 0 ? -1 : 1;
    const dd2_vehicle_vector incident_center = dd2_collision_add(
        incident->center,
        dd2_collision_scale(incident->axes[incident_axis], sign * dd2_car_half[incident_axis]));
    const unsigned along = (incident_axis + 1) % DD2_CAR_AXES;
    const unsigned across = (incident_axis + 2) % DD2_CAR_AXES;
    const double signs[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
    dd2_car_polygon polygon = {.count = 4};
    for (unsigned corner = 0; corner < 4; ++corner) {
        polygon.points[corner] = dd2_collision_add(
            incident_center,
            dd2_collision_add(
                dd2_collision_scale(incident->axes[along], signs[corner][0] * dd2_car_half[along]),
                dd2_collision_scale(incident->axes[across],
                                    signs[corner][1] * dd2_car_half[across])));
    }
    for (unsigned axis = 0; axis < DD2_CAR_AXES; ++axis) {
        if (axis == face) {
            continue;
        }
        polygon = dd2_car_clip(polygon, center, reference->axes[axis], dd2_car_half[axis]);
        polygon = dd2_car_clip(polygon, center, dd2_collision_scale(reference->axes[axis], -1),
                               dd2_car_half[axis]);
    }
    if (polygon.count == 0) {
        return dd2_collision_scale(dd2_collision_add(center, incident_center), 1.0 / 2);
    }
    /* Area weighting makes collinear/duplicate clipping vertices harmless;
     * averaging vertex counts would move the impulse point at topology changes. */
    const dd2_vehicle_vector point = dd2_car_centroid(&polygon, outward);
    const double distance =
        dd2_collision_dot(dd2_collision_add(point, dd2_collision_scale(center, -1)), outward);
    return dd2_collision_add(point, dd2_collision_scale(outward, -distance / 2));
}

static dd2_vehicle_vector dd2_car_edge_center(const dd2_car_box *box, unsigned along,
                                              dd2_vehicle_vector toward) {
    dd2_vehicle_vector center = box->center;
    for (unsigned axis = 0; axis < DD2_CAR_AXES; ++axis) {
        if (axis == along) {
            continue;
        }
        const double sign = dd2_collision_dot(toward, box->axes[axis]) >= 0 ? 1 : -1;
        center = dd2_collision_add(center,
                                   dd2_collision_scale(box->axes[axis], sign * dd2_car_half[axis]));
    }
    return center;
}

static dd2_vehicle_vector dd2_car_edge_contact(const dd2_car_box *first, const dd2_car_box *second,
                                               unsigned first_axis, unsigned second_axis,
                                               dd2_vehicle_vector normal) {
    const dd2_vehicle_vector first_center =
        dd2_car_edge_center(first, first_axis, dd2_collision_scale(normal, -1));
    const dd2_vehicle_vector second_center = dd2_car_edge_center(second, second_axis, normal);
    const dd2_vehicle_vector relative =
        dd2_collision_add(first_center, dd2_collision_scale(second_center, -1));
    const double cosine = dd2_collision_dot(first->axes[first_axis], second->axes[second_axis]);
    const double first_distance = dd2_collision_dot(relative, first->axes[first_axis]);
    const double second_distance = dd2_collision_dot(relative, second->axes[second_axis]);
    const double determinant = 1 - (cosine * cosine);
    double along = determinant > dd2_car_axis_tolerance
                       ? ((cosine * second_distance) - first_distance) / determinant
                       : 0;
    along = fmax(-dd2_car_half[first_axis], fmin(dd2_car_half[first_axis], along));
    double across = second_distance + (cosine * along);
    across = fmax(-dd2_car_half[second_axis], fmin(dd2_car_half[second_axis], across));
    along = fmax(-dd2_car_half[first_axis],
                 fmin(dd2_car_half[first_axis], (cosine * across) - first_distance));
    return dd2_collision_scale(
        dd2_collision_add(
            dd2_collision_add(first_center, dd2_collision_scale(first->axes[first_axis], along)),
            dd2_collision_add(second_center,
                              dd2_collision_scale(second->axes[second_axis], across))),
        1.0 / 2);
}

static dd2_vehicle_vector dd2_car_point(const dd2_car_box *first, const dd2_car_box *second,
                                        dd2_car_interval hit) {
    if (hit.axis < DD2_CAR_AXES) {
        return dd2_car_face_contact(first, second, hit.axis, dd2_collision_scale(hit.normal, -1));
    }
    if (hit.axis < DD2_CAR_AXES * 2) {
        return dd2_car_face_contact(second, first, hit.axis - DD2_CAR_AXES, hit.normal);
    }
    const unsigned cross = hit.axis - (DD2_CAR_AXES * 2);
    return dd2_car_edge_contact(first, second, cross / DD2_CAR_AXES, cross % DD2_CAR_AXES,
                                hit.normal);
}

bool dd2_car_contact_sweep(const dd2_vehicle *first_start, const dd2_vehicle *first_end,
                           const dd2_vehicle *second_start, const dd2_vehicle *second_end,
                           dd2_car_contact *contact) {
    if (contact == NULL) {
        return false;
    }
    *contact = (dd2_car_contact){0};
    if (!dd2_vehicle_valid(first_start) || !dd2_vehicle_valid(first_end) ||
        !dd2_vehicle_valid(second_start) || !dd2_vehicle_valid(second_end)) {
        return false;
    }
    const dd2_vehicle_vector relative =
        dd2_collision_add(first_start->position, dd2_collision_scale(second_start->position, -1));
    const dd2_vehicle_vector finish =
        dd2_collision_add(first_end->position, dd2_collision_scale(second_end->position, -1));
    const dd2_vehicle_vector motion = dd2_collision_add(finish, dd2_collision_scale(relative, -1));
    const double squared = dd2_collision_dot(motion, motion);
    const double time =
        squared > 0 ? fmax(0, fmin(1, -dd2_collision_dot(relative, motion) / squared)) : 0;
    const dd2_vehicle_vector closest =
        dd2_collision_add(relative, dd2_collision_scale(motion, time));
    if (dd2_collision_dot(closest, closest) > 4 * dd2_car_radius * dd2_car_radius) {
        return false;
    }
    const double angle = fmax(dd2_car_angle(first_start->rotation, first_end->rotation),
                              dd2_car_angle(second_start->rotation, second_end->rotation));
    const unsigned pieces =
        (unsigned)fmin((double)DD2_CAR_ANGULAR_PIECES, fmax(1, ceil(angle / dd2_car_piece_angle)));
    for (unsigned piece = 0; piece < pieces; ++piece) {
        const double begin = (double)piece / (double)pieces;
        const double end = (double)(piece + 1) / (double)pieces;
        const double middle = (begin + end) / 2;
        dd2_car_box first = dd2_car_box_at(first_start, first_end, middle);
        dd2_car_box second = dd2_car_box_at(second_start, second_end, middle);
        const dd2_vehicle_rotation first_begin =
            dd2_collision_rotation(first_start->rotation, first_end->rotation, begin);
        const dd2_vehicle_rotation first_finish =
            dd2_collision_rotation(first_start->rotation, first_end->rotation, end);
        const dd2_vehicle_rotation second_begin =
            dd2_collision_rotation(second_start->rotation, second_end->rotation, begin);
        const dd2_vehicle_rotation second_finish =
            dd2_collision_rotation(second_start->rotation, second_end->rotation, end);
        /* Every corner stays within this ball around its midpoint orientation.
         * Envelopes add a small, explicit angular contact skin, not frame samples. */
        const dd2_vehicle_rotation first_middle =
            dd2_collision_rotation(first_start->rotation, first_end->rotation, middle);
        const dd2_vehicle_rotation second_middle =
            dd2_collision_rotation(second_start->rotation, second_end->rotation, middle);
        first.padding = 2 * dd2_car_radius *
                        sin(fmax(dd2_car_angle(first_begin, first_middle),
                                 dd2_car_angle(first_middle, first_finish)) /
                            2);
        second.padding = 2 * dd2_car_radius *
                         sin(fmax(dd2_car_angle(second_begin, second_middle),
                                  dd2_car_angle(second_middle, second_finish)) /
                             2);
        const dd2_car_interval hit =
            dd2_car_translation(&first, &second, dd2_collision_position(relative, finish, begin),
                                dd2_collision_position(relative, finish, end));
        if (!hit.found) {
            continue;
        }
        const double found_time = begin + (hit.time / (double)pieces);
        first.center =
            dd2_collision_position(first_start->position, first_end->position, found_time);
        second.center =
            dd2_collision_position(second_start->position, second_end->position, found_time);
        *contact = (dd2_car_contact){.time = found_time,
                                     .penetration = hit.depth,
                                     .normal = hit.normal,
                                     .point = dd2_car_point(&first, &second, hit)};
        return true;
    }
    return false;
}
