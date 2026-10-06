#include "physics/road_surface.h"

#include "assets/level.h"
#include "assets/road.h"
#include "physics/numeric.h"
#include "physics/road_contact.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum { DD2_SURFACE_LEAF_CELLS = 4, DD2_SURFACE_STACK_SIZE = 32 };

typedef struct {
    double min[2];
    double max[2];
} dd2_surface_bounds;

typedef struct {
    dd2_surface_bounds bounds;
    double center[2];
    double key;
    uint32_t cell;
} dd2_surface_entry;

typedef struct {
    dd2_surface_bounds bounds;
    uint32_t first;
    uint32_t count;
    uint32_t left;
    uint32_t right;
} dd2_surface_node;

struct dd2_road_surface {
    const dd2_road *road;
    dd2_surface_entry *entries;
    dd2_surface_node *nodes;
    size_t node_count;
};

typedef struct {
    const dd2_road_surface *surface;
    dd2_surface_query query;
    dd2_road_contact contact;
    dd2_surface_statistics statistics;
    double highest;
    bool selecting;
    bool found;
} dd2_surface_search;

void dd2_road_surface_destroy(dd2_road_surface *surface) {
    if (surface != NULL) {
        free(surface->entries);
        free(surface->nodes);
        free(surface);
    }
}

static void dd2_surface_entry_init(dd2_surface_entry *entry, const dd2_road *road, uint32_t index) {
    const dd2_road_cell *cell = &dd2_road_cells(road)[index];
    const dd2_track_vertex *vertices = dd2_road_vertices(road);
    entry->cell = index;
    for (size_t corner = 0; corner < DD2_ROAD_CORNERS; ++corner) {
        const dd2_track_vertex vertex = vertices[cell->vertices[corner]];
        const double coordinates[] = {(double)vertex.x, (double)vertex.z};
        for (size_t axis = 0; axis < 2; ++axis) {
            if (corner == 0 || coordinates[axis] < entry->bounds.min[axis]) {
                entry->bounds.min[axis] = coordinates[axis];
            }
            if (corner == 0 || coordinates[axis] > entry->bounds.max[axis]) {
                entry->bounds.max[axis] = coordinates[axis];
            }
        }
    }
    for (size_t axis = 0; axis < 2; ++axis) {
        /* Source integer midpoints are exact binary64; sorting is deterministic. */
        entry->center[axis] = (entry->bounds.min[axis] + entry->bounds.max[axis]) / 2;
        entry->bounds.min[axis] -= DD2_ROAD_EDGE_TOLERANCE;
        entry->bounds.max[axis] += DD2_ROAD_EDGE_TOLERANCE;
    }
}

static int dd2_surface_entry_compare(const void *first, const void *second) {
    const dd2_surface_entry *left = first;
    const dd2_surface_entry *right = second;
    if (left->key < right->key) {
        return -1;
    }
    if (left->key > right->key) {
        return 1;
    }
    return (int)(left->cell > right->cell) - (int)(left->cell < right->cell);
}

static dd2_surface_bounds dd2_surface_range_bounds(const dd2_surface_entry *entries, size_t count) {
    dd2_surface_bounds bounds = entries[0].bounds;
    for (size_t index = 1; index < count; ++index) {
        for (size_t axis = 0; axis < 2; ++axis) {
            bounds.min[axis] = fmin(bounds.min[axis], entries[index].bounds.min[axis]);
            bounds.max[axis] = fmax(bounds.max[axis], entries[index].bounds.max[axis]);
        }
    }
    return bounds;
}

