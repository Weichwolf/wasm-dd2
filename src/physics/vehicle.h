#ifndef DD2_PHYSICS_VEHICLE_H
#define DD2_PHYSICS_VEHICLE_H

#include "assets/road.h"
#include "physics/road_contact.h"
#include "physics/road_surface.h"

#include <stdbool.h>
#include <stdint.h>

enum { DD2_VEHICLE_WHEELS = 4 };
#define DD2_VEHICLE_STEP_SECONDS 0.005

typedef struct {
    double x;
    double y;
    double z;
} dd2_vehicle_vector;

typedef struct {
    double x;
    double y;
    double z;
    double w;
} dd2_vehicle_rotation;

typedef struct {
    double throttle; /* -1 reverse, +1 forward */
    double brake;    /* 0..1, opposes motion */
    double steer;    /* -1 left, +1 right */
} dd2_vehicle_control;

typedef struct {
    dd2_vehicle_vector position;
    double yaw;
} dd2_vehicle_spawn;

typedef enum {
    DD2_VEHICLE_WHEEL_NONE,
    DD2_VEHICLE_WHEEL_ROAD,
    DD2_VEHICLE_WHEEL_BODY
} dd2_vehicle_wheel_support;
typedef struct {
    dd2_vehicle_vector mount;
    dd2_vehicle_vector center;
    dd2_vehicle_vector point;
    /* Height/normal apply to every support; cell/triangle only to roads. */
    dd2_road_contact contact;
    double compression;
    double load; /* Normal force divided by body mass, world units/s^2. */
    dd2_vehicle_wheel_support support;
    unsigned body; /* Physical slot, meaningful only for BODY support. */
    bool grounded;
} dd2_vehicle_wheel;

typedef struct {
    dd2_vehicle_vector position;
    dd2_vehicle_vector velocity;
    dd2_vehicle_rotation rotation;
    dd2_vehicle_vector angular_velocity; /* World axes, radians/s. */
    double steering;                     /* Front wheel angle, radians. */
    dd2_vehicle_wheel wheels[DD2_VEHICLE_WHEELS];
    uint64_t steps;
} dd2_vehicle;

/* Original world coordinates: +Y up, local +Z forward, local +X right.
 * Wheel order: front right, rear right, front left, rear left. Road and index
 * must describe the same immutable geometry and outlive each step. No pointers
 * or allocations are retained in the vehicle. Caller advances exactly one fixed
 * step per call, independent of rendering/input event cadence.
 * Invalid/nonfinite controls or core state fail transactionally. Wheel outputs
 * describe contacts/forces at the beginning of the last successful step.
 * This implements a tunable four-wheel rigid body, not original fixed-point
 * parity. Body/wall/car collisions, damage, detached wheels and recovery remain
 * separate work; an inverted body has no tire support. */
bool dd2_vehicle_valid(const dd2_vehicle *vehicle);
/* Inverse world inertia divided by mass, for collision impulses. Requires a
 * validated rotation; torque is an impulse moment divided by body mass. */
dd2_vehicle_vector dd2_vehicle_angular_response(dd2_vehicle_rotation rotation,
                                                dd2_vehicle_vector torque);
bool dd2_vehicle_reset(dd2_vehicle *vehicle, dd2_vehicle_spawn spawn);
/* Refresh the wheel geometry/support after a discontinuous pose change without
 * advancing body motion, steering or the fixed-step counter. Invalid core state
 * or missing geometry preserves every wheel. Road and surface must match. */
bool dd2_vehicle_refresh_wheels(dd2_vehicle *vehicle, const dd2_road *road,
                                const dd2_road_surface *surface);
bool dd2_vehicle_step(dd2_vehicle *vehicle, const dd2_road *road, const dd2_road_surface *surface,
                      dd2_vehicle_control control);
typedef struct {
    const dd2_road *road;
    const dd2_road_surface *surface;
    const dd2_vehicle_control *controls;
    unsigned count;
} dd2_vehicle_field_step;
/* One simultaneous field step, including tires supported by other bodies.
 * All observations use the immutable initial field. Body support exchanges
 * equal/opposite force at one common contact point. No allocation or retained
 * pointers; invalid input/output preserves every body. Count is 1..20; controls
 * and geometry are borrowed and must not overlap writable vehicle storage.
 * Body collisions remain a separate call after this prediction. */
bool dd2_vehicle_step_field(dd2_vehicle *vehicles, const dd2_vehicle_field_step *step);
dd2_vehicle_vector dd2_vehicle_rotate(dd2_vehicle_rotation rotation, dd2_vehicle_vector vector);

#endif
