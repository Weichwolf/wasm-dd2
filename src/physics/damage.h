#ifndef DD2_PHYSICS_DAMAGE_H
#define DD2_PHYSICS_DAMAGE_H

#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    DD2_DAMAGE_FRONT_LEFT,
    DD2_DAMAGE_FRONT_RIGHT,
    DD2_DAMAGE_SIDE_LEFT,
    DD2_DAMAGE_SIDE_RIGHT,
    DD2_DAMAGE_REAR_LEFT,
    DD2_DAMAGE_REAR_RIGHT,
    DD2_DAMAGE_REGIONS
} dd2_damage_region;

typedef struct {
    double regions[DD2_DAMAGE_REGIONS]; /* Local crush, 0 intact .. 1 exhausted. */
    uint64_t steps;
    bool retired;
} dd2_vehicle_damage;

typedef struct {
    const dd2_vehicle_collision_report *contacts;
    unsigned count;
} dd2_damage_frame;

/* Typed source-inspired six-zone damage. Caller owns count (1..20) states.
 * Consume one complete contact report inside each 5 ms simulation transaction.
 * Normal impulse and closing speed thresholds exclude support/repair chatter.
 * Each region receives its strongest weighted crush load of this fixed step,
 * rather than counting solver corrections as independent accidents. Either
 * exhausted front region retires the engine. Rear/side crush does not do so.
 * Validation/counter overflow preserves every state. No retained pointers.
 * Thresholds and power/deformation curves are rewrite tuning, not fixed-point
 * original parity. Wheels, detached panels and accident scores are separate. */
bool dd2_damage_valid(const dd2_vehicle_damage *damage);
bool dd2_damage_step(dd2_vehicle_damage *damage, dd2_damage_frame frame);

/* Require validated damage/control and finite local mesh point. Intact damage
 * passes controls/points through exactly. Retired cars brake and stay physical.
 * Deformation returns a body-local point without changing shared asset meshes. */
double dd2_damage_health(const dd2_vehicle_damage *damage);
dd2_vehicle_control dd2_damage_control(const dd2_vehicle_damage *damage,
                                       dd2_vehicle_control control);
dd2_vehicle_vector dd2_damage_deform(const dd2_vehicle_damage *damage, dd2_vehicle_vector point);

#endif
