#include "physics/barrier_world.h"

#include "assets/barriers.h"
#include "assets/level.h"
#include "physics/numeric.h"
#include "physics/vehicle.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum { DD2_BARRIER_LEAF = 4, DD2_BARRIER_STACK = 32, DD2_BARRIER_AXES = 3 };
static const double dd2_barrier_height = 400;
static const double dd2_barrier_tolerance = 1e-6;
static const double dd2_barrier_time_tolerance = 1e-10;
static const double dd2_barrier_coordinate_limit = 2147483648.0;
static const double dd2_barrier_size_limit = 10000;

typedef struct {
    double min[DD2_BARRIER_AXES];
    double max[DD2_BARRIER_AXES];
} dd2_barrier_bounds;
typedef struct {
    dd2_barrier_bounds bounds;
    double center[DD2_BARRIER_AXES];
    double key;
    uint32_t barrier;
} dd2_barrier_entry;
typedef struct {
    dd2_barrier_bounds bounds;
    uint32_t first;
    uint32_t count;
    uint32_t left;
    uint32_t right;
} dd2_barrier_node;
struct dd2_barrier_world {
    const dd2_barriers *barriers;
    dd2_barrier_entry *entries;
    dd2_barrier_node *nodes;
    size_t node_count;
};
typedef struct {
    double enter;
    double leave;
    dd2_vehicle_vector normal;
} dd2_barrier_interval;

static bool dd2_barrier_clip(dd2_barrier_interval *interval, double position, double delta,
                             double low, double high, dd2_vehicle_vector axis) {
    if (delta == 0) {
        return position >= low && position <= high;
    }
    double first = (low - position) / delta;
    double last = (high - position) / delta;
    double sign = -1;
    if (first > last) {
        const double saved = first;
        first = last;
        last = saved;
        sign = 1;
    }
    if (first > interval->enter) {
        interval->enter = first;
        interval->normal =
            (dd2_vehicle_vector){.x = axis.x * sign, .y = axis.y * sign, .z = axis.z * sign};
    }
    interval->leave = fmin(interval->leave, last);
    return interval->enter <= interval->leave;
}

static bool dd2_barrier_circle_interval(dd2_barrier_interval *interval, dd2_barrier_sweep sweep,
                                        dd2_track_vertex center) {
    const double xpos = sweep.start.x - (double)center.x;
    const double zpos = sweep.start.z - (double)center.z;
    const double delta_x = sweep.end.x - sweep.start.x;
    const double delta_z = sweep.end.z - sweep.start.z;
    const double speed_squared = (delta_x * delta_x) + (delta_z * delta_z);
    const double distance = (xpos * xpos) + (zpos * zpos) - (sweep.radius * sweep.radius);
    if (speed_squared == 0) {
        return distance <= 0;
    }
    /* Ray coordinates avoid cancellation of b*b-a*c at distant starts. */
    const double speed = sqrt(speed_squared);
    const double along = ((xpos * delta_x) + (zpos * delta_z)) / speed;
    const double perpendicular = ((xpos * delta_z) - (zpos * delta_x)) / speed;
    const double discriminant = (sweep.radius * sweep.radius) - (perpendicular * perpendicular);
    if (discriminant < -dd2_barrier_tolerance * sweep.radius) {
        return false;
    }
    const double root = sqrt(fmax(0, discriminant));
    const double first = (-along - root) / speed;
    const double last = (-along + root) / speed;
    interval->enter = fmax(interval->enter, first);
    interval->leave = fmin(interval->leave, last);
    return interval->enter <= interval->leave;
}

