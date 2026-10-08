#ifndef DD2_PHYSICS_BODY_SURFACE_H
#define DD2_PHYSICS_BODY_SURFACE_H

#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

enum { DD2_BODY_SURFACE_AXES = 3 };
typedef struct {
    dd2_vehicle_vector mount;
    dd2_vehicle_vector axis;
    double radius;
    double min_displacement;
    double max_displacement;
} dd2_body_wheel_query;
typedef struct {
    dd2_vehicle_vector center;
    dd2_vehicle_vector axes[DD2_BODY_SURFACE_AXES];
    dd2_vehicle_vector velocity;
    dd2_vehicle_vector angular_velocity;
} dd2_body_surface_box;
typedef struct {
    dd2_body_surface_box boxes[DD2_VEHICLE_FLEET_LIMIT];
    unsigned count;
} dd2_body_surface;
typedef struct {
    dd2_vehicle_vector point;
    dd2_vehicle_vector normal;
    dd2_vehicle_vector velocity;
    double maximum_height;
    double displacement;
    double minimum_displacement;
    unsigned body;
} dd2_body_surface_contact;

/* Owned, immutable source-sized box geometry and surface motion for one field
 * step. Preparation validates every body; failure clears the field. No pointers
 * or allocations are retained. */
bool dd2_body_surface_prepare(const dd2_vehicle *vehicles, unsigned count, dd2_body_surface *field);
/* Highest upward-facing finite box face within the inclusive vertical window,
 * excluding the querying body. Faces with normal Y below 0.2 cannot support a
 * tire. Ties within the existing road edge tolerance use the lowest body ID,
 * then source-axis order, anchored to the global maximum. Surface velocity
 * includes angular motion at the returned contact point. maximum_height retains
 * the global height before tie selection for merging with road contacts.
 * Clears on failure.
 * The field must come from successful preparation and remain unchanged. */
bool dd2_body_surface_sample(const dd2_body_surface *field, const dd2_surface_query *query,
                             unsigned exclude, dd2_body_surface_contact *contact);
/* Finite sphere/face contact along a unit suspension axis. The closest face in
 * the inclusive displacement window wins; tolerance ties retain body/axis order.
 * displacement is the plane intersection; negative values represent bounded
 * bump penetration with a fully compressed tire at the mount. Its point is
 * projected onto the finite face. minimum_displacement anchors global ties;
 * maximum_height applies only to vertical samples. No allocation. Clears on
 * invalid query or no hit. The field must remain successfully prepared. */
bool dd2_body_surface_wheel_sample(const dd2_body_surface *field, const dd2_body_wheel_query *query,
                                   unsigned exclude, dd2_body_surface_contact *contact);

#endif