static void dd2_surface_build(dd2_road_surface *surface) {
    /* Breadth-first construction uses the already allocated node array as its
     * queue. Each split halves the range, independent of coincident centroids. */
    for (size_t index = 0; index < surface->node_count; ++index) {
        dd2_surface_node *node = &surface->nodes[index];
        const dd2_surface_node range = *node;
        dd2_surface_entry *entries = surface->entries + range.first;
        node->bounds = dd2_surface_range_bounds(entries, range.count);
        if (range.count <= DD2_SURFACE_LEAF_CELLS) {
            continue;
        }
        const size_t axis =
            node->bounds.max[0] - node->bounds.min[0] >= node->bounds.max[1] - node->bounds.min[1]
                ? 0U
                : 1U;
        for (size_t entry = 0; entry < range.count; ++entry) {
            entries[entry].key = entries[entry].center[axis];
        }
        qsort(entries, range.count, sizeof(*entries), dd2_surface_entry_compare);
        const uint32_t left_count = range.count / 2;
        node->count = 0;
        node->left = (uint32_t)surface->node_count++;
        node->right = (uint32_t)surface->node_count++;
        surface->nodes[node->left] = (dd2_surface_node){.first = range.first, .count = left_count};
        surface->nodes[node->right] = (dd2_surface_node){.first = range.first + left_count,
                                                         .count = range.count - left_count};
    }
}

dd2_road_surface *dd2_road_surface_create(const dd2_road *road) {
    const size_t count = dd2_road_cell_count(road);
    if (count == 0 || count > UINT32_MAX / 2 || count > SIZE_MAX / sizeof(dd2_surface_entry) ||
        count > SIZE_MAX / sizeof(dd2_surface_node) / 2) {
        return NULL;
    }
    dd2_road_surface *surface = calloc(1, sizeof(*surface));
    if (surface == NULL) {
        return NULL;
    }
    surface->road = road;
    surface->entries = calloc(count, sizeof(*surface->entries));
    surface->nodes = calloc((count * 2) - 1, sizeof(*surface->nodes));
    if (surface->entries == NULL || surface->nodes == NULL) {
        dd2_road_surface_destroy(surface);
        return NULL;
    }
    for (size_t index = 0; index < count; ++index) {
        dd2_surface_entry_init(&surface->entries[index], road, (uint32_t)index);
    }
    surface->node_count = 1;
    surface->nodes[0].count = (uint32_t)count;
    dd2_surface_build(surface);
    return surface;
}

static bool dd2_surface_contains(dd2_surface_bounds bounds, dd2_road_point point) {
    return point.x >= bounds.min[0] && point.x <= bounds.max[0] && point.z >= bounds.min[1] &&
           point.z <= bounds.max[1];
}

static void dd2_surface_consider(dd2_surface_search *search, dd2_road_contact candidate) {
    if (!search->selecting) {
        if (!search->found || candidate.height > search->highest) {
            search->highest = candidate.height;
        }
        search->found = true;
        return;
    }
    if (candidate.height < search->highest - DD2_ROAD_EDGE_TOLERANCE) {
        return;
    }
    if (!search->found) {
        search->contact = candidate;
        search->found = true;
    } else if (candidate.cell == search->query.preferred_cell ||
               (search->contact.cell != search->query.preferred_cell &&
                candidate.cell < search->contact.cell)) {
        search->contact = candidate;
    }
}

static void dd2_surface_leaf(dd2_surface_search *search, const dd2_surface_node *node) {
    for (size_t entry = node->first; entry < (size_t)node->first + node->count; ++entry) {
        const dd2_surface_entry *candidate = &search->surface->entries[entry];
        ++search->statistics.bounds_tests;
        if (!dd2_surface_contains(candidate->bounds, search->query.point)) {
            continue;
        }
        ++search->statistics.cell_tests;
        dd2_road_contact contact = {0};
        if (dd2_road_contact_cell(search->surface->road, candidate->cell, search->query.point,
                                  &contact) &&
            contact.height >= search->query.min_height &&
            contact.height <= search->query.max_height) {
            dd2_surface_consider(search, contact);
        }
    }
}

