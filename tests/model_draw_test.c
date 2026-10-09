#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/image.h"
#include "assets/model.h"
#include "image_fixture.h"
#include "model_fixture.h"
#include "render/frustum.h"
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
static const float dd2_draw_test_half = 0.5F;
static const float dd2_draw_test_outside = 3;
static const float dd2_draw_test_pivot = 2;
static const float dd2_draw_test_reverse = 180;
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

static void dd2_draw_test_scalar(uint8_t *destination, float value) {
    uint32_t bits = 0;
    const unsigned char *source = (const unsigned char *)&value;
    unsigned char *target = (unsigned char *)&bits;
    for (size_t index = 0; index < sizeof(bits); ++index) {
        target[index] = source[index];
    }
    dd2_test_write_le32(destination, bits);
}

static void dd2_draw_test_projection(void) {
    glViewport(0, 0, DD2_DRAW_TEST_SIDE, DD2_DRAW_TEST_SIDE);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static bool dd2_draw_test_role(dd2_model_role role, bool transparent) {
    uint8_t bytes[DD2_MODEL_TEST_BYTES] = {0};
    dd2_model_test_source(bytes);
    dd2_test_write_le32(bytes + DD2_MODEL_TEST_ROLE, (uint32_t)role);
    if (transparent) {
        dd2_draw_test_scalar(bytes + DD2_MODEL_TEST_COLOR + ((size_t)3 * DD2_MODEL_TEST_WORD),
                             dd2_draw_test_half);
    }
    dd2_renderer *renderer = dd2_renderer_create(
        &(dd2_render_options){.width = DD2_DRAW_TEST_SIDE, .height = DD2_DRAW_TEST_SIDE});
    dd2_model *model = dd2_model_create((dd2_byte_view){bytes, sizeof(bytes)});
    dd2_draw_test_source source = {.fixture = DD2_DRAW_TEST_RGB_FIXTURE};
    dd2_model_draw *draw = model != NULL && renderer != NULL
                               ? dd2_model_draw_create(model, dd2_draw_test_loader, &source)
                               : NULL;
    bool passed = draw != NULL;
    if (passed) {
        dd2_draw_test_projection();
        passed = dd2_model_draw_frame(draw, (dd2_model_draw_options){.cockpit_only = true});
        const bool interior = role == DD2_MODEL_COCKPIT || role == DD2_MODEL_STEERING;
        const dd2_model_draw_stats stats = dd2_model_draw_statistics(draw);
        passed = passed && stats.tested == 1 && stats.culled == 0 &&
                 stats.role_filtered == (interior ? 0U : 1U) &&
                 stats.triangles == (interior ? 1U : 0U) && stats.batches == stats.triangles;
        const uint8_t *pixels = dd2_renderer_pixels(renderer);
        passed = passed && pixels != NULL;
        if (pixels != NULL && !interior) {
            for (size_t pixel = 0; pixel < (size_t)DD2_DRAW_TEST_SIDE * DD2_DRAW_TEST_SIDE;
                 ++pixel) {
                for (size_t channel = 0; channel < 3; ++channel) {
                    passed = passed && pixels[(pixel * DD2_DRAW_TEST_CHANNELS) + channel] == 0;
                }
            }
        }
        dd2_draw_test_projection();
        passed = passed && dd2_model_draw_frame(draw, (dd2_model_draw_options){0});
        const dd2_model_draw_stats exterior = dd2_model_draw_statistics(draw);
        passed = passed && exterior.tested == 1 && exterior.triangles == 1 &&
                 exterior.batches == 1 && exterior.culled == 0 && exterior.role_filtered == 0;
    }
    dd2_model_draw_destroy(draw);
    dd2_model_destroy(model);
    dd2_renderer_destroy(renderer);
    return passed;
}

typedef struct {
    dd2_model_role role;
    float local_x;
    float translation[3];
    dd2_model_draw_options options;
    bool visible;
} dd2_draw_test_visibility;

static bool dd2_draw_test_visibility_case(dd2_draw_test_visibility test) {
    uint8_t bytes[DD2_MODEL_TEST_BYTES] = {0};
    dd2_model_test_source(bytes);
    dd2_test_write_le32(bytes + DD2_MODEL_TEST_ROLE, (uint32_t)test.role);
    for (size_t index = 0; index < 3; ++index) {
        float x_pos = test.local_x;
        if (index == 1) {
            x_pos += 1;
        }
        dd2_draw_test_scalar(
            bytes + DD2_MODEL_TEST_VERTICES + (index * DD2_MODEL_TEST_VERTEX_BYTES), x_pos);
    }
    dd2_draw_test_scalar(bytes + DD2_MODEL_TEST_PIVOT, dd2_draw_test_pivot);
    dd2_renderer *renderer = dd2_renderer_create(
        &(dd2_render_options){.width = DD2_DRAW_TEST_SIDE, .height = DD2_DRAW_TEST_SIDE});
    dd2_model *model = dd2_model_create((dd2_byte_view){bytes, sizeof(bytes)});
    dd2_draw_test_source source = {.fixture = DD2_DRAW_TEST_RGB_FIXTURE};
    dd2_model_draw *draw = model != NULL && renderer != NULL
                               ? dd2_model_draw_create(model, dd2_draw_test_loader, &source)
                               : NULL;
    bool passed = draw != NULL;
    if (passed) {
        dd2_draw_test_projection();
        glTranslatef(test.translation[0], test.translation[1], test.translation[2]);
        float before[DD2_FRUSTUM_MATRIX] = {0};
        float after[DD2_FRUSTUM_MATRIX] = {0};
        glGetFloatv(GL_MODELVIEW_MATRIX, before);
        passed = dd2_model_draw_frame(draw, test.options);
        glGetFloatv(GL_MODELVIEW_MATRIX, after);
        const dd2_model_draw_stats stats = dd2_model_draw_statistics(draw);
        passed = passed && stats.tested == 1 && stats.role_filtered == 0 &&
                 stats.culled == (test.visible ? 0U : 1U) &&
                 stats.triangles == (test.visible ? 1U : 0U) && stats.batches == stats.triangles &&
                 glGetError() == GL_NO_ERROR;
        for (size_t index = 0; index < DD2_FRUSTUM_MATRIX; ++index) {
            passed = passed && before[index] == after[index];
        }
    }
    dd2_model_draw_destroy(draw);
    dd2_model_destroy(model);
    dd2_renderer_destroy(renderer);
    return passed;
}

static bool dd2_draw_test_selection(void) {
    for (unsigned role = DD2_MODEL_EXTERIOR; role <= DD2_MODEL_STEERING; ++role) {
        if (!dd2_draw_test_role((dd2_model_role)role, false) ||
            !dd2_draw_test_role((dd2_model_role)role, true)) {
            return false;
        }
    }
    const dd2_draw_test_visibility cases[] = {
        {.visible = true},
        {.translation = {dd2_draw_test_outside, 0, 0}},
        {.translation = {-1, 0, 0}, .visible = true},
        {.translation = {-dd2_draw_test_half, 0, 0}, .visible = true},
        {.translation = {0, 0, dd2_draw_test_outside}},
        {.role = DD2_MODEL_STEERING, .options = {.steering = dd2_draw_test_reverse}},
        {.role = DD2_MODEL_STEERING,
         .local_x = dd2_draw_test_outside,
         .options = {.steering = dd2_draw_test_reverse},
         .visible = true},
        {.role = DD2_MODEL_WHEEL_FRONT_LEFT, .options = {.front_steer = dd2_draw_test_reverse}},
        {.role = DD2_MODEL_WHEEL_REAR_RIGHT,
         .options = {.wheel_roll = dd2_draw_test_reverse},
         .visible = true}};
    for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        if (!dd2_draw_test_visibility_case(cases[index])) {
            return false;
        }
    }
    return true;
}

int main(void) {
    dd2_model_draw_destroy(NULL);
    if (dd2_model_draw_create(NULL, NULL, NULL) != NULL || dd2_model_draw_batch_count(NULL) != 0 ||
        dd2_model_draw_frame(NULL, (dd2_model_draw_options){0}) ||
        !dd2_draw_test_case(DD2_DRAW_TEST_RGB_FIXTURE, false) ||
        !dd2_draw_test_case(DD2_DRAW_TEST_RGBA_FIXTURE, false) ||
        !dd2_draw_test_case(DD2_DRAW_TEST_RGB_FIXTURE, true) || !dd2_draw_test_cutout() ||
        !dd2_draw_test_selection() || dd2_model_draw_statistics(NULL).tested != 0) {
        return EXIT_FAILURE;
    }
    puts("{\"scope\":\"authored GL upload orientation, mipmaps, alpha, state and failure "
         "lifetime, cockpit roles and posed frustum submission\",\"pass\":true}");
    return EXIT_SUCCESS;
}
