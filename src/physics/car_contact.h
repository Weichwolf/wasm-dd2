#ifndef DD2_PHYSICS_CAR_CONTACT_H
#define DD2_PHYSICS_CAR_CONTACT_H

#include "physics/vehicle.h"

#include <stdbool.h>

typedef struct {
    double time;
    double penetration;
    dd2_vehicle_vector normal; /* From second toward first. */
    dd2_vehicle_vector point;
    bool unresolved; /* Conservative time bound; no physical point or impulse. */
} dd2_car_contact;

/* Source-sized oriented boxes, including roll/pitch and separate bridge heights.
 * Continuous translation/rotation with conservative endpoint envelopes for
 * rotating SAT axes, refined until an actual-pose SAT check confirms contact.
 * Initial angular pieces target 0.01 rad, capped at 64. Each query uses at most
 * 512 refinement windows, at most 48 levels deep. Exhaustion returns true with
 * unresolved set and an earliest possible time; the caller must stop there
 * without applying an impulse. This is a body proxy, not mesh collision.
 * Clears output on no contact or invalid/nonfinite state. No allocations.
 * Equal-time axis ties use fixed source-axis order. Relative normal travel
 * within 1e-6 world units per sweep is stationary for contact selection; a
 * touching pair must close beyond that tolerance to create an event. */
bool dd2_car_contact_sweep(const dd2_vehicle *first_start, const dd2_vehicle *first_end,
                           const dd2_vehicle *second_start, const dd2_vehicle *second_end,
                           dd2_car_contact *contact);
#endif
