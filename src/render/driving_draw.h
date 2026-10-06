#ifndef DD2_RENDER_DRIVING_DRAW_H
#define DD2_RENDER_DRIVING_DRAW_H

#include "assets/track.h"
#include "physics/damage.h"
#include "physics/vehicle.h"
#include "render/mesh_draw.h"
#include "render/renderer.h"

#include <stdbool.h>

/* Borrows a validated simulation state and track. Uses a world-up chase camera,
 * full body orientation and four sprung, steered, rolling wheel models. */
typedef struct {
    const dd2_vehicle *vehicle;
    const dd2_vehicle_damage *damage;
    double wheel_roll;
    const dd2_vehicle *opponents;
    const dd2_vehicle_damage *opponent_damage;
    const double *opponent_rolls;
    unsigned opponent_count;
    dd2_render_options viewport;
} dd2_driving_view;
bool dd2_driving_draw(dd2_mesh_materials *materials, const dd2_track *track, dd2_driving_view view);

#endif
