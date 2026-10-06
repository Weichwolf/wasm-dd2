#include "physics/road_contact.h"

#include "assets/level.h"
#include "assets/road.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* World-coordinate tolerance, not a fraction of an arbitrarily large cell. */
static const double dd2_contact_edge_tolerance = DD2_ROAD_EDGE_TOLERANCE;

typedef struct {
    double value[3];
} dd2_contact_vector;

static dd2_contact_vector dd2_contact_difference(dd2_track_vertex first, dd2_track_vertex second) {
    return (dd2_contact_vector){.value = {(double)first.x - (double)second.x,
                                          (double)first.y - (double)second.y,
                                          (double)first.z - (double)second.z}};
}

static bool dd2_contact_bounds(const dd2_track_vertex vertices[3], dd2_road_point point) {
    double min_x = (double)vertices[0].x;
    double max_x = min_x;
    double min_z = (double)vertices[0].z;
    double max_z = min_z;
    for (size_t index = 1; index < 3; ++index) {
        min_x = fmin(min_x, (double)vertices[index].x);
        max_x = fmax(max_x, (double)vertices[index].x);
        min_z = fmin(min_z, (double)vertices[index].z);
        max_z = fmax(max_z, (double)vertices[index].z);
    }
    return point.x >= min_x - DD2_ROAD_EDGE_TOLERANCE &&
           point.x <= max_x + DD2_ROAD_EDGE_TOLERANCE &&
           point.z >= min_z - DD2_ROAD_EDGE_TOLERANCE && point.z <= max_z + DD2_ROAD_EDGE_TOLERANCE;
}

static bool dd2_contact_triangle(const dd2_track_vertex vertices[3], dd2_road_point point,
                                 dd2_road_contact *result) {
    /* Per-edge tolerance alone can extend an acute corner arbitrarily far.
     * Clip to world-distance bounds before interpolation and BVH selection. */
    if (!dd2_contact_bounds(vertices, point)) {
        return false;
    }
    const dd2_contact_vector edge = dd2_contact_difference(vertices[1], vertices[0]);
    const dd2_contact_vector other = dd2_contact_difference(vertices[2], vertices[0]);
    const double area = (edge.value[0] * other.value[2]) - (edge.value[2] * other.value[0]);
    if (area == 0) {
        return false;
    }
    const double relative_x = point.x - (double)vertices[0].x;
    const double relative_z = point.z - (double)vertices[0].z;
    const double weight_edge =
        ((relative_x * other.value[2]) - (relative_z * other.value[0])) / area;
    const double weight_other =
        ((edge.value[0] * relative_z) - (edge.value[2] * relative_x)) / area;
    const double tolerance_scale = dd2_contact_edge_tolerance / fabs(area);
    const double edge_tolerance = tolerance_scale * hypot(other.value[0], other.value[2]);
    const double other_tolerance = tolerance_scale * hypot(edge.value[0], edge.value[2]);
    const double start_tolerance =
        tolerance_scale * hypot(other.value[0] - edge.value[0], other.value[2] - edge.value[2]);
    if (weight_edge < -edge_tolerance || weight_other < -other_tolerance ||
        weight_edge + weight_other > 1 + start_tolerance) {
        return false;
    }
    result->height =
        (double)vertices[0].y + (weight_edge * edge.value[1]) + (weight_other * other.value[1]);
    const double normal[] = {(edge.value[1] * other.value[2]) - (edge.value[2] * other.value[1]),
                             -area,
                             (edge.value[0] * other.value[1]) - (edge.value[1] * other.value[0])};
    const double length =
        sqrt((normal[0] * normal[0]) + (normal[1] * normal[1]) + (normal[2] * normal[2]));
    const double factor = (normal[1] > 0 ? 1 : -1) / length;
    for (size_t axis = 0; axis < 3; ++axis) {
        result->normal[axis] = normal[axis] * factor;
    }
    return true;
}

bool dd2_road_contact_cell(const dd2_road *road, size_t cell, dd2_road_point point,
                           dd2_road_contact *result) {
    if (result == NULL) {
        return false;
    }
    *result = (dd2_road_contact){0};
    if (cell >= dd2_road_cell_count(road)) {
        return false;
    }
    const dd2_road_cell *surface = &dd2_road_cells(road)[cell];
    const dd2_track_vertex *vertices = dd2_road_vertices(road);
    static const size_t corners[2][3] = {{0, 1, 3}, {2, 3, 1}};
    for (unsigned triangle = 0; triangle < 2; ++triangle) {
        if ((surface->triangle_mask & (1U << triangle)) == 0) {
            continue;
        }
        dd2_track_vertex points[3] = {0};
        for (size_t corner = 0; corner < 3; ++corner) {
            points[corner] = vertices[surface->vertices[corners[triangle][corner]]];
        }
        if (dd2_contact_triangle(points, point, result)) {
            result->cell = (uint32_t)cell;
            result->triangle = triangle;
            return true;
        }
    }
    return false;
}
