#ifndef DD2_RENDER_SCORE_DRAW_H
#define DD2_RENDER_SCORE_DRAW_H

#include "game/accidents.h"
#include "render/renderer.h"

#include <stdbool.h>

/* Player accident points (PTS) and credited destructions (KO). No allocation or
 * asset mutation. Overlay after world rendering; restores matrices and depth. */
bool dd2_score_draw(const dd2_accident_driver *score, dd2_render_options viewport);

#endif
