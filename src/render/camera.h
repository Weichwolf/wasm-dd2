#ifndef DD2_RENDER_CAMERA_H
#define DD2_RENDER_CAMERA_H

#include "assets/mesh.h"
#include "assets/model.h"
#include "assets/scene.h"
#include "assets/world.h"
#include "render/renderer.h"

#include <stdbool.h>

typedef struct {
    float center[3];
    float radius;
    float pitch;
    float yaw;
    float zoom;
    float pan_x;
    float pan_y;
} dd2_camera;

typedef struct {
    float yaw;
    float pitch;
    float zoom;
    float pan_x;
    float pan_y;
} dd2_camera_motion;

/* Orbit/inspection camera. Fitting resets navigation; empty geometry fails.
 * Motion axes are normalized finite values, seconds is finite and nonnegative.
 * This camera does not implement the driving/chase camera or simulation. */
bool dd2_camera_fit_mesh(dd2_camera *camera, const dd2_mesh *mesh);
bool dd2_camera_fit_scene(dd2_camera *camera, const dd2_scene *scene);
bool dd2_camera_fit_model(dd2_camera *camera, const dd2_model *model);
bool dd2_camera_fit_world(dd2_camera *camera, const dd2_world *world);
bool dd2_camera_step(dd2_camera *camera, dd2_camera_motion motion, float seconds);
void dd2_camera_apply(const dd2_camera *camera, dd2_render_options viewport);

#endif