static void dd2_surface_visit(dd2_surface_search *search) {
    /* Creation limits cells to <2^31 and halves each range down to <=4 cells.
     * The pending DFS siblings therefore fit in this fixed, allocation-free stack. */
    uint32_t pending[DD2_SURFACE_STACK_SIZE] = {0};
    size_t count = 1;
    while (count != 0) {
        const dd2_surface_node *node = &search->surface->nodes[pending[--count]];
        ++search->statistics.bounds_tests;
        if (!dd2_surface_contains(node->bounds, search->query.point)) {
            continue;
        }
        if (node->count == 0) {
            pending[count++] = node->right;
            pending[count++] = node->left;
        } else {
            dd2_surface_leaf(search, node);
        }
    }
}

bool dd2_road_surface_sample(const dd2_road_surface *surface, dd2_surface_query query,
                             dd2_road_contact *result, dd2_surface_statistics *statistics) {
    if (statistics != NULL) {
        *statistics = (dd2_surface_statistics){0};
    }
    if (result == NULL) {
        return false;
    }
    *result = (dd2_road_contact){0};
    if (surface == NULL || !dd2_numeric_finite(&query.point.x) ||
        !dd2_numeric_finite(&query.point.z) || !dd2_numeric_finite(&query.min_height) ||
        !dd2_numeric_finite(&query.max_height) || query.min_height > query.max_height) {
        return false;
    }
    dd2_surface_search search = {.surface = surface, .query = query};
    dd2_surface_visit(&search);
    if (search.found) {
        /* Global maximum first, then stable tie selection: a pairwise tolerance
         * comparison is nontransitive and would depend on leaf visitation order. */
        search.found = false;
        search.selecting = true;
        dd2_surface_visit(&search);
    }
    *result = search.contact;
    if (statistics != NULL) {
        *statistics = search.statistics;
    }
    return search.found;
}

static const double dd2_surface_time_tolerance = 1e-10;
static const double dd2_surface_coordinate_limit = 2147483648.0;
static const double dd2_surface_recovery_limit = 1000;

typedef struct {
    const dd2_road_surface *surface;
    dd2_surface_sweep sweep;
    dd2_surface_bounds bounds;
    dd2_surface_hit hit;
    dd2_surface_statistics statistics;
    double earliest;
    bool found;
    bool selecting;
} dd2_surface_sweep_search;

static bool dd2_surface_intersects(dd2_surface_bounds first, dd2_surface_bounds second) {
    for (size_t axis = 0; axis < 2; ++axis) {
        if (first.min[axis] > second.max[axis] || first.max[axis] < second.min[axis]) {
            return false;
        }
    }
    return true;
}

static double dd2_surface_plane_distance(dd2_road_position point, dd2_track_vertex origin,
                                         const dd2_road_contact *plane) {
    return ((point.x - (double)origin.x) * plane->normal[0]) +
           ((point.y - (double)origin.y) * plane->normal[1]) +
           ((point.z - (double)origin.z) * plane->normal[2]);
}

static bool dd2_surface_triangle_sweep(const dd2_road *road, uint32_t cell, unsigned triangle,
                                       dd2_surface_sweep sweep, dd2_surface_hit *hit) {
    /* The first vertex of each triangle is also its corresponding quad corner. */
    const dd2_track_vertex origin =
        dd2_road_vertices(road)[dd2_road_cells(road)[cell].vertices[(size_t)triangle * 2]];
    dd2_road_contact plane = {0};
    if (!dd2_road_contact_triangle(road, cell, triangle,
                                   (dd2_road_point){.x = (double)origin.x, .z = (double)origin.z},
                                   &plane)) {
        return false;
    }
    const double start = dd2_surface_plane_distance(sweep.start, origin, &plane);
    const double end = dd2_surface_plane_distance(sweep.end, origin, &plane);
    double time = 0;
    double penetration = 0;
    if (start < 0) {
        if (start < -sweep.recovery) {
            return false;
        }
        penetration = -start;
    } else {
        if (end >= start || end > 0) {
            return false;
        }
        time = start / (start - end);
    }
    const dd2_road_position point = {.x = sweep.start.x + ((sweep.end.x - sweep.start.x) * time),
                                     .y = sweep.start.y + ((sweep.end.y - sweep.start.y) * time),
                                     .z = sweep.start.z + ((sweep.end.z - sweep.start.z) * time)};
    if (!dd2_road_contact_triangle(road, cell, triangle,
                                   (dd2_road_point){.x = point.x, .z = point.z}, &plane)) {
        return false;
    }
    *hit = (dd2_surface_hit){.time = time,
                             .penetration = penetration,
                             .point = {.x = point.x, .y = plane.height, .z = point.z},
                             .road = plane};
    return true;
}