static bool dd2_barrier_contact_make(dd2_barrier_segment segment, dd2_barrier_sweep sweep,
                                     dd2_barrier_interval interval, dd2_barrier_contact *contact) {
    const double time = interval.enter;
    const dd2_vehicle_vector position = {
        .x = sweep.start.x + ((sweep.end.x - sweep.start.x) * time),
        .y = sweep.start.y + ((sweep.end.y - sweep.start.y) * time),
        .z = sweep.start.z + ((sweep.end.z - sweep.start.z) * time)};
    const double delta_x = (double)segment.end.x - (double)segment.start.x;
    const double delta_z = (double)segment.end.z - (double)segment.start.z;
    const double length_squared = (delta_x * delta_x) + (delta_z * delta_z);
    const double fraction =
        length_squared > 0 ? fmax(0, fmin(1, (((position.x - (double)segment.start.x) * delta_x) +
                                              ((position.z - (double)segment.start.z) * delta_z)) /
                                                 length_squared))
                           : 0;
    const dd2_vehicle_vector nearest = {
        .x = (double)segment.start.x + (delta_x * fraction),
        .y = (double)segment.start.y +
             (((double)segment.end.y - (double)segment.start.y) * fraction),
        .z = (double)segment.start.z + (delta_z * fraction)};
    const double distance = hypot(position.x - nearest.x, position.z - nearest.z);
    dd2_vehicle_vector normal = interval.normal;
    double penetration = 0;
    if (normal.y == 0 || time == 0) {
        if (distance > dd2_barrier_tolerance) {
            normal = (dd2_vehicle_vector){.x = (position.x - nearest.x) / distance,
                                          .z = (position.z - nearest.z) / distance};
        } else if (length_squared > 0) {
            const double length = sqrt(length_squared);
            normal = (dd2_vehicle_vector){.x = delta_z / length, .z = -delta_x / length};
        } else {
            normal = (dd2_vehicle_vector){.x = 1};
        }
        penetration = fmax(0, sweep.radius - distance);
    }
    const dd2_vehicle_vector delta = {.x = sweep.end.x - sweep.start.x,
                                      .y = sweep.end.y - sweep.start.y,
                                      .z = sweep.end.z - sweep.start.z};
    const double approach = (delta.x * normal.x) + (delta.y * normal.y) + (delta.z * normal.z);
    if (time == 0 && penetration <= dd2_barrier_tolerance && approach >= -dd2_barrier_tolerance) {
        return false;
    }
    *contact = (dd2_barrier_contact){
        .time = time,
        .penetration = penetration,
        .normal = normal,
        .point = {.x = nearest.x,
                  .y = normal.y != 0
                           ? position.y - (normal.y * sweep.half_height)
                           : fmax(nearest.y, fmin(nearest.y + dd2_barrier_height, position.y)),
                  .z = nearest.z}};
    return true;
}

static void dd2_barrier_select(dd2_barrier_contact candidate, dd2_barrier_contact *contact,
                               bool *found) {
    if (!*found || candidate.time < contact->time) {
        *contact = candidate;
        *found = true;
    }
}

static bool dd2_barrier_segment_sweep(dd2_barrier_segment segment, dd2_barrier_sweep sweep,
                                      dd2_barrier_contact *contact) {
    bool found = false;
    const double delta_x = (double)segment.end.x - (double)segment.start.x;
    const double delta_z = (double)segment.end.z - (double)segment.start.z;
    const double length = hypot(delta_x, delta_z);
    if (length > 0) {
        const double along = (((sweep.start.x - (double)segment.start.x) * delta_x) +
                              ((sweep.start.z - (double)segment.start.z) * delta_z)) /
                             length;
        const double move_along = (((sweep.end.x - sweep.start.x) * delta_x) +
                                   ((sweep.end.z - sweep.start.z) * delta_z)) /
                                  length;
        const double across = (((sweep.start.x - (double)segment.start.x) * delta_z) -
                               ((sweep.start.z - (double)segment.start.z) * delta_x)) /
                              length;
        const double move_across = (((sweep.end.x - sweep.start.x) * delta_z) -
                                    ((sweep.end.z - sweep.start.z) * delta_x)) /
                                   length;
        const double slope = ((double)segment.end.y - (double)segment.start.y) / length;
        dd2_barrier_interval interval = {.leave = 1};
        if (dd2_barrier_clip(&interval, across, move_across, -sweep.radius, sweep.radius,
                             (dd2_vehicle_vector){.x = delta_z / length, .z = -delta_x / length}) &&
            dd2_barrier_clip(&interval, along, move_along, 0, length,
                             (dd2_vehicle_vector){.x = delta_x / length, .z = delta_z / length}) &&
            dd2_barrier_clip(&interval, sweep.start.y - (double)segment.start.y - (along * slope),
                             sweep.end.y - sweep.start.y - (move_along * slope), -sweep.half_height,
                             dd2_barrier_height + sweep.half_height,
                             (dd2_vehicle_vector){.y = 1})) {
            dd2_barrier_contact candidate = {0};
            if (dd2_barrier_contact_make(segment, sweep, interval, &candidate)) {
                dd2_barrier_select(candidate, contact, &found);
            }
        }
    }
    const dd2_track_vertex ends[] = {segment.start, segment.end};
    for (unsigned endpoint = 0; endpoint < 2; ++endpoint) {
        dd2_barrier_interval interval = {.leave = 1};
        if (dd2_barrier_circle_interval(&interval, sweep, ends[endpoint]) &&
            dd2_barrier_clip(&interval, sweep.start.y - (double)ends[endpoint].y,
                             sweep.end.y - sweep.start.y, -sweep.half_height,
                             dd2_barrier_height + sweep.half_height,
                             (dd2_vehicle_vector){.y = 1})) {
            dd2_barrier_contact candidate = {0};
            if (dd2_barrier_contact_make(segment, sweep, interval, &candidate)) {
                dd2_barrier_select(candidate, contact, &found);
            }
        }
    }
    return found;
}

