#include "assets/barriers.h"

#include "assets/level.h"
#include "assets/road.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum { DD2_BARRIER_RACING_LEVELS = 7, DD2_BARRIER_LEVELS = 11, DD2_BARRIER_SIDES = 2 };
struct dd2_barriers {
    dd2_barrier_segment *segments;
    size_t count;
    double radius;
};

/* Barrier_Collision / Barrier_Corner_Collision offsets applied to the first
 * and last decoded lane. Diagonal edges follow the active source triangle. */
static const unsigned dd2_barrier_first[12][2] = {{0, 3}, {0, 3}, {1, 3}, {0, 3}, {1, 3}, {0, 2},
                                                  {0, 3}, {0, 2}, {0, 3}, {0, 3}, {1, 3}, {0, 3}};
static const unsigned dd2_barrier_last[12][2] = {{2, 1}, {2, 1}, {2, 1}, {2, 0}, {2, 0}, {2, 1},
                                                 {3, 1}, {3, 1}, {2, 1}, {2, 1}, {3, 1}, {2, 1}};

void dd2_barriers_destroy(dd2_barriers *barriers) {
    if (barriers != NULL) {
        free(barriers->segments);
        free(barriers);
    }
}

dd2_barriers *dd2_barriers_create(const dd2_road *road, unsigned level) {
    if (road == NULL || level == 0 || level > DD2_BARRIER_LEVELS ||
        ((level <= DD2_BARRIER_RACING_LEVELS) != (dd2_road_strip_count(road) != 0))) {
        return NULL;
    }
    dd2_barriers *barriers = calloc(1, sizeof(*barriers));
    if (barriers == NULL) {
        return NULL;
    }
    if (level > DD2_BARRIER_RACING_LEVELS) {
        static const double radii[] = {14990, 14600, 14400, 14400};
        barriers->radius = radii[level - DD2_BARRIER_RACING_LEVELS - 1];
        return barriers;
    }
    barriers->count = dd2_road_strip_count(road) * DD2_BARRIER_SIDES;
    barriers->segments = calloc(barriers->count, sizeof(*barriers->segments));
    if (barriers->segments == NULL) {
        dd2_barriers_destroy(barriers);
        return NULL;
    }
    const dd2_road_strip *strips = dd2_road_strips(road);
    const dd2_road_cell *cells = dd2_road_cells(road);
    const dd2_track_vertex *vertices = dd2_road_vertices(road);
    for (size_t index = 0; index < dd2_road_strip_count(road); ++index) {
        const dd2_road_strip *strip = &strips[index];
        for (unsigned side = 0; side < DD2_BARRIER_SIDES; ++side) {
            const dd2_road_cell *cell =
                &cells[strip->first_cell + (side == 0 ? 0U : (unsigned)strip->lanes - 1)];
            const unsigned *corners =
                side == 0 ? dd2_barrier_first[strip->kind] : dd2_barrier_last[strip->kind];
            barriers->segments[(index * DD2_BARRIER_SIDES) + side] =
                (dd2_barrier_segment){.start = vertices[cell->vertices[corners[0]]],
                                      .end = vertices[cell->vertices[corners[1]]],
                                      .strip = (uint32_t)index,
                                      .side = side};
        }
    }
    return barriers;
}

size_t dd2_barriers_count(const dd2_barriers *barriers) {
    return barriers != NULL ? barriers->count : 0;
}
const dd2_barrier_segment *dd2_barriers_segments(const dd2_barriers *barriers) {
    return barriers != NULL ? barriers->segments : NULL;
}
double dd2_barriers_radius(const dd2_barriers *barriers) {
    return barriers != NULL ? barriers->radius : 0;
}
