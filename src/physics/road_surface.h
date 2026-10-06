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

/* Owns a balanced XZ bounds hierarchy; borrows immutable road geometry, which
 * must outlive it. Creation/destruction are separate from allocation-free queries.
 * Returns the highest contact within the inclusive height window. Heights within
 * DD2_ROAD_EDGE_TOLERANCE form a tie: prefer the supplied cell, otherwise the
 * lowest cell index. Invalid/nonfinite queries fail and clear result/statistics.
 * This is surface selection, not suspension, wall collision or off-road recovery. */
dd2_road_surface *dd2_road_surface_create(const dd2_road *road);
void dd2_road_surface_destroy(dd2_road_surface *surface);
bool dd2_road_surface_sample(const dd2_road_surface *surface, dd2_surface_query query,
                             dd2_road_contact *result, dd2_surface_statistics *statistics);

#endif
