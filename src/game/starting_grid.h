#ifndef DD2_GAME_STARTING_GRID_H
#define DD2_GAME_STARTING_GRID_H
#include "assets/road.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    dd2_vehicle_spawn spawn;
    uint32_t cell;
} dd2_grid_start;
/* Original grid slot layout; championship driver-to-slot ordering is separate.
 * Racing grids walk previous strips, with source lane alternation and the SCA
 * special grid. Arenas use the source ring/half-ring, with continuous trig.
 * All starts retain the nominal source ride height; the game settles bodies.
 * Failure preserves output. Borrows immutable road/index, retains no pointers. */
bool dd2_starting_grid(const dd2_road *road, const dd2_road_surface *surface, unsigned level,
                       unsigned count, dd2_grid_start *starts);
#endif
