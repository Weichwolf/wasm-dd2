#ifndef DD2_ASSETS_ROAD_H
#define DD2_ASSETS_ROAD_H

#include "assets/level.h"

#include <stddef.h>
#include <stdint.h>

enum { DD2_ROAD_CORNERS = 4 };
#define DD2_ROAD_NO_STRIP UINT32_MAX

typedef enum { DD2_ROAD_RACING, DD2_ROAD_ARENA } dd2_road_layout;
typedef struct dd2_road dd2_road;
enum { DD2_ROAD_UNITS_PER_METER = 160 };

typedef struct {
    uint32_t source_offset;
    uint32_t next;
    uint32_t previous;
    uint32_t branch;
    uint32_t first_cell;
    uint32_t main_order;
    uint16_t source_number;
    uint16_t first_vertex;
    uint16_t flags;
    uint8_t kind;
    uint8_t lanes;
    uint8_t source_lane_start;
    uint8_t heading;
} dd2_road_strip;

typedef struct {
    /* Quad order A, A+1, B+1, B. Triangle 0 uses corners 0,1,3;
     * triangle 1 uses 2,3,1. Type-specific missing edge triangles are masked. */
    uint32_t vertices[DD2_ROAD_CORNERS];
    uint32_t strip;
    uint32_t lane;
    uint8_t surface_flags;
    uint8_t heading;
    uint8_t triangle_mask;
} dd2_road_cell;

/* Owns decoded vertices, strips and lane cells. Original bytes may be released
 * after creation. Links are bounded indices, never original memory addresses.
 * The main loop starts at strip 0. game/course supplies separate branch/main
 * lap equivalence; original generated lane-start fields remain unimplemented. */
dd2_road *dd2_road_create(const dd2_level_data *level, dd2_road_layout layout);
/* Reads our DD2ROAD1 container, without original level/archive input. Geometry
 * uses signed fixed positions at 160 units per meter, matching the current
 * physics rules. Copies vertices and indexed topology; source bytes may be
 * released immediately. Original source/provenance fields are zero. */
dd2_road *dd2_road_create_prepared(dd2_byte_view bytes);
void dd2_road_destroy(dd2_road *road);
size_t dd2_road_vertex_count(const dd2_road *road);
const dd2_track_vertex *dd2_road_vertices(const dd2_road *road);
size_t dd2_road_strip_count(const dd2_road *road);
const dd2_road_strip *dd2_road_strips(const dd2_road *road);
size_t dd2_road_main_count(const dd2_road *road);
size_t dd2_road_cell_count(const dd2_road *road);
const dd2_road_cell *dd2_road_cells(const dd2_road *road);

#endif
