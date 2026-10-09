#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/image.h"
#include "assets/model.h"
#include "assets/track.h"
#include "model_fixture.h"
#include "render/frustum.h"
#include "render/model_draw.h"
#include "render/renderer.h"
#include "render/sky_draw.h"
#include "track_fixture.h"

#include <GL/softgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum { DD2_SKY_TEST_SIDE = 64, DD2_SKY_TEST_CHANNELS = 4, DD2_SKY_TEST_SAMPLE = 40 };
static const float dd2_sky_test_rotation = 180;
static const float dd2_sky_test_translation = 128;

static dd2_model *dd2_sky_test_model(void *user, const char *path) {
    (void)user;
    (void)path;
    uint8_t bytes[DD2_MODEL_TEST_BYTES] = {0};
    dd2_model_test_source(bytes);
    dd2_test_write_le32(bytes + DD2_MODEL_TEST_TEXTURE_INDEX, DD2_MODEL_NO_TEXTURE);
    dd2_test_write_le32(bytes + DD2_MODEL_TEST_COLOR, 0);
    dd2_test_write_le32(bytes + DD2_MODEL_TEST_COLOR + DD2_MODEL_TEST_WORD, 0);
    enum { DD2_SKY_TEST_TEXTURE_BYTES = DD2_MODEL_TEXTURE_MAPS * DD2_MODEL_NAME_BYTES };
    uint8_t compact[DD2_MODEL_TEST_BYTES - DD2_SKY_TEST_TEXTURE_BYTES] = {0};
    for (size_t index = 0; index < sizeof(compact); ++index) {
        compact[index] =
            bytes[index < DD2_MODEL_TEST_TEXTURE ? index : index + DD2_SKY_TEST_TEXTURE_BYTES];
    }
    const size_t texture_count = 8;
    dd2_test_write_le32(compact + texture_count, 0);
    return dd2_model_create((dd2_byte_view){compact, sizeof(compact)});
}

static dd2_image *dd2_sky_test_no_image(void *user, const char *path) {
    (void)user;
    (void)path;
    return NULL;
}

static void dd2_sky_test_view(void) {
    glViewport(0, 0, DD2_SKY_TEST_SIDE, DD2_SKY_TEST_SIDE);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -2, 2);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static bool dd2_sky_test_pixel(dd2_renderer *renderer, bool red) {
    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    if (pixels == NULL) {
        return false;
    }
    const size_t sample = ((size_t)DD2_SKY_TEST_SAMPLE * DD2_SKY_TEST_SIDE + DD2_SKY_TEST_SAMPLE) *
                          DD2_SKY_TEST_CHANNELS;
    return pixels[sample] == (red ? UINT8_MAX : 0) && pixels[sample + 1] == 0 &&
           pixels[sample + 2] == (red ? 0 : UINT8_MAX);
}

static bool dd2_sky_test_frame(dd2_sky_draw *sky, dd2_renderer *renderer) {
    dd2_sky_test_view();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(2, 3, 2, 3, -2, 2);
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    if (!dd2_sky_draw_frame(sky)) {
        return false;
    }
    const dd2_sky_draw_stats hidden = dd2_sky_draw_statistics(sky);
    GLboolean hidden_writes = GL_FALSE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &hidden_writes);
    if (hidden.tested != DD2_TRACK_SKY_PATCHES || hidden.culled != DD2_TRACK_SKY_PATCHES ||
        hidden.visible != 0 || hidden.batches != 0 || hidden.triangles != 0 ||
        !glIsEnabled(GL_DEPTH_TEST) || hidden_writes != GL_TRUE) {
        return false;
    }
    dd2_sky_test_view();
    if (!dd2_sky_draw_frame(sky) || !dd2_sky_test_pixel(renderer, false)) {
        return false;
    }
    const dd2_sky_draw_stats initial = dd2_sky_draw_statistics(sky);
    if (initial.tested != DD2_TRACK_SKY_PATCHES || initial.visible != DD2_TRACK_SKY_PATCHES ||
        initial.culled != 0 || initial.triangles != DD2_TRACK_SKY_PATCHES) {
        return false;
    }
    dd2_sky_test_view();
    glTranslatef(dd2_sky_test_translation, -dd2_sky_test_translation, dd2_sky_test_translation);
    float before[DD2_FRUSTUM_MATRIX] = {0};
    float after[DD2_FRUSTUM_MATRIX] = {0};
    glGetFloatv(GL_MODELVIEW_MATRIX, before);
    if (!dd2_sky_draw_frame(sky) || !dd2_sky_test_pixel(renderer, false)) {
        return false;
    }
    glGetFloatv(GL_MODELVIEW_MATRIX, after);
    for (size_t element = 0; element < DD2_FRUSTUM_MATRIX; ++element) {
        if (before[element] != after[element]) {
            return false;
        }
    }
    glLoadIdentity();
    /* This triangle is farther than the blue background. It must still pass
     * depth testing because a background must leave the cleared depth intact. */
    glBegin(GL_TRIANGLES);
    glColor3f(1, 0, 0);
    glVertex3f(0, 0, -1);
    glVertex3f(1, 0, -1);
    glVertex3f(0, 1, -1);
    glEnd();
    GLboolean depth_writes = GL_FALSE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_writes);
    if (!dd2_sky_test_pixel(renderer, true) || !glIsEnabled(GL_DEPTH_TEST) ||
        depth_writes != GL_TRUE) {
        return false;
    }
    dd2_sky_test_view();
    glRotatef(dd2_sky_test_rotation, 0, 0, 1);
    if (!dd2_sky_draw_frame(sky)) {
        return false;
    }
    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    const size_t sample = ((size_t)DD2_SKY_TEST_SAMPLE * DD2_SKY_TEST_SIDE + DD2_SKY_TEST_SAMPLE) *
                          DD2_SKY_TEST_CHANNELS;
    return pixels != NULL && pixels[sample] == 0 && pixels[sample + 2] == 0 &&
           glGetError() == GL_NO_ERROR;
}

int main(void) {
    uint8_t scene[DD2_TRACK_TEST_SCENE_BYTES] = {0};
    uint8_t road[DD2_TRACK_TEST_ROAD_BYTES] = {0};
    dd2_track_test_source((dd2_track_test_buffers){.scene = scene, .road = road});
    dd2_track *track = dd2_track_create_prepared(
        DD2_TRACK_TEST_ARENA,
        (dd2_track_prepared_source){.scene = {scene, sizeof(scene)}, .road = {road, sizeof(road)}},
        dd2_sky_test_model, NULL);
    dd2_renderer *renderer = dd2_renderer_create(
        &(dd2_render_options){.width = DD2_SKY_TEST_SIDE, .height = DD2_SKY_TEST_SIDE});
    dd2_model_texture_cache *cache = dd2_model_texture_cache_create(dd2_sky_test_no_image, NULL);
    dd2_sky_draw *sky = dd2_sky_draw_create(track, cache);
    const bool passed = renderer != NULL && sky != NULL && dd2_sky_test_frame(sky, renderer) &&
                        dd2_model_texture_cache_count(cache) == 0 &&
                        dd2_sky_draw_create(NULL, cache) == NULL && !dd2_sky_draw_frame(NULL);
    dd2_sky_draw_destroy(sky);
    dd2_model_texture_cache_destroy(cache);
    dd2_renderer_destroy(renderer);
    dd2_track_destroy(track);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
