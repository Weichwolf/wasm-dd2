#ifndef DD2_RENDER_DAMAGE_DRAW_H
#define DD2_RENDER_DAMAGE_DRAW_H

#include "physics/damage.h"
#include "render/renderer.h"

#include <stdbool.h>

/* Six colored zones of a forward-pointing car icon and an engine-health bar.
 * Borrows validated state; overlays after the world draw and restores matrices
 * and depth testing. No asset/font allocations or mutation. */
bool dd2_damage_draw(const dd2_vehicle_damage *damage, dd2_render_options viewport);

#endif
