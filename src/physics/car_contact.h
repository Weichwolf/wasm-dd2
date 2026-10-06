#ifndef DD2_PHYSICS_CAR_CONTACT_H
#define DD2_PHYSICS_CAR_CONTACT_H

#include "physics/vehicle.h"

#include <stdbool.h>

typedef struct {
    double time;
    double penetration;
    dd2_vehicle_vector normal; /* From second toward first. */
    dd2_vehicle_vector point;
} dd2_car_contact;

/* Source-sized oriented boxes, including roll/pitch and separate bridge heights.
 * Exact continuous translation per angular piece; rotation uses conservative
 * envelopes around the midpoint orientation, at most 0.01 rad per piece for
 * normal fixed-step motion. This is a body proxy, not mesh collision.
 * Clears output on no contact or invalid/nonfinite state. No allocations.
 * Equal-time axis ties use fixed source-axis order. Relative normal travel
 * within 1e-6 world units per sweep is stationary for contact selection; a
 * touching pair must close beyond that tolerance to create an event. */
bool dd2_car_contact_sweep(const dd2_vehicle *first_start, const dd2_vehicle *first_end,
                           const dd2_vehicle *second_start, const dd2_vehicle *second_end,
                           dd2_car_contact *contact);
#endif
