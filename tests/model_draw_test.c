#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/image.h"
#include "assets/model.h"
#include "image_fixture.h"
#include "model_fixture.h"
#include "render/model_draw.h"
#include "render/renderer.h"

#include <GL/softgl.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_DRAW_TEST_SIDE = 2,
    DD2_DRAW_TEST_CHANNELS = 4,
    DD2_DRAW_TEST_UV = 24,
    DD2_DRAW_TEST_RGB_FIXTURE = 5,
    DD2_DRAW_TEST_RGBA_FIXTURE = 15,
    DD2_DRAW_TEST_RGBA_ROW = 8,
    DD2_DRAW_TEST_MIP_RED = 15,
    DD2_DRAW_TEST_MIP_GREEN = 30,
    DD2_DRAW_TEST_MIP_BLUE = 65
};
static const uint32_t dd2_draw_test_two = UINT32_C(0x40000000);
static const uint32_t dd2_draw_test_tiled = UINT32_C(0x43000000);
static const double dd2_draw_test_decode_break = 0.04045;
static const double dd2_draw_test_slope = 12.92;
static const double dd2_draw_test_offset = 0.055;
static const double dd2_draw_test_gain = 1.055;
static const double dd2_draw_test_exponent = 2.4;

static unsigned dd2_draw_test_linear(uint8_t byte) {
    const double value = (double)byte / UINT8_MAX;
    const double decoded =
        value <= dd2_draw_test_decode_break
            ? value / dd2_draw_test_slope
            : pow((value + dd2_draw_test_offset) / dd2_draw_test_gain, dd2_draw_test_exponent);
    return (unsigned)lround(decoded * UINT8_MAX);
}

typedef struct {
    size_t fixture;
    unsigned calls;
    bool fail;
} dd2_draw_test_source;

static dd2_image *dd2_draw_test_loader(void *user, const char *resource) {
    dd2_draw_test_source *source = user;
    ++source->calls;
    if (source->fail || resource == NULL) {
        return NULL;
    }
    const dd2_png_test_fixture *fixture = &dd2_png_test_fixtures[source->fixture];
    return dd2_image_create_png((dd2_byte_view){fixture->encoded, fixture->length});
}

static dd2_model *dd2_draw_test_model(bool minify) {
    uint8_t bytes[DD2_MODEL_TEST_BYTES] = {0};
    dd2_model_test_source(bytes);
    dd2_test_write_le32(bytes + DD2_MODEL_TEST_VERTICES + DD2_MODEL_TEST_VERTEX_BYTES,
                        dd2_draw_test_two);
    dd2_test_write_le32(bytes + DD2_MODEL_TEST_VERTICES +
                            ((size_t)DD2_MODEL_TEST_VERTEX_BYTES * 2) + DD2_MODEL_TEST_WORD,
                        dd2_draw_test_two);
    const uint32_t uvw = minify ? dd2_draw_test_tiled : dd2_draw_test_two;
    dd2_test_write_le32(
        bytes + DD2_MODEL_TEST_VERTICES + DD2_MODEL_TEST_VERTEX_BYTES + DD2_DRAW_TEST_UV, uvw);
    dd2_test_write_le32(bytes + DD2_MODEL_TEST_VERTICES +
                            ((size_t)DD2_MODEL_TEST_VERTEX_BYTES * 2) + DD2_DRAW_TEST_UV +
                            DD2_MODEL_TEST_WORD,
                        uvw);
    return dd2_model_create((dd2_byte_view){bytes, sizeof(bytes)});
}

static bool dd2_draw_test_pixels(const uint8_t *pixels, size_t fixture_index, bool minify) {
    const dd2_png_test_fixture *fixture = &dd2_png_test_fixtures[fixture_index];
    const uint8_t mip[] = {DD2_DRAW_TEST_MIP_RED, DD2_DRAW_TEST_MIP_GREEN, DD2_DRAW_TEST_MIP_BLUE};
    for (size_t row = 0; row < DD2_DRAW_TEST_SIDE; ++row) {
        for (size_t column = 0; column < DD2_DRAW_TEST_SIDE; ++column) {
            const size_t source = ((DD2_DRAW_TEST_SIDE - row - 1) * DD2_DRAW_TEST_RGBA_ROW) +
                                  (column * DD2_DRAW_TEST_CHANNELS);
            const size_t destination =
                ((row * DD2_DRAW_TEST_SIDE) + column) * DD2_DRAW_TEST_CHANNELS;
            for (size_t channel = 0; channel < 3; ++channel) {
                const unsigned expected =
                    minify ? mip[channel]
                           : (dd2_draw_test_linear(fixture->expected[source + channel]) *
                                  fixture->expected[source + 3] +
                              (UINT8_MAX / 2)) /
                                 UINT8_MAX;
                const unsigned actual = pixels[destination + channel];
                if ((actual > expected ? actual - expected : expected - actual) > 1) {
                    return false;
                }
            }
        }
    }
    return true;
}