static void dd2_surface_sweep_consider(dd2_surface_sweep_search *search,
                                       dd2_surface_hit candidate) {
    if (!search->selecting) {
        if (!search->found || candidate.time < search->earliest) {
            search->earliest = candidate.time;
        }
        search->found = true;
    } else if (candidate.time <= search->earliest + dd2_surface_time_tolerance &&
               (!search->found || candidate.road.cell < search->hit.road.cell ||
                (candidate.road.cell == search->hit.road.cell &&
                 candidate.road.triangle < search->hit.road.triangle))) {
        search->hit = candidate;
        search->found = true;
    }
}

static void dd2_surface_sweep_visit(dd2_surface_sweep_search *search) {
    uint32_t pending[DD2_SURFACE_STACK_SIZE] = {0};
    size_t count = 1;
    while (count != 0) {
        const dd2_surface_node *node = &search->surface->nodes[pending[--count]];
        ++search->statistics.bounds_tests;
        if (!dd2_surface_intersects(node->bounds, search->bounds)) {
            continue;
        }
        if (node->count == 0) {
            pending[count++] = node->right;
            pending[count++] = node->left;
            continue;
        }
        for (size_t entry = node->first; entry < (size_t)node->first + node->count; ++entry) {
            const dd2_surface_entry *candidate = &search->surface->entries[entry];
            ++search->statistics.bounds_tests;
            if (!dd2_surface_intersects(candidate->bounds, search->bounds)) {
                continue;
            }
            ++search->statistics.cell_tests;
            for (unsigned triangle = 0; triangle < 2; ++triangle) {
                dd2_surface_hit hit = {0};
                if (dd2_surface_triangle_sweep(search->surface->road, candidate->cell, triangle,
                                               search->sweep, &hit)) {
                    dd2_surface_sweep_consider(search, hit);
                }
            }
        }
    }
}

static bool dd2_surface_position_valid(const dd2_road_position *position) {
    return dd2_numeric_finite(&position->x) && dd2_numeric_finite(&position->y) &&
           dd2_numeric_finite(&position->z) && fabs(position->x) <= dd2_surface_coordinate_limit &&
           fabs(position->y) <= dd2_surface_coordinate_limit &&
           fabs(position->z) <= dd2_surface_coordinate_limit;
}

bool dd2_road_surface_sweep(const dd2_road_surface *surface, dd2_surface_sweep sweep,
                            dd2_surface_hit *result, dd2_surface_statistics *statistics) {
    if (statistics != NULL) {
        *statistics = (dd2_surface_statistics){0};
    }
    if (result == NULL) {
        return false;
    }
    *result = (dd2_surface_hit){0};
    if (surface == NULL || !dd2_surface_position_valid(&sweep.start) ||
        !dd2_surface_position_valid(&sweep.end) || !dd2_numeric_finite(&sweep.recovery) ||
        sweep.recovery < 0 || sweep.recovery > dd2_surface_recovery_limit) {
        return false;
    }
    dd2_surface_sweep_search search = {
        .surface = surface,
        .sweep = sweep,
        .bounds = {.min = {fmin(sweep.start.x, sweep.end.x), fmin(sweep.start.z, sweep.end.z)},
                   .max = {fmax(sweep.start.x, sweep.end.x), fmax(sweep.start.z, sweep.end.z)}}};
    dd2_surface_sweep_visit(&search);
    if (search.found) {
        search.selecting = true;
        search.found = false;
        dd2_surface_sweep_visit(&search);
    }
    *result = search.hit;
    if (statistics != NULL) {
        *statistics = search.statistics;
    }
    return search.found;
}