static bool dd2_barrier_arena_sweep(double arena_radius, dd2_barrier_sweep sweep,
                                    dd2_barrier_contact *contact) {
    const double radius = arena_radius - sweep.radius;
    if (radius <= 0) {
        return false;
    }
    const double start_distance = hypot(sweep.start.x, sweep.start.z);
    double time = 0;
    if (start_distance < radius) {
        if (hypot(sweep.end.x, sweep.end.z) < radius) {
            return false;
        }
        const double delta_x = sweep.end.x - sweep.start.x;
        const double delta_z = sweep.end.z - sweep.start.z;
        const double speed_squared = (delta_x * delta_x) + (delta_z * delta_z);
        const double projection = (sweep.start.x * delta_x) + (sweep.start.z * delta_z);
        const double distance = (start_distance * start_distance) - (radius * radius);
        const double root = sqrt((projection * projection) - (speed_squared * distance));
        time = projection >= 0 ? -distance / (projection + root)
                               : (-projection + root) / speed_squared;
    }
    const dd2_vehicle_vector position = {
        .x = sweep.start.x + ((sweep.end.x - sweep.start.x) * time),
        .y = sweep.start.y + ((sweep.end.y - sweep.start.y) * time),
        .z = sweep.start.z + ((sweep.end.z - sweep.start.z) * time)};
    const double length = hypot(position.x, position.z);
    const dd2_vehicle_vector normal = {.x = -position.x / length, .z = -position.z / length};
    const double penetration = fmax(0, start_distance - radius);
    const double approach =
        ((sweep.end.x - sweep.start.x) * normal.x) + ((sweep.end.z - sweep.start.z) * normal.z);
    if (time == 0 && penetration <= dd2_barrier_tolerance && approach >= -dd2_barrier_tolerance) {
        return false;
    }
    *contact = (dd2_barrier_contact){
        .time = fmax(0, fmin(1, time)),
        .penetration = penetration,
        .normal = normal,
        .point = {.x = -normal.x * arena_radius, .y = position.y, .z = -normal.z * arena_radius}};
    return true;
}

void dd2_barrier_world_destroy(dd2_barrier_world *world) {
    if (world != NULL) {
        free(world->entries);
        free(world->nodes);
        free(world);
    }
}

static int dd2_barrier_entry_compare(const void *first, const void *second) {
    const dd2_barrier_entry *left = first;
    const dd2_barrier_entry *right = second;
    if (left->key < right->key) {
        return -1;
    }
    if (left->key > right->key) {
        return 1;
    }
    return (int)(left->barrier > right->barrier) - (int)(left->barrier < right->barrier);
}

