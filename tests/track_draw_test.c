#include "assets/bytes.h"
#include "assets/car_class.h"
#include "assets/image.h"
#include "assets/track.h"
#include "image_fixture.h"
#include "physics/vehicle.h"
#include "render/camera.h"
#include "render/driving_draw.h"
#include "render/renderer.h"
#include "render/track_draw.h"
#include "track_fixture.h"

#include <GL/softgl.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

enum {
    DD2_TRACK_DRAW_TEST_WIDTH = 160,
    DD2_TRACK_DRAW_TEST_HEIGHT = 90,
    DD2_TRACK_DRAW_TEST_MATRIX = 16,
    DD2_TRACK_DRAW_TEST_OPPONENTS = 3
};
static const double dd2_track_draw_test_units = 160;
static const float dd2_track_draw_test_tolerance = 0.0001F;
static const float dd2_track_draw_test_relative = 0.000001F;
static const double dd2_track_draw_test_medium = 6400;
static const double dd2_track_draw_test_distant = 16000;

typedef struct {
    unsigned calls;
} dd2_track_draw_test_images;
static dd2_image *dd2_track_draw_test_image(void *user, const char *resource) {
    dd2_track_draw_test_images *images = user;
    if (resource == NULL) {
        return NULL;
    }
    ++images->calls;
    const dd2_png_test_fixture *fixture = &dd2_png_test_fixtures[0];
    return dd2_image_create_png((dd2_byte_view){fixture->encoded, fixture->length});
}

static bool dd2_track_draw_test_camera(dd2_driving_view view) {
    float reference[DD2_TRACK_DRAW_TEST_MATRIX] = {0};
    float prepared[DD2_TRACK_DRAW_TEST_MATRIX] = {0};
    float reference_projection[DD2_TRACK_DRAW_TEST_MATRIX] = {0};
    float prepared_projection[DD2_TRACK_DRAW_TEST_MATRIX] = {0};
    const dd2_vehicle_vector original = dd2_driving_camera_apply(view, 1);
    glGetFloatv(GL_MODELVIEW_MATRIX, reference);
    glGetFloatv(GL_PROJECTION_MATRIX, reference_projection);
    const dd2_vehicle_vector scaled = dd2_driving_camera_apply(view, dd2_track_draw_test_units);
    glGetFloatv(GL_MODELVIEW_MATRIX, prepared);
    glGetFloatv(GL_PROJECTION_MATRIX, prepared_projection);
    const size_t translation_begin = 12;
    const size_t translation_end = 15;
    const size_t projection_translation = 14;
    for (size_t index = 0; index < DD2_TRACK_DRAW_TEST_MATRIX; ++index) {
        const float matrix_scale = index >= translation_begin && index < translation_end
                                       ? (float)dd2_track_draw_test_units
                                       : 1;
        const float projection_scale =
            index == projection_translation ? (float)dd2_track_draw_test_units : 1;
        if (fabsf(reference[index] - (prepared[index] * matrix_scale)) >
                (dd2_track_draw_test_relative * (1 + fabsf(reference[index]))) ||
            fabsf(reference_projection[index] - (prepared_projection[index] * projection_scale)) >
                (dd2_track_draw_test_relative * (1 + fabsf(reference_projection[index])))) {
            return false;
        }
    }
    return fabs(original.x - (scaled.x * dd2_track_draw_test_units)) <
               dd2_track_draw_test_tolerance &&
           fabs(original.y - (scaled.y * dd2_track_draw_test_units)) <
               dd2_track_draw_test_tolerance &&
           fabs(original.z - (scaled.z * dd2_track_draw_test_units)) <
               dd2_track_draw_test_tolerance;
}

static bool dd2_track_draw_test_frames(dd2_track_draw *draw, dd2_track_draw_test_images *images) {
    dd2_vehicle player = {0};
    dd2_vehicle opponents[DD2_TRACK_DRAW_TEST_OPPONENTS] = {0};
    const double positions[] = {dd2_track_draw_test_medium, dd2_track_draw_test_distant,
                                -dd2_track_draw_test_distant};
    double rolls[DD2_TRACK_DRAW_TEST_OPPONENTS] = {0};
    if (!dd2_vehicle_reset(&player, (dd2_vehicle_spawn){0})) {
        return false;
    }
    for (size_t index = 0; index < DD2_TRACK_DRAW_TEST_OPPONENTS; ++index) {
        if (!dd2_vehicle_reset(&opponents[index],
                               (dd2_vehicle_spawn){.position = {.z = positions[index]}})) {
            return false;
        }
    }
    dd2_driving_view view = {
        .vehicle = &player,
        .opponents = opponents,
        .opponent_rolls = rolls,
        .opponent_count = DD2_TRACK_DRAW_TEST_OPPONENTS,
        .viewport = {.width = DD2_TRACK_DRAW_TEST_WIDTH, .height = DD2_TRACK_DRAW_TEST_HEIGHT}};
    if (!dd2_track_draw_test_camera(view) || !dd2_track_draw_driving(draw, view)) {
        return false;
    }
    const dd2_track_draw_stats stats = dd2_track_draw_statistics(draw);
    const size_t visible_models = 15;
    const size_t hidden_models = 5;
    if (stats.vehicle_models != visible_models || stats.culled_models != hidden_models ||
        stats.vehicle_triangles != visible_models || stats.body_lods[0] != 1 ||
        stats.body_lods[1] != 1 || stats.body_lods[2] != 1 || stats.uploaded_textures != 1 ||
        images->calls != 1 || !dd2_track_draw_driving(draw, view) || images->calls != 1) {
        return false;
    }
    opponents[0].rotation = (dd2_vehicle_rotation){0};
    return !dd2_track_draw_driving(draw, view) && images->calls == 1 && glGetError() == GL_NO_ERROR;
}

int main(void) {
    dd2_track *track = dd2_track_test_load(NULL, DD2_TRACK_TEST_ARENA);
    dd2_renderer *renderer = dd2_renderer_create(&(dd2_render_options){
        .width = DD2_TRACK_DRAW_TEST_WIDTH, .height = DD2_TRACK_DRAW_TEST_HEIGHT});
    dd2_track_draw_test_images images = {0};
    dd2_track_draw *draw =
        renderer != NULL ? dd2_track_draw_create(track, dd2_track_draw_test_image, &images) : NULL;
    dd2_camera camera = {0};
    bool passed = draw != NULL && images.calls == 0 && dd2_track_draw_fit(track, &camera, true) &&
                  dd2_track_draw_inspect(draw, &camera, true,
                                         (dd2_render_options){.width = DD2_TRACK_DRAW_TEST_WIDTH,
                                                              .height = DD2_TRACK_DRAW_TEST_HEIGHT},
                                         DD2_CAR_ROOKIE) &&
                  images.calls == 1 && dd2_track_draw_test_frames(draw, &images);
    passed = passed && dd2_track_draw_create(NULL, NULL, NULL) == NULL &&
             !dd2_track_draw_fit(NULL, &camera, false) &&
             dd2_track_draw_statistics(NULL).vehicle_models == 0;
    dd2_track_draw_destroy(draw);
    dd2_track_destroy(track);
    dd2_renderer_destroy(renderer);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
