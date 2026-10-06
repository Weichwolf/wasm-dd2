#ifndef DD2_RENDER_RACE_DRAW_H
#define DD2_RENDER_RACE_DRAW_H

#include "game/race.h"
#include "render/renderer.h"

#include <stdbool.h>

/* Race position, source-timed start lights and frozen twenty-driver results.
 * Draw after the world/HUD. No retained state; restores matrices and depth. */
bool dd2_race_draw(const dd2_race *race, dd2_render_options viewport);

#endif
