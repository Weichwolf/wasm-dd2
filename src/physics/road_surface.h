#ifndef DD2_PHYSICS_ROAD_SURFACE_H
#define DD2_PHYSICS_ROAD_SURFACE_H

#include "assets/road.h"
#include "physics/road_contact.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct dd2_road_surface dd2_road_surface;

typedef struct {
    dd2_road_point point;
    double min_height;
    double max_height;
    uint32_t preferred_cell;
} dd2_surface_query;

typedef struct {
    size_t bounds_tests;
    size_t cell_tests;
} dd2_surface_statistics;

typedef struct {
    double x;
    double y;
    double z;
} dd2_road_position;
typedef struct {
    dd2_road_position start;
    dd2_road_position end;
    double recovery; /* Maximum initial overlap, measured along the normal. */
} dd2_surface_sweep;
typedef struct {
    double time;
    double penetration;
    dd2_road_position point;
    dd2_road_contact road;
} dd2_surface_hit;

/* Owns a balanced XZ bounds hierarchy and prepared sweep planes; borrows
 * immutable road geometry, which
 * must outlive it. Creation/destruction are separate from allocation-free queries.
 * Returns the highest contact within the inclusive height window. Heights within
 * DD2_ROAD_EDGE_TOLERANCE form a tie: prefer the supplied cell, otherwise the
 * lowest cell index. Invalid/nonfinite queries fail and clear result/statistics.
 * This is surface selection, not suspension, wall collision or off-road recovery. */
dd2_road_surface *dd2_road_surface_create(const dd2_road *road);
void dd2_road_surface_destroy(dd2_road_surface *surface);
bool dd2_road_surface_sample(const dd2_road_surface *surface, dd2_surface_query query,
                             dd2_road_contact *result, dd2_surface_statistics *statistics);

/* One-sided point/triangle sweep. Finds the earliest downward crossing, also
 * when both endpoints are outside a triangle. Repairs bounded initial overlaps;
 * deeper surfaces above the start are ignored, preserving bridge separation.
 * Earliest-time ties prefer lowest cell, then triangle. Invalid inputs clear
 * outputs. No allocations; the road must remain immutable. */
bool dd2_road_surface_sweep(const dd2_road_surface *surface, dd2_surface_sweep sweep,
                            dd2_surface_hit *result, dd2_surface_statistics *statistics);

#endif
