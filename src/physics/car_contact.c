#include "physics/car_contact.h"

#include "physics/collision_math.h"
#include "physics/vehicle.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef DD2_CAR_QUERY_WINDOWS
#define DD2_CAR_QUERY_WINDOWS 512
#endif

/* Match Get_Corner_Positions and Check_2D_Car_Collision, retaining full pose. */
enum {
    DD2_CAR_AXES = 3,
    DD2_CAR_SAT_AXES = 15,
    DD2_CAR_POLYGON_POINTS = 16,
    DD2_CAR_ANGULAR_PIECES = 64,
    DD2_CAR_REFINEMENT_LIMIT = DD2_CAR_QUERY_WINDOWS,
    DD2_CAR_REFINEMENT_DEPTH = 48
};
static const double dd2_car_half[DD2_CAR_AXES] = {186, 130, 450};
static const double dd2_car_radius = 504;
static const double dd2_car_piece_angle = 0.01;
static const double dd2_car_axis_tolerance = 1e-12;
static const double dd2_car_contact_tolerance = 1e-6;

typedef struct {
    dd2_vehicle_vector center;
    dd2_vehicle_vector axes[DD2_CAR_AXES];
} dd2_car_box;
typedef struct {
    double time;
    double leave;
    double depth;
    dd2_vehicle_vector normal;
    unsigned axis;
    bool found;
    bool unresolved;
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
    double radius = 0;
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

typedef struct {
    dd2_car_box begin;
    dd2_car_box end;
    dd2_car_box middle;
    double speed;
    double acceleration;
    double span;
} dd2_car_window;
typedef struct {
    const dd2_vehicle *start;
    const dd2_vehicle *end;
    double speed;
    double acceleration;
} dd2_car_motion;
typedef struct {
    dd2_car_motion first;
    dd2_car_motion second;
    unsigned remaining;
} dd2_car_sweep;
typedef struct {
    double speed;
    double acceleration;
} dd2_car_axis_motion;
static dd2_car_axis_motion dd2_car_axis_motion_bound(const dd2_car_window *first,
                                                     const dd2_car_window *second, unsigned index) {
    if (index < DD2_CAR_AXES) {
        return (dd2_car_axis_motion){.speed = first->speed, .acceleration = first->acceleration};
    }
    if (index < DD2_CAR_AXES * 2) {
        return (dd2_car_axis_motion){.speed = second->speed, .acceleration = second->acceleration};
    }
    return (dd2_car_axis_motion){.speed = first->speed + second->speed,
                                 .acceleration = first->acceleration + second->acceleration +
                                                 (2 * first->speed * second->speed)};
}
static dd2_car_axis_interval dd2_car_axis_window(const dd2_car_window *first,
                                                 const dd2_car_window *second, unsigned index,
                                                 dd2_vehicle_vector begin,
                                                 dd2_vehicle_vector finish) {
    dd2_vehicle_vector axis = dd2_car_axis(&first->middle, &second->middle, index);
    const double squared = dd2_collision_dot(axis, axis);
    if (squared <= dd2_car_axis_tolerance * dd2_car_axis_tolerance) {
        return (dd2_car_axis_interval){0};
    }
    const double factor = 1 / sqrt(squared);
    const dd2_vehicle_vector first_axis =
        dd2_collision_scale(dd2_car_axis(&first->begin, &second->begin, index), factor);
    const dd2_vehicle_vector last_axis =
        dd2_collision_scale(dd2_car_axis(&first->end, &second->end, index), factor);
    const dd2_car_axis_motion motion = dd2_car_axis_motion_bound(first, second, index);
    const double speed = motion.speed;
    const double travel_squared =
        dd2_collision_dot(dd2_collision_add(finish, dd2_collision_scale(begin, -1)),
                          dd2_collision_add(finish, dd2_collision_scale(begin, -1)));
    const double distance_max =
        fmax(sqrt(dd2_collision_dot(begin, begin)), sqrt(dd2_collision_dot(finish, finish)));
    const double acceleration = motion.acceleration;
    const double half_sum = dd2_car_half[0] + dd2_car_half[1] + dd2_car_half[2];
    const double support_bound =
        half_sum * (first->acceleration + second->acceleration +
                    2 * (first->speed + second->speed) * speed + 2 * acceleration);
    const double span = first->span;
    /* Endpoint support is an upper chord except for curvature of the rotating
     * axes. |f''| * span^2/8 bounds that curvature for separation and every
     * signed support term, including changing cross-product SAT axes. */
    const double padding = (2 * sqrt(travel_squared) * speed * span +
                            (distance_max * acceleration + support_bound) * span * span) *
                           factor / 8;
    const double radius = dd2_car_projection(&first->begin, first_axis) +
                          dd2_car_projection(&second->begin, first_axis) + padding;
    const double last_radius = dd2_car_projection(&first->end, last_axis) +
                               dd2_car_projection(&second->end, last_axis) + padding;
    const double distance = dd2_collision_dot(first_axis, begin);
    const double velocity = dd2_collision_dot(finish, last_axis) - distance;
    const double radius_change = last_radius - radius;
    axis = dd2_collision_scale(axis, 1 / sqrt(squared));
    const double minimum_axis = fmax(0, fmin(sqrt(dd2_collision_dot(first_axis, first_axis)),
                                             sqrt(dd2_collision_dot(last_axis, last_axis))) -
                                            (speed * span * factor / 2));
    const double tolerance = dd2_car_contact_tolerance * minimum_axis;
    if (fabs(distance) - radius >= -tolerance &&
        (distance > 0 ? 1 : -1) * (distance + velocity) - last_radius >= -tolerance &&
        fabs(distance) <= radius + tolerance) {
        return (dd2_car_axis_interval){.separated = true};
    }
    double sign = distance > 0 ? 1 : -1;
    if (distance == 0) {
        sign = velocity > 0 ? -1 : 1;
    }
    dd2_car_axis_interval interval = {.enter = -1,
                                      .leave = 2,
                                      .depth = radius - fabs(distance),
                                      .overlap_normal = dd2_collision_scale(axis, sign),
                                      .valid = true};
    for (unsigned side = 0; side < 2; ++side) {
        const double direction = side == 0 ? 1 : -1;
        const double gap = (direction * distance) - radius;
        const double change = (direction * velocity) - radius_change;
        if (fabs(change) <= dd2_car_axis_tolerance) {
            if (gap > 0) {
                interval.separated = true;
            }
        } else if (change < 0) {
            const double time = -gap / change;
            if (time > interval.enter) {
                interval.enter = time;
                interval.entry_normal = dd2_collision_scale(axis, direction);
            }
        } else {
            interval.leave = fmin(interval.leave, -gap / change);
        }
    }
    return interval;
}
static dd2_car_interval dd2_car_translation(const dd2_car_window *first,
                                            const dd2_car_window *second, dd2_vehicle_vector begin,
                                            dd2_vehicle_vector finish) {
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
            return (dd2_car_interval){.time = enter,
                                      .leave = leave,
                                      .axis = index,
                                      .normal = window.entry_normal,
                                      .found = true};
        }
        if (enter == 0 && window.depth <= overlap + dd2_car_contact_tolerance) {
            const dd2_vehicle_vector axis = window.overlap_normal;
            return (dd2_car_interval){.depth = fmax(0, overlap),
                                      .leave = leave,
                                      .axis = index,
                                      .normal = axis,
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
    polygon = dd2_car_clip(polygon, center, outward, dd2_car_contact_tolerance);
    if (polygon.count == 0) {
        return dd2_collision_scale(dd2_collision_add(center, incident_center), 1.0 / 2);
    }
    /* Tilted faces meet at their deepest edge or corner. Using the whole
     * incident face would move the impulse to a separating part of that face.
     * Parallel faces retain the area centroid and its clipping continuity. */
    double deepest = dd2_car_radius * 4;
    for (unsigned index = 0; index < polygon.count; ++index) {
        const double value = dd2_collision_dot(
            dd2_collision_add(polygon.points[index], dd2_collision_scale(center, -1)), outward);
        deepest = fmin(deepest, value);
    }
    dd2_car_polygon nearest = {0};
    for (unsigned index = 0; index < polygon.count; ++index) {
        const double value = dd2_collision_dot(
            dd2_collision_add(polygon.points[index], dd2_collision_scale(center, -1)), outward);
        if (value <= deepest + dd2_car_contact_tolerance) {
            nearest.points[nearest.count++] = polygon.points[index];
        }
    }
    const dd2_vehicle_vector point = dd2_car_centroid(&nearest, outward);
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

static dd2_car_interval dd2_car_static(const dd2_car_box *first, const dd2_car_box *second) {
    dd2_car_interval axes[DD2_CAR_SAT_AXES] = {0};
    double minimum = 4 * dd2_car_radius;
    const dd2_vehicle_vector relative =
        dd2_collision_add(first->center, dd2_collision_scale(second->center, -1));
    for (unsigned index = 0; index < DD2_CAR_SAT_AXES; ++index) {
        dd2_vehicle_vector axis = dd2_car_axis(first, second, index);
        const double squared = dd2_collision_dot(axis, axis);
        if (squared <= dd2_car_axis_tolerance * dd2_car_axis_tolerance) {
            continue;
        }
        axis = dd2_collision_scale(axis, 1 / sqrt(squared));
        const double distance = dd2_collision_dot(relative, axis);
        const double depth =
            dd2_car_projection(first, axis) + dd2_car_projection(second, axis) - fabs(distance);
        if (depth < -dd2_car_contact_tolerance) {
            return (dd2_car_interval){.depth = depth};
        }
        minimum = fmin(minimum, depth);
        axes[index] = (dd2_car_interval){.depth = depth,
                                         .axis = index,
                                         .normal = dd2_collision_scale(axis, distance > 0 ? 1 : -1),
                                         .found = true};
    }
    /* Anchor ties to the actual minimum, then use source-axis order. Roundoff
     * must not switch a face contact to a distant supported edge point. */
    for (unsigned index = 0; index < DD2_CAR_SAT_AXES; ++index) {
        if (axes[index].found && axes[index].depth <= minimum + dd2_car_contact_tolerance) {
            dd2_car_interval hit = axes[index];
            hit.depth = minimum;
            return hit;
        }
    }
    return (dd2_car_interval){0};
}
static dd2_car_motion dd2_car_motion_create(const dd2_vehicle *start, const dd2_vehicle *end) {
    const dd2_vehicle_rotation first = start->rotation;
    const double sign = dd2_collision_rotation_dot(first, end->rotation) < 0 ? -1 : 1;
    const dd2_vehicle_rotation delta = {.x = (sign * end->rotation.x) - first.x,
                                        .y = (sign * end->rotation.y) - first.y,
                                        .z = (sign * end->rotation.z) - first.z,
                                        .w = (sign * end->rotation.w) - first.w};
    const double squared = dd2_collision_rotation_dot(delta, delta);
    dd2_car_motion motion = {.start = start, .end = end};
    if (squared == 0) {
        return motion;
    }
    const double projection = dd2_collision_rotation_dot(first, delta);
    const double first_squared = dd2_collision_rotation_dot(first, first);
    const double parallel = projection / first_squared;
    const dd2_vehicle_rotation perpendicular = {.x = delta.x - (parallel * first.x),
                                                .y = delta.y - (parallel * first.y),
                                                .z = delta.z - (parallel * first.z),
                                                .w = delta.w - (parallel * first.w)};
    const double closest = fmax(0, fmin(1, -projection / squared));
    const dd2_vehicle_rotation minimum = {.x = first.x + (delta.x * closest),
                                          .y = first.y + (delta.y * closest),
                                          .z = first.z + (delta.z * closest),
                                          .w = first.w + (delta.w * closest)};
    const double minimum_squared = dd2_collision_rotation_dot(minimum, minimum);
    const double area =
        sqrt(first_squared * dd2_collision_rotation_dot(perpendicular, perpendicular));
    /* For normalized linear quaternion interpolation, angular speed is
     * 2*area/|q|^2. Differentiating it also bounds angular acceleration.
     * This includes the small endpoint norm error accepted by vehicle_valid. */
    motion.speed = 2 * area / minimum_squared;
    const double angular_acceleration =
        2 * motion.speed * fmax(fabs(projection), fabs(projection + squared)) / minimum_squared;
    motion.acceleration = motion.speed * motion.speed + angular_acceleration;
    return motion;
}
static dd2_car_window dd2_car_window_create(dd2_car_motion motion, double begin, double end) {
    return (dd2_car_window){.begin = dd2_car_box_at(motion.start, motion.end, begin),
                            .end = dd2_car_box_at(motion.start, motion.end, end),
                            .middle = dd2_car_box_at(motion.start, motion.end, (begin + end) / 2),
                            .speed = motion.speed,
                            .acceleration = motion.acceleration,
                            .span = end - begin};
}
static bool dd2_car_unresolved(dd2_car_interval *hit, double *time, double begin) {
    *hit = (dd2_car_interval){.found = true, .unresolved = true};
    *time = begin;
    return true;
}
typedef struct {
    double begin;
    double end;
    unsigned depth;
} dd2_car_refinement;
static bool dd2_car_refine(dd2_car_sweep *sweep, double begin, double end, dd2_car_interval *hit,
                           double *time) {
    /* Earliest-first depth search keeps at most one pending right half per
     * depth. The explicit stack also bounds native/WASM stack consumption. */
    dd2_car_refinement pending[DD2_CAR_REFINEMENT_DEPTH + 2] = {{.begin = begin, .end = end}};
    unsigned count = 1;
    while (count > 0) {
        const dd2_car_refinement current = pending[--count];
        begin = current.begin;
        end = current.end;
        const unsigned depth = current.depth;
        if (sweep->remaining == 0) {
            return dd2_car_unresolved(hit, time, begin);
        }
        --sweep->remaining;
        const double span = end - begin;
        const dd2_car_window first = dd2_car_window_create(sweep->first, begin, end);
        const dd2_car_window second = dd2_car_window_create(sweep->second, begin, end);
        const dd2_vehicle_vector relative =
            dd2_collision_add(first.begin.center, dd2_collision_scale(second.begin.center, -1));
        const dd2_vehicle_vector finish =
            dd2_collision_add(first.end.center, dd2_collision_scale(second.end.center, -1));
        const dd2_car_interval candidate = dd2_car_translation(&first, &second, relative, finish);
        if (!candidate.found) {
            continue;
        }
        const double next_begin = begin + (span * candidate.time);
        const double next_end = begin + (span * candidate.leave);
        const double point_error =
            dd2_car_radius * (first.acceleration + second.acceleration) * span * span / 8;
        if (point_error <= dd2_car_contact_tolerance / 4) {
            const dd2_car_box actual_first =
                dd2_car_box_at(sweep->first.start, sweep->first.end, next_begin);
            const dd2_car_box actual_second =
                dd2_car_box_at(sweep->second.start, sweep->second.end, next_begin);
            const dd2_car_interval narrow = dd2_car_static(&actual_first, &actual_second);
            if (narrow.depth >= -dd2_car_contact_tolerance) {
                *hit = narrow;
                hit->depth = fmax(0, narrow.depth);
                *time = next_begin;
                return true;
            }
        }
        const double middle = (next_begin + next_end) / 2;
        if (depth >= DD2_CAR_REFINEMENT_DEPTH || middle <= next_begin || middle >= next_end) {
            return dd2_car_unresolved(hit, time, next_begin);
        }
        /* Discard certified empty time ranges; visit the earlier half next. */
        pending[count++] =
            (dd2_car_refinement){.begin = middle, .end = next_end, .depth = depth + 1};
        pending[count++] =
            (dd2_car_refinement){.begin = next_begin, .end = middle, .depth = depth + 1};
    }
    return false;
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
    dd2_car_sweep sweep = {.first = dd2_car_motion_create(first_start, first_end),
                           .second = dd2_car_motion_create(second_start, second_end),
                           .remaining = DD2_CAR_REFINEMENT_LIMIT};
    const dd2_car_box initial_first = dd2_car_box_at(first_start, first_end, 0);
    const dd2_car_box initial_second = dd2_car_box_at(second_start, second_end, 0);
    const dd2_car_interval overlap = dd2_car_static(&initial_first, &initial_second);
    if (overlap.found && overlap.depth > dd2_car_contact_tolerance) {
        *contact =
            (dd2_car_contact){.penetration = overlap.depth,
                              .normal = overlap.normal,
                              .point = dd2_car_point(&initial_first, &initial_second, overlap)};
        return true;
    }
    for (unsigned piece = 0; piece < pieces; ++piece) {
        dd2_car_interval hit = {0};
        double found_time = 0;
        if (!dd2_car_refine(&sweep, (double)piece / (double)pieces,
                            (double)(piece + 1) / (double)pieces, &hit, &found_time)) {
            continue;
        }
        if (hit.unresolved) {
            *contact = (dd2_car_contact){.time = found_time, .unresolved = true};
            return true;
        }
        const dd2_car_box first = dd2_car_box_at(first_start, first_end, found_time);
        const dd2_car_box second = dd2_car_box_at(second_start, second_end, found_time);
        *contact = (dd2_car_contact){.time = found_time,
                                     .penetration = hit.depth,
                                     .normal = hit.normal,
                                     .point = dd2_car_point(&first, &second, hit)};
        return true;
    }
    return false;
}
