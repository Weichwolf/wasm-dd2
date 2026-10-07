#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/textures.h"
#include "render/renderer.h"

#include <GL/softgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { DD2_TEXTURE_PROBE_SIDE = 64, DD2_TEXTURE_PROBE_ALPHA = 255 };

static bool dd2_draw_texture(dd2_texture_set *textures) {
    uint8_t *rgba = malloc(DD2_TEXTURE_PAGE_RGBA_BYTES);
    if (rgba == NULL) {
        return false;
    }
    const bool converted = dd2_texture_page_rgba(
        textures, (dd2_texture_sample){.shade = DD2_TEST_NEUTRAL_SHADE, .cutout = true},
        (dd2_byte_buffer){.data = rgba, .size = DD2_TEXTURE_PAGE_RGBA_BYTES});
    if (!converted) {
        free(rgba);
        return false;
    }
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DD2_TEXTURE_PAGE_SIDE, DD2_TEXTURE_PAGE_SIDE, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    free(rgba);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0F);
    glViewport(0, 0, DD2_TEXTURE_PROBE_SIDE, DD2_TEXTURE_PROBE_SIDE);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    const float u_min = (float)(DD2_TEXTURE_PAGE_SIDE - 2) / (float)DD2_TEXTURE_PAGE_SIDE;
    const float v_min = (float)(DD2_TEXTURE_PAGE_SIDE - 1) / (float)DD2_TEXTURE_PAGE_SIDE;
    glBegin(GL_QUADS);
    glTexCoord2f(u_min, v_min);
    glVertex2f(-1.0F, -1.0F);
    glTexCoord2f(1.0F, v_min);
    glVertex2f(1.0F, -1.0F);
    glTexCoord2f(1.0F, 1.0F);
    glVertex2f(1.0F, 1.0F);
    glTexCoord2f(u_min, 1.0F);
    glVertex2f(-1.0F, 1.0F);
    glEnd();
    glDeleteTextures(1, &texture);
    return glGetError() == GL_NO_ERROR;
}

static bool dd2_texture_render_probe(dd2_texture_set *textures) {
    dd2_renderer *renderer = dd2_renderer_create(
        &(dd2_render_options){.width = DD2_TEXTURE_PROBE_SIDE, .height = DD2_TEXTURE_PROBE_SIDE});
    if (renderer == NULL) {
        return false;
    }
    bool passed = dd2_draw_texture(textures);
    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    if (pixels == NULL) {
        passed = false;
    } else {
        for (size_t row = 0; row < DD2_TEXTURE_PROBE_SIDE; ++row) {
            for (size_t column = 0; column < DD2_TEXTURE_PROBE_SIDE; ++column) {
                const size_t offset =
                    ((row * DD2_TEXTURE_PROBE_SIDE) + column) * DD2_TEST_COLOR_CHANNELS;
                const bool opaque = column < DD2_TEXTURE_PROBE_SIDE / 2;
                passed = passed && pixels[offset] == (opaque ? DD2_TEST_TEXEL_A : 0) &&
                         pixels[offset + 1] == (opaque ? UINT8_MAX - DD2_TEST_TEXEL_A : 0) &&
                         pixels[offset + 2] == (opaque ? DD2_TEST_PALETTE_BLUE : 0) &&
                         pixels[offset + 3] == DD2_TEXTURE_PROBE_ALPHA;
            }
        }
    }
    dd2_renderer_destroy(renderer);
    return passed;
}

int main(void) {
    dd2_test_texture_fixture fixture = {0};
    const dd2_texture_sources sources = dd2_test_texture_sources(&fixture);
    dd2_texture_set *textures = dd2_texture_set_create(&sources);
    if (textures == NULL) {
        return EXIT_FAILURE;
    }
    const bool passed = dd2_texture_render_probe(textures);
    dd2_texture_set_destroy(textures);
    if (!passed) {
        return EXIT_FAILURE;
    }
    puts("{\"scope\":\"decoded texture upload and cutout on SoftGL\",\"pass\":true}");
    return EXIT_SUCCESS;
}
