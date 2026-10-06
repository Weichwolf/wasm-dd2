#ifndef DD2_RENDER_SCORE_DRAW_H
#define DD2_RENDER_SCORE_DRAW_H

#include "game/accidents.h"
#include "game/laps.h"
#include "render/renderer.h"

#include <stdbool.h>

/* Player accident points (PTS) and credited destructions (KO). No allocation or
 * asset mutation. Overlay after world rendering; restores matrices and depth. */
bool dd2_score_draw(const dd2_accident_driver *score, dd2_render_options viewport);
/* Current/required lap in the upper left; FIN when this driver's laps finish.
 * The partial grid approach displays lap 1. Arenas omit this overlay. */
bool dd2_lap_draw(const dd2_lap_driver *lap, unsigned required, dd2_render_options viewport);

#endif
