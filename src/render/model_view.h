#ifndef DD2_RENDER_MODEL_VIEW_H
#define DD2_RENDER_MODEL_VIEW_H

#include "render/model_draw.h"
#include "render/renderer.h"

#include <stdbool.h>

typedef struct {
    float yaw;
    float pitch;
    float distance;
    bool cockpit;
} dd2_model_view;

typedef struct {
    float yaw;
    float pitch;
    float zoom;
} dd2_model_view_motion;

dd2_model_view dd2_model_view_default(bool cockpit);
bool dd2_model_view_step(dd2_model_view *view, dd2_model_view_motion motion, float seconds);
/* Meter-space perspective camera with +Z forward/+X right in the cockpit.
 * Its reflected view basis requires clockwise front faces; returned draw options
 * preserve that convention and carry the model-local eye for transparent sorting. */
dd2_model_draw_options dd2_model_view_apply(const dd2_model_view *view,
                                            dd2_render_options viewport);

#endif
