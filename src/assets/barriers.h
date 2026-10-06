#ifndef DD2_ASSETS_BARRIERS_H
#define DD2_ASSETS_BARRIERS_H

#include "assets/level.h"
#include "assets/road.h"

#include <stddef.h>
#include <stdint.h>

typedef struct dd2_barriers dd2_barriers;
typedef struct {
    dd2_track_vertex start;
    dd2_track_vertex end;
    uint32_t strip;
    unsigned side;
} dd2_barrier_segment;

/* Owns copied source collision lines, independent of road lifetime. Directed
 * lines have their road interior on the (dz,-dx) side. Arenas retain an analytic
 * circle rather than replacing the original radius by a polygon or grid edge. */
dd2_barriers *dd2_barriers_create(const dd2_road *road, unsigned level);
void dd2_barriers_destroy(dd2_barriers *barriers);
size_t dd2_barriers_count(const dd2_barriers *barriers);
const dd2_barrier_segment *dd2_barriers_segments(const dd2_barriers *barriers);
double dd2_barriers_radius(const dd2_barriers *barriers);

#endif
