#ifndef DD2_PHYSICS_ROAD_CONTACT_H
#define DD2_PHYSICS_ROAD_CONTACT_H

#include "assets/road.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    double x;
    double z;
} dd2_road_point;

typedef struct {
    double height;
    double normal[3];
    uint32_t cell;
    unsigned triangle;
} dd2_road_contact;

/* Shared numerical edge distance for contact tests and conservative search bounds. */
#define DD2_ROAD_EDGE_TOLERANCE 1e-6

/* Vertical contact with one known lane cell. Inclusive edges, no extrapolation;
 * missing/degenerate triangles fail, and failure clears result. Finite inputs
 * in world coordinates are required. Normal is unit length and positive Y.
 * Searching, wheel suspension and off-road recovery belong to the vehicle step. */
bool dd2_road_contact_cell(const dd2_road *road, size_t cell, dd2_road_point point,
                           dd2_road_contact *result);
/* Same contact primitive for a specific active triangle (0 or 1). */
bool dd2_road_contact_triangle(const dd2_road *road, size_t cell, unsigned triangle,
                               dd2_road_point point, dd2_road_contact *result);

#endif
