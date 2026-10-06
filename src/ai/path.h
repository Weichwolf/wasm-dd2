#ifndef DD2_AI_PATH_H
#define DD2_AI_PATH_H

#include "assets/road.h"
#include "physics/vehicle.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t cell;
    double lane; /* Normalized lateral position, 0..1. */
    dd2_vehicle_vector position;
    double distance; /* Horizontal lookahead, 0..16000 world units. */
} dd2_ai_path_query;
typedef struct {
    dd2_vehicle_vector point;
    dd2_vehicle_vector direction;
    double width;
    double curvature;
    uint32_t strip;
} dd2_ai_path_sample;

/* Follow owned road next-links, including alternate branches and loop closure.
 * Lane centers interpolate across changing widths; partial edge cells use their
 * active triangle centroid. Projection/lookahead use horizontal arc length.
 * No allocations or retained pointers. Invalid input/degenerate paths clear output. */
bool dd2_ai_path(const dd2_road *road, dd2_ai_path_query query, dd2_ai_path_sample *sample);

#endif