static void dd2_barrier_build(dd2_barrier_world *world) {
    for (size_t index = 0; index < world->node_count; ++index) {
        dd2_barrier_node *node = &world->nodes[index];
        const dd2_barrier_node range = *node;
        dd2_barrier_entry *entries = world->entries + range.first;
        node->bounds = entries[0].bounds;
        for (size_t entry = 1; entry < range.count; ++entry) {
            for (size_t axis = 0; axis < DD2_BARRIER_AXES; ++axis) {
                node->bounds.min[axis] =
                    fmin(node->bounds.min[axis], entries[entry].bounds.min[axis]);
                node->bounds.max[axis] =
                    fmax(node->bounds.max[axis], entries[entry].bounds.max[axis]);
            }
        }
        if (range.count <= DD2_BARRIER_LEAF) {
            continue;
        }
        size_t axis = 0;
        for (size_t candidate = 1; candidate < DD2_BARRIER_AXES; ++candidate) {
            if (node->bounds.max[candidate] - node->bounds.min[candidate] >
                node->bounds.max[axis] - node->bounds.min[axis]) {
                axis = candidate;
            }
        }
        for (size_t entry = 0; entry < range.count; ++entry) {
            entries[entry].key = entries[entry].center[axis];
        }
        qsort(entries, range.count, sizeof(*entries), dd2_barrier_entry_compare);
        const uint32_t left_count = range.count / 2;
        node->count = 0;
        node->left = (uint32_t)world->node_count++;
        node->right = (uint32_t)world->node_count++;
        world->nodes[node->left] = (dd2_barrier_node){.first = range.first, .count = left_count};
        world->nodes[node->right] = (dd2_barrier_node){.first = range.first + left_count,
                                                       .count = range.count - left_count};
    }
}

dd2_barrier_world *dd2_barrier_world_create(const dd2_barriers *barriers) {
    const size_t count = dd2_barriers_count(barriers);
    if (barriers == NULL || (count == 0 && dd2_barriers_radius(barriers) <= 0) ||
        count > UINT32_MAX / 2) {
        return NULL;
    }
    dd2_barrier_world *world = calloc(1, sizeof(*world));
    if (world == NULL) {
        return NULL;
    }
    world->barriers = barriers;
    if (count == 0) {
        return world;
    }
    world->entries = calloc(count, sizeof(*world->entries));
    world->nodes = calloc((count * 2) - 1, sizeof(*world->nodes));
    if (world->entries == NULL || world->nodes == NULL) {
        dd2_barrier_world_destroy(world);
        return NULL;
    }
    const dd2_barrier_segment *segments = dd2_barriers_segments(barriers);
    for (size_t index = 0; index < count; ++index) {
        dd2_barrier_entry *entry = &world->entries[index];
        entry->barrier = (uint32_t)index;
        const double start[] = {(double)segments[index].start.x, (double)segments[index].start.y,
                                (double)segments[index].start.z};
        const double end[] = {(double)segments[index].end.x, (double)segments[index].end.y,
                              (double)segments[index].end.z};
        for (size_t axis = 0; axis < DD2_BARRIER_AXES; ++axis) {
            entry->bounds.min[axis] = fmin(start[axis], end[axis]) - dd2_barrier_tolerance;
            entry->bounds.max[axis] = fmax(start[axis], end[axis]) + dd2_barrier_tolerance;
            if (axis == 1) {
                entry->bounds.max[axis] += dd2_barrier_height;
            }
            entry->center[axis] = (entry->bounds.min[axis] + entry->bounds.max[axis]) / 2;
        }
    }
    world->node_count = 1;
    world->nodes[0].count = (uint32_t)count;
    dd2_barrier_build(world);
    return world;
}

static bool dd2_barrier_overlap(dd2_barrier_bounds first, dd2_barrier_bounds second) {
    for (size_t axis = 0; axis < DD2_BARRIER_AXES; ++axis) {
        if (first.min[axis] > second.max[axis] || first.max[axis] < second.min[axis]) {
            return false;
        }
    }
    return true;
}

typedef struct {
    dd2_barrier_sweep sweep;
    dd2_barrier_bounds bounds;
    dd2_barrier_contact contact;
    dd2_barrier_statistics statistics;
    double earliest;
    bool selecting;
    bool found;
} dd2_barrier_search;

