#include "game/starting_grid.h"

#include "assets/level.h"
#include "assets/road.h"
#include "physics/road_contact.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

enum {
    DD2_GRID_RACING_LEVELS = 7,
    DD2_GRID_LEVELS = 11,
    DD2_GRID_SPLIT = 8,
    DD2_GRID_MERGE = 9,
    DD2_GRID_HEADING_ZERO = 192,
    DD2_GRID_HEADINGS = 256
};
static const double dd2_grid_turn = 6.28318530717958647693;
static const double dd2_grid_ride_height = 190;
static const double dd2_grid_radius = 12000;
static const double dd2_grid_height_limit = 2147483648.0;

static uint32_t dd2_grid_strip(const dd2_road *road, uint32_t number) {
    const dd2_road_strip *strips = dd2_road_strips(road);
    const size_t count = dd2_road_strip_count(road);
    for (size_t index = 0; index < count; ++index) {
        if (strips[index].main_order == number) {
            return (uint32_t)index;
        }
    }
    uint32_t current = 0;
    uint32_t generated = (uint32_t)dd2_road_main_count(road);
    for (size_t visited = 0; visited < count; ++visited) {
        if (strips[current].kind == DD2_GRID_SPLIT) {
            current = strips[current].branch;
            for (size_t branch = 0; branch < count; ++branch) {
                if (strips[current].kind == DD2_GRID_MERGE) {
                    break;
                }
                if (generated++ == number) {
                    return current;
                }
                current = strips[current].next;
            }
        }
        current = strips[current].next;
        if (current == 0) {
            break;
        }
    }
    return DD2_ROAD_NO_STRIP;
}

static bool dd2_grid_racing(const dd2_road *road, unsigned level, unsigned slot,
                            dd2_grid_start *start) {
    static const uint32_t first[] = {266, 656, 535, 614, 22, 242};
    static const unsigned bases[] = {2, 1, 3, 2, 2, 2};
    static const int directions[] = {1, 1, -1, -1, 1, 1};
    static const uint32_t sca_numbers[DD2_VEHICLE_FLEET_LIMIT] = {449, 448, 447, 446, 445, 444, 443,
                                                                  442, 441, 440, 439, 438, 437, 436,
                                                                  435, 434, 573, 572, 571, 570};
    static const unsigned sca_lanes[DD2_VEHICLE_FLEET_LIMIT] = {1, 2, 1, 2, 1, 3, 2, 3, 3, 4,
                                                                3, 4, 4, 5, 4, 5, 1, 2, 1, 2};
    uint32_t strip = dd2_grid_strip(road, level == DD2_GRID_RACING_LEVELS ? sca_numbers[slot]
                                                                          : first[level - 1]);
    if (strip == DD2_ROAD_NO_STRIP) {
        return false;
    }
    if (level != DD2_GRID_RACING_LEVELS) {
        for (unsigned step = 0; step < slot; ++step) {
            strip = dd2_road_strips(road)[strip].previous;
        }
    }
    const dd2_road_strip geometry = dd2_road_strips(road)[strip];
    const unsigned lane =
        level == DD2_GRID_RACING_LEVELS
            ? sca_lanes[slot]
            : (unsigned)((int)bases[level - 1] + (directions[level - 1] * (int)(slot & 1U)));
    if (lane >= geometry.lanes) {
        return false;
    }
    const uint32_t cell = geometry.first_cell + lane;
    const dd2_road_cell *quad = &dd2_road_cells(road)[cell];
    const dd2_track_vertex *vertices = dd2_road_vertices(road);
    dd2_vehicle_spawn spawn = {0};
    for (size_t corner = 0; corner < DD2_ROAD_CORNERS; ++corner) {
        const dd2_track_vertex point = vertices[quad->vertices[corner]];
        spawn.position.x += (double)point.x / (double)DD2_ROAD_CORNERS;
        spawn.position.y += (double)point.y / (double)DD2_ROAD_CORNERS;
        spawn.position.z += (double)point.z / (double)DD2_ROAD_CORNERS;
    }
    const uint8_t heading = (uint8_t)(DD2_GRID_HEADING_ZERO - geometry.heading);
    spawn.yaw = (double)heading * dd2_grid_turn / (double)DD2_GRID_HEADINGS;
    spawn.position.y += dd2_grid_ride_height;
    *start = (dd2_grid_start){.spawn = spawn, .cell = cell};
    return true;
}

static bool dd2_grid_arena(const dd2_road_surface *surface, unsigned level, unsigned slot,
                           unsigned count, dd2_grid_start *start) {
    const double angle =
        level == DD2_GRID_LEVELS
            ? (dd2_grid_turn * (double)slot / (2 * (double)count)) - (dd2_grid_turn / 4)
            : dd2_grid_turn * (double)slot / (double)count;
    dd2_vehicle_spawn spawn = {
        .position = {.x = sin(angle) * dd2_grid_radius, .z = -cos(angle) * dd2_grid_radius},
        .yaw = -angle};
    /* Keep exact source first positions, including zero coordinates at grid edges. */
    if (slot == 0) {
        spawn.position = level == DD2_GRID_LEVELS ? (dd2_vehicle_vector){.x = -dd2_grid_radius}
                                                  : (dd2_vehicle_vector){.z = -dd2_grid_radius};
    }
    dd2_road_contact contact = {0};
    if (!dd2_road_surface_sample(
            surface,
            (dd2_surface_query){.point = {.x = spawn.position.x, .z = spawn.position.z},
                                .min_height = -dd2_grid_height_limit,
                                .max_height = dd2_grid_height_limit,
                                .preferred_cell = DD2_ROAD_NO_STRIP},
            &contact, NULL)) {
        return false;
    }
    spawn.position.y = contact.height + dd2_grid_ride_height;
    *start = (dd2_grid_start){.spawn = spawn, .cell = contact.cell};
    return true;
}

bool dd2_starting_grid(const dd2_road *road, const dd2_road_surface *surface, unsigned level,
                       unsigned count, dd2_grid_start *starts) {
    if (road == NULL || surface == NULL || starts == NULL || level == 0 ||
        level > DD2_GRID_LEVELS || count == 0 || count > DD2_VEHICLE_FLEET_LIMIT ||
        ((level <= DD2_GRID_RACING_LEVELS) != (dd2_road_strip_count(road) != 0))) {
        return false;
    }
    dd2_grid_start candidates[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned slot = 0; slot < count; ++slot) {
        if (!(level <= DD2_GRID_RACING_LEVELS
                  ? dd2_grid_racing(road, level, slot, &candidates[slot])
                  : dd2_grid_arena(surface, level, slot, count, &candidates[slot]))) {
            return false;
        }
    }
    for (unsigned slot = 0; slot < count; ++slot) {
        starts[slot] = candidates[slot];
    }
    return true;
}
