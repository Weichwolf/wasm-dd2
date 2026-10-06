#ifndef DD2_PHYSICS_BARRIER_WORLD_H
#define DD2_PHYSICS_BARRIER_WORLD_H

#include "assets/barriers.h"
#include "physics/vehicle.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct dd2_barrier_world dd2_barrier_world;
typedef struct {
    dd2_vehicle_vector start;
    dd2_vehicle_vector end;
    double radius;
    double half_height;
} dd2_barrier_sweep;
typedef struct {
    double time;
    double penetration;
    dd2_vehicle_vector point;
    dd2_vehicle_vector normal;
    uint32_t barrier;
} dd2_barrier_contact;
typedef struct {
    size_t bounds_tests;
    size_t segment_tests;
} dd2_barrier_statistics;

/* Owns a balanced index; borrows barriers until destruction. Sweeps a rounded
 * horizontal footprint and vertical interval against source segments or the
 * analytic arena circle. Road walls have a tuned 400-unit height, avoiding
 * interaction with another bridge level. Queries allocate nothing, select the
 * earliest time and use stable source-ID ties. Initial overlaps report depth;
 * touching/escaping motion is ignored. Invalid input clears output and fails. */
dd2_barrier_world *dd2_barrier_world_create(const dd2_barriers *barriers);
void dd2_barrier_world_destroy(dd2_barrier_world *world);
bool dd2_barrier_world_sweep(const dd2_barrier_world *world, dd2_barrier_sweep sweep,
                             dd2_barrier_contact *contact, dd2_barrier_statistics *statistics);

#endif