static bool dd2_draw_test_case(size_t fixture, bool minify) {
    dd2_renderer *renderer = dd2_renderer_create(
        &(dd2_render_options){.width = DD2_DRAW_TEST_SIDE, .height = DD2_DRAW_TEST_SIDE});
    dd2_model *model = dd2_draw_test_model(minify);
    dd2_draw_test_source source = {.fixture = fixture};
    dd2_model_draw *draw = model != NULL && renderer != NULL
                               ? dd2_model_draw_create(model, dd2_draw_test_loader, &source)
                               : NULL;
    bool passed = draw != NULL && source.calls == 1 && dd2_model_draw_batch_count(draw) == 1;
    if (passed) {
        glViewport(0, 0, DD2_DRAW_TEST_SIDE, DD2_DRAW_TEST_SIDE);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, 1, 0, 1, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        passed = dd2_model_draw_frame(draw, (dd2_model_draw_options){0});
        const uint8_t *pixels = dd2_renderer_pixels(renderer);
        GLboolean depth_write = GL_FALSE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_write);
        passed = passed && pixels != NULL && dd2_draw_test_pixels(pixels, fixture, minify) &&
                 depth_write == GL_TRUE && glIsEnabled(GL_LIGHTING) == GL_FALSE &&
                 glIsEnabled(GL_BLEND) == GL_FALSE && glIsEnabled(GL_TEXTURE_2D) == GL_FALSE &&
                 glGetError() == GL_NO_ERROR;
    }
    dd2_model_draw_destroy(draw);
    source.fail = true;
    draw = dd2_model_draw_create(model, dd2_draw_test_loader, &source);
    passed = passed && draw == NULL;
    dd2_model_draw_destroy(draw);
    dd2_model_destroy(model);
    dd2_renderer_destroy(renderer);
    return passed;
}

static bool dd2_draw_test_cutout_pixels(const uint8_t *pixels) {
    const dd2_png_test_fixture *fixture = &dd2_png_test_fixtures[DD2_DRAW_TEST_RGBA_FIXTURE];
    const uint8_t threshold = 127;
    for (size_t row = 0; row < DD2_DRAW_TEST_SIDE; ++row) {
        for (size_t column = 0; column < DD2_DRAW_TEST_SIDE; ++column) {
            const size_t source = ((DD2_DRAW_TEST_SIDE - row - 1) * DD2_DRAW_TEST_RGBA_ROW) +
                                  (column * DD2_DRAW_TEST_CHANNELS);
            const size_t target = ((row * DD2_DRAW_TEST_SIDE) + column) * DD2_DRAW_TEST_CHANNELS;
            for (size_t channel = 0; channel < 3; ++channel) {
                unsigned expected = channel == 1 ? UINT8_MAX : 0;
                if (fixture->expected[source + 3] > threshold) {
                    expected = dd2_draw_test_linear(fixture->expected[source + channel]);
                }
                const unsigned actual = pixels[target + channel];
                if ((actual > expected ? actual - expected : expected - actual) > 1) {
                    return false;
                }
            }
        }
    }
    return true;
}

static bool dd2_draw_test_cutout(void) {
    dd2_renderer *renderer = dd2_renderer_create(
        &(dd2_render_options){.width = DD2_DRAW_TEST_SIDE, .height = DD2_DRAW_TEST_SIDE});
    dd2_model *model = dd2_draw_test_model(false);
    dd2_draw_test_source source = {.fixture = DD2_DRAW_TEST_RGBA_FIXTURE};
    dd2_model_draw *draw = model != NULL && renderer != NULL
                               ? dd2_model_draw_create(model, dd2_draw_test_loader, &source)
                               : NULL;
    bool passed = draw != NULL;
    if (passed) {
        glViewport(0, 0, DD2_DRAW_TEST_SIDE, DD2_DRAW_TEST_SIDE);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, 1, 0, 1, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        passed = dd2_model_draw_frame(draw, (dd2_model_draw_options){.cutout_textures = true});
        // The green background must survive discarded alpha texels and remain
        // occluded behind retained texels, including alpha 128 at the threshold.
        const float behind = -0.5F;
        glColor3f(0, 1, 0);
        glBegin(GL_TRIANGLES);
        glVertex3f(0, 0, behind);
        glVertex3f(2, 0, behind);
        glVertex3f(0, 2, behind);
        glEnd();
        const uint8_t *pixels = dd2_renderer_pixels(renderer);
        passed = passed && pixels != NULL && dd2_draw_test_cutout_pixels(pixels) &&
                 glIsEnabled(GL_ALPHA_TEST) == GL_FALSE && glGetError() == GL_NO_ERROR;
    }
    dd2_model_draw_destroy(draw);
    dd2_model_destroy(model);
    dd2_renderer_destroy(renderer);
    return passed;
}

int main(void) {
    dd2_model_draw_destroy(NULL);
    if (dd2_model_draw_create(NULL, NULL, NULL) != NULL || dd2_model_draw_batch_count(NULL) != 0 ||
        dd2_model_draw_frame(NULL, (dd2_model_draw_options){0}) ||
        !dd2_draw_test_case(DD2_DRAW_TEST_RGB_FIXTURE, false) ||
        !dd2_draw_test_case(DD2_DRAW_TEST_RGBA_FIXTURE, false) ||
        !dd2_draw_test_case(DD2_DRAW_TEST_RGB_FIXTURE, true) || !dd2_draw_test_cutout()) {
        return EXIT_FAILURE;
    }
    puts("{\"scope\":\"authored GL upload orientation, mipmaps, alpha, state and failure "
         "lifetime\",\"pass\":true}");
    return EXIT_SUCCESS;
}
