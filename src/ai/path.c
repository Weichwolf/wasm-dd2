#include "ai/path.h"

#include "assets/level.h"
#include "assets/road.h"
#include "physics/numeric.h"
#include "physics/vehicle.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

enum { DD2_AI_PATH_STEPS = 128 };
static const double dd2_ai_path_limit = 16000;
static const double dd2_ai_path_position_limit = 2147483648.0;
static const double dd2_ai_path_epsilon = 1e-8;

static dd2_vehicle_vector dd2_ai_path_cell(const dd2_road *road, uint32_t index) {
    const dd2_road_cell *cell = &dd2_road_cells(road)[index];
    static const unsigned triangle[2][3] = {{0, 1, 3}, {2, 3, 1}};
    dd2_vehicle_vector center = {0};
    const unsigned count = cell->triangle_mask == 3 ? DD2_ROAD_CORNERS : 3;
    for (unsigned corner = 0; corner < count; ++corner) {
        const unsigned selected =
            cell->triangle_mask == 3 ? corner : triangle[cell->triangle_mask == 1 ? 0 : 1][corner];
        const dd2_track_vertex point = dd2_road_vertices(road)[cell->vertices[selected]];
        center.x += (double)point.x / (double)count;
        center.y += (double)point.y / (double)count;
        center.z += (double)point.z / (double)count;
    }
    return center;
}

typedef struct {
    uint32_t strip;
    double fraction;
} dd2_ai_path_lane;
static dd2_vehicle_vector dd2_ai_path_center(const dd2_road *road, dd2_ai_path_lane lane) {
    const dd2_road_strip strip = dd2_road_strips(road)[lane.strip];
    const double coordinate =
        fmax(0, fmin((double)strip.lanes - 1, (lane.fraction * (double)strip.lanes) - (1.0 / 2)));
    const unsigned first = (unsigned)floor(coordinate);
    const unsigned second = first + 1 < strip.lanes ? first + 1 : first;
    const double fraction = coordinate - (double)first;
    const dd2_vehicle_vector begin = dd2_ai_path_cell(road, strip.first_cell + first);
    const dd2_vehicle_vector end = dd2_ai_path_cell(road, strip.first_cell + second);
    return (dd2_vehicle_vector){.x = begin.x + ((end.x - begin.x) * fraction),
                                .y = begin.y + ((end.y - begin.y) * fraction),
                                .z = begin.z + ((end.z - begin.z) * fraction)};
}

static double dd2_ai_path_width(const dd2_road *road, uint32_t index) {
    const dd2_road_strip strip = dd2_road_strips(road)[index];
    const dd2_road_cell *cells = dd2_road_cells(road);
    const dd2_track_vertex *points = dd2_road_vertices(road);
    const dd2_road_cell first = cells[strip.first_cell];
    const dd2_road_cell last = cells[strip.first_cell + strip.lanes - 1];
    const double xpos = ((double)points[last.vertices[1]].x + points[last.vertices[2]].x -
                         points[first.vertices[0]].x - points[first.vertices[3]].x) /
                        2;
    const double zpos = ((double)points[last.vertices[1]].z + points[last.vertices[2]].z -
                         points[first.vertices[0]].z - points[first.vertices[3]].z) /
                        2;
    return hypot(xpos, zpos);
}

static bool dd2_ai_path_valid(const dd2_road *road, const dd2_ai_path_query *query) {
    return road != NULL && query->cell < dd2_road_cell_count(road) &&
           dd2_road_cells(road)[query->cell].strip != DD2_ROAD_NO_STRIP &&
           dd2_numeric_finite(&query->lane) && query->lane >= 0 && query->lane <= 1 &&
           dd2_numeric_finite(&query->distance) && query->distance >= 0 &&
           query->distance <= dd2_ai_path_limit && dd2_numeric_finite(&query->position.x) &&
           dd2_numeric_finite(&query->position.y) && dd2_numeric_finite(&query->position.z) &&
           fabs(query->position.x) <= dd2_ai_path_position_limit &&
           fabs(query->position.y) <= dd2_ai_path_position_limit &&
           fabs(query->position.z) <= dd2_ai_path_position_limit;
}

bool dd2_ai_path(const dd2_road *road, dd2_ai_path_query query, dd2_ai_path_sample *sample) {
    if (sample == NULL) {
        return false;
    }
    *sample = (dd2_ai_path_sample){0};
    if (!dd2_ai_path_valid(road, &query)) {
        return false;
    }
    uint32_t strip = dd2_road_cells(road)[query.cell].strip;
    dd2_vehicle_vector begin =
        dd2_ai_path_center(road, (dd2_ai_path_lane){.strip = strip, .fraction = query.lane});
    dd2_vehicle_vector previous = {0};
    double remaining = query.distance;
    double curvature = 0;
    double previous_length = 0;
    for (unsigned step = 0; step < DD2_AI_PATH_STEPS; ++step) {
        const uint32_t next = dd2_road_strips(road)[strip].next;
        const dd2_vehicle_vector end =
            dd2_ai_path_center(road, (dd2_ai_path_lane){.strip = next, .fraction = query.lane});
        const dd2_vehicle_vector delta = {.x = end.x - begin.x, .z = end.z - begin.z};
        const double length = hypot(delta.x, delta.z);
        if (length > dd2_ai_path_epsilon) {
            const dd2_vehicle_vector direction = {.x = delta.x / length, .z = delta.z / length};
            if (previous_length > 0) {
                const double angle =
                    fabs(atan2((previous.x * direction.z) - (previous.z * direction.x),
                               (previous.x * direction.x) + (previous.z * direction.z)));
                curvature = fmax(curvature, 2 * angle / (previous_length + length));
            }
            double offset = 0;
            if (step == 0) {
                offset = fmax(0, fmin(length, ((query.position.x - begin.x) * direction.x) +
                                                  ((query.position.z - begin.z) * direction.z)));
            }
            if (remaining <= length - offset) {
                const double fraction = (offset + remaining) / length;
                *sample =
                    (dd2_ai_path_sample){.point = {.x = begin.x + ((end.x - begin.x) * fraction),
                                                   .y = begin.y + ((end.y - begin.y) * fraction),
                                                   .z = begin.z + ((end.z - begin.z) * fraction)},
                                         .direction = direction,
                                         .width = dd2_ai_path_width(road, strip),
                                         .curvature = curvature,
                                         .strip = strip};
                return true;
            }
            remaining -= length - offset;
            previous = direction;
            previous_length = length;
        }
        strip = next;
        begin = end;
    }
    return false;
}