static void dd2_barrier_consider(dd2_barrier_search *search, dd2_barrier_contact candidate) {
    if (!search->selecting) {
        search->earliest = search->found ? fmin(search->earliest, candidate.time) : candidate.time;
        search->found = true;
    } else if (candidate.time <= search->earliest + dd2_barrier_time_tolerance &&
               (!search->found || candidate.barrier < search->contact.barrier)) {
        search->contact = candidate;
        search->found = true;
    }
}

static void dd2_barrier_visit(const dd2_barrier_world *world, dd2_barrier_search *search) {
    uint32_t pending[DD2_BARRIER_STACK] = {0};
    size_t count = 1;
    while (count != 0) {
        const dd2_barrier_node *node = &world->nodes[pending[--count]];
        ++search->statistics.bounds_tests;
        if (!dd2_barrier_overlap(node->bounds, search->bounds)) {
            continue;
        }
        if (node->count == 0) {
            pending[count++] = node->right;
            pending[count++] = node->left;
            continue;
        }
        for (size_t index = node->first; index < (size_t)node->first + node->count; ++index) {
            const dd2_barrier_entry *entry = &world->entries[index];
            ++search->statistics.bounds_tests;
            if (!dd2_barrier_overlap(entry->bounds, search->bounds)) {
                continue;
            }
            ++search->statistics.segment_tests;
            dd2_barrier_contact contact = {0};
            if (dd2_barrier_segment_sweep(dd2_barriers_segments(world->barriers)[entry->barrier],
                                          search->sweep, &contact)) {
                contact.barrier = entry->barrier;
                dd2_barrier_consider(search, contact);
            }
        }
    }
}

static bool dd2_barrier_number_valid(const double *number, double limit) {
    return dd2_numeric_finite(number) && fabs(*number) <= limit;
}

static bool dd2_barrier_vector_valid(const dd2_vehicle_vector *vector) {
    return dd2_barrier_number_valid(&vector->x, dd2_barrier_coordinate_limit) &&
           dd2_barrier_number_valid(&vector->y, dd2_barrier_coordinate_limit) &&
           dd2_barrier_number_valid(&vector->z, dd2_barrier_coordinate_limit);
}

bool dd2_barrier_world_sweep(const dd2_barrier_world *world, dd2_barrier_sweep sweep,
                             dd2_barrier_contact *contact, dd2_barrier_statistics *statistics) {
    if (statistics != NULL) {
        *statistics = (dd2_barrier_statistics){0};
    }
    if (contact == NULL) {
        return false;
    }
    *contact = (dd2_barrier_contact){0};
    if (world == NULL || !dd2_barrier_vector_valid(&sweep.start) ||
        !dd2_barrier_vector_valid(&sweep.end) ||
        !dd2_barrier_number_valid(&sweep.radius, dd2_barrier_size_limit) || sweep.radius <= 0 ||
        !dd2_barrier_number_valid(&sweep.half_height, dd2_barrier_size_limit) ||
        sweep.half_height < 0) {
        return false;
    }
    if (dd2_barriers_radius(world->barriers) > 0) {
        return dd2_barrier_arena_sweep(dd2_barriers_radius(world->barriers), sweep, contact);
    }
    dd2_barrier_search search = {.sweep = sweep};
    const double start[] = {sweep.start.x, sweep.start.y, sweep.start.z};
    const double end[] = {sweep.end.x, sweep.end.y, sweep.end.z};
    for (size_t axis = 0; axis < DD2_BARRIER_AXES; ++axis) {
        const double radius = axis == 1 ? sweep.half_height : sweep.radius;
        search.bounds.min[axis] = fmin(start[axis], end[axis]) - radius;
        search.bounds.max[axis] = fmax(start[axis], end[axis]) + radius;
    }
    dd2_barrier_visit(world, &search);
    if (search.found) {
        search.selecting = true;
        search.found = false;
        dd2_barrier_visit(world, &search);
    }
    *contact = search.contact;
    if (statistics != NULL) {
        *statistics = search.statistics;
    }
    return search.found;
}
