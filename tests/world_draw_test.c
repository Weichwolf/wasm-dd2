#include "assets/bytes.h"
#include "assets/image.h"
#include "assets/world.h"
#include "image_fixture.h"
#include "render/model_draw.h"
#include "render/renderer.h"
#include "render/world_draw.h"
#include "world_fixture.h"

#include <GL/softgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_WORLD_DRAW_TEST_SIDE = 16 };
typedef struct {
    size_t calls;
    bool fail_hidden;
} dd2_world_draw_test_source;

static dd2_image *dd2_world_draw_test_image(void *user, const char *path) {
    dd2_world_draw_test_source *source = user;
    ++source->calls;
    if (source->fail_hidden && strcmp(path, "textures/missing.png") == 0) {
        return NULL;
    }
    const dd2_png_test_fixture *fixture = &dd2_png_test_fixtures[0];
    return dd2_image_create_png((dd2_byte_view){fixture->encoded, fixture->length});
}

static void dd2_world_draw_test_camera(float translate) {
    glViewport(0, 0, DD2_WORLD_DRAW_TEST_SIDE, DD2_WORLD_DRAW_TEST_SIDE);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(translate, 0, 0);
}

static bool dd2_world_draw_test_frames(dd2_world_draw *draw, dd2_world_draw_test_source *source) {
    dd2_world_draw_stats stats = {0};
    dd2_world_draw_test_camera(0);
    const dd2_model_draw_options options = {.cutout_textures = true};
    if (!dd2_world_draw_frame(draw, options, &stats) || stats.tested != 3 || stats.visible != 2 ||
        stats.culled != 1 || stats.triangles != 2 || stats.uploaded_textures != 1 ||
        source->calls != 1) {
        return false;
    }
    // Repeated frames reuse both the model batches and their one shared upload.
    if (!dd2_world_draw_frame(draw, options, &stats) || source->calls != 1) {
        return false;
    }
    const float hidden_shift = -8;
    dd2_world_draw_test_camera(hidden_shift);
    if (dd2_world_draw_frame(draw, options, &stats) || source->calls != 2 ||
        stats.uploaded_textures != 1) {
        return false;
    }
    source->fail_hidden = false;
    if (!dd2_world_draw_frame(draw, options, &stats) || stats.visible != 1 || stats.culled != 2 ||
        stats.triangles != 1 || stats.uploaded_textures != 2 || source->calls != 3) {
        return false;
    }
    const float all_shift = -32;
    dd2_world_draw_test_camera(all_shift);
    return dd2_world_draw_frame(draw, options, &stats) && stats.visible == 0 && stats.culled == 3 &&
           stats.triangles == 0 && source->calls == 3 && glIsEnabled(GL_ALPHA_TEST) == GL_FALSE &&
           glGetError() == GL_NO_ERROR;
}

static bool dd2_world_draw_test_shared(const dd2_world *world) {
    dd2_world_draw_test_source source = {0};
    dd2_model_texture_cache *cache =
        dd2_model_texture_cache_create(dd2_world_draw_test_image, &source);
    dd2_world_draw *first = dd2_world_draw_create_shared(world, cache);
    dd2_world_draw *second = dd2_world_draw_create_shared(world, cache);
    dd2_world_draw_test_camera(0);
    dd2_world_draw_stats stats = {0};
    bool passed = first != NULL && second != NULL &&
                  dd2_world_draw_frame(first, (dd2_model_draw_options){0}, &stats) &&
                  source.calls == 1;
    dd2_world_draw_destroy(first);
    passed = passed && dd2_world_draw_frame(second, (dd2_model_draw_options){0}, &stats) &&
             source.calls == 1 && dd2_model_texture_cache_count(cache) == 1;
    dd2_world_draw_destroy(second);
    passed = passed && dd2_model_texture_cache_count(cache) == 1;
    dd2_model_texture_cache_destroy(cache);
    return passed;
}

int main(void) {
    uint8_t bytes[DD2_WORLD_TEST_BYTES] = {0};
    dd2_world_test_source(bytes);
    dd2_world_test_loader_state state = {.hidden_texture = true};
    dd2_world *world =
        dd2_world_create((dd2_byte_view){bytes, sizeof(bytes)}, dd2_world_test_loader, &state);
    dd2_renderer *renderer = dd2_renderer_create(&(dd2_render_options){
        .width = DD2_WORLD_DRAW_TEST_SIDE, .height = DD2_WORLD_DRAW_TEST_SIDE});
    dd2_world_draw_test_source source = {.fail_hidden = true};
    dd2_world_draw *draw =
        renderer != NULL ? dd2_world_draw_create(world, dd2_world_draw_test_image, &source) : NULL;
    const bool passed = draw != NULL && source.calls == 0 &&
                        dd2_world_draw_test_frames(draw, &source) &&
                        dd2_world_draw_test_shared(world);
    dd2_world_draw_destroy(draw);
    dd2_world_destroy(world);
    dd2_renderer_destroy(renderer);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
