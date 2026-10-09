#include "render/model_view.h"

#include "render/model_draw.h"
#include "render/renderer.h"

#include <GL/softgl.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

static const float dd2_model_view_radians = 0.017453292519943295F;
static const float dd2_model_view_yaw = 35;
static const float dd2_model_view_pitch = 16;
static const float dd2_model_view_distance = 7.2F;
static const float dd2_model_view_target_height = 0.65F;
static const float dd2_model_view_cockpit_height = 1.19F;
static const float dd2_model_view_cockpit_left = -0.4F;
static const float dd2_model_view_cockpit_back = -0.3F;
static const float dd2_model_view_min_distance = 3.5F;
static const float dd2_model_view_max_distance = 25;
static const float dd2_model_view_max_pitch = 75;
static const float dd2_model_view_turn_rate = 75;
static const float dd2_model_view_full_turn = 360;
static const double dd2_model_view_near = 0.025;
static const double dd2_model_view_far = 120;
static const double dd2_model_view_exterior_tangent = 0.414213562373095;
static const double dd2_model_view_cockpit_tangent = 0.637070260807493;
static const float dd2_model_view_sky[] = {0.17F, 0.23F, 0.31F, 1};

dd2_model_view dd2_model_view_default(bool cockpit) {
    return (dd2_model_view){.yaw = cockpit ? 0 : dd2_model_view_yaw,
                            .pitch = cockpit ? 0 : dd2_model_view_pitch,
                            .distance = dd2_model_view_distance,
                            .cockpit = cockpit};
}

bool dd2_model_view_step(dd2_model_view *view, dd2_model_view_motion motion, float seconds) {
    if (view == NULL || seconds <= 0 || seconds > 1 ||
        (motion.yaw == 0 && motion.pitch == 0 && motion.zoom == 0)) {
        return false;
    }
    view->yaw = fmodf(view->yaw + (motion.yaw * seconds * dd2_model_view_turn_rate),
                      dd2_model_view_full_turn);
    view->pitch = fminf(dd2_model_view_max_pitch,
                        fmaxf(-dd2_model_view_max_pitch,
                              view->pitch + (motion.pitch * seconds * dd2_model_view_turn_rate)));
    if (!view->cockpit) {
        view->distance =
            fminf(dd2_model_view_max_distance,
                  fmaxf(dd2_model_view_min_distance, view->distance * expf(motion.zoom * seconds)));
    }
    return true;
}

static void dd2_model_view_matrix(const float *eye, const float *forward) {
    const float horizontal = hypotf(forward[0], forward[2]);
    const float right[] = {forward[2] / horizontal, 0, -forward[0] / horizontal};
    const float up_axis[] = {-forward[1] * forward[0] / horizontal, horizontal,
                             -forward[1] * forward[2] / horizontal};
    const float matrix[] = {right[0],
                            up_axis[0],
                            -forward[0],
                            0,
                            right[1],
                            up_axis[1],
                            -forward[1],
                            0,
                            right[2],
                            up_axis[2],
                            -forward[2],
                            0,
                            -(right[0] * eye[0]) - (right[2] * eye[2]),
                            -(up_axis[0] * eye[0]) - (up_axis[1] * eye[1]) - (up_axis[2] * eye[2]),
                            (forward[0] * eye[0]) + (forward[1] * eye[1]) + (forward[2] * eye[2]),
                            1};
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(matrix);
}

dd2_model_draw_options dd2_model_view_apply(const dd2_model_view *view,
                                            dd2_render_options viewport) {
    dd2_model_draw_options options = {.lighting = true, .clockwise_front = true};
    if (view == NULL || viewport.width <= 0 || viewport.height <= 0) {
        return options;
    }
    const float yaw = view->yaw * dd2_model_view_radians;
    const float pitch = view->pitch * dd2_model_view_radians;
    float forward[] = {sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch)};
    if (view->cockpit) {
        options.eye[0] = dd2_model_view_cockpit_left;
        options.eye[1] = dd2_model_view_cockpit_height;
        options.eye[2] = dd2_model_view_cockpit_back;
    } else {
        for (size_t axis = 0; axis < sizeof(forward) / sizeof(forward[0]); ++axis) {
            options.eye[axis] = forward[axis] * view->distance;
            forward[axis] = -forward[axis];
        }
        options.eye[1] += dd2_model_view_target_height;
    }
    const double top = dd2_model_view_near * (view->cockpit ? dd2_model_view_cockpit_tangent
                                                            : dd2_model_view_exterior_tangent);
    const double aspect = (double)viewport.width / (double)viewport.height;
    glViewport(0, 0, viewport.width, viewport.height);
    glDepthMask(GL_TRUE);
    glClearColor(dd2_model_view_sky[0], dd2_model_view_sky[1], dd2_model_view_sky[2],
                 dd2_model_view_sky[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-top * aspect, top * aspect, -top, top, dd2_model_view_near, dd2_model_view_far);
    dd2_model_view_matrix(options.eye, forward);
    return options;
}
