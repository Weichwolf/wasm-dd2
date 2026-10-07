#ifndef DD2_RENDER_RACE_DRAW_H
#define DD2_RENDER_RACE_DRAW_H

#include "game/championship.h"
#include "game/race.h"
#include "render/renderer.h"

#include <stdbool.h>

/* Race position, source-timed start lights and frozen twenty-driver results.
 * Draw after the world/HUD. No retained state; restores matrices and depth. */
bool dd2_race_draw(const dd2_race *race, dd2_render_options viewport);
/* Actual season metadata and stable-ID standings. Draw after the race HUD;
 * interstitial/terminal screens show the four complete league divisions. */
bool dd2_championship_draw(const dd2_championship *championship, dd2_render_options viewport);

#endif
