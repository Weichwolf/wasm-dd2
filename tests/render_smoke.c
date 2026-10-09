#include "render/renderer.h"

#include <GL/softgl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { DD2_PROBE_WIDTH = 64, DD2_PROBE_HEIGHT = 64, DD2_RGBA_CHANNELS = 4, DD2_CHANNEL_MAX = 255 };

static bool dd2_probe_render(int samples) {
    const dd2_render_options options = {
        .width = DD2_PROBE_WIDTH, .height = DD2_PROBE_HEIGHT, .samples = samples};
    dd2_renderer *renderer = dd2_renderer_create(&options);
    if (renderer == NULL) {
        return false;
    }

    glViewport(0, 0, options.width, options.height);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(1.0F, 0.0F, 0.0F);
    const float extent = 0.75F;
    glBegin(GL_TRIANGLES);
    glVertex2f(-extent, -extent);
    glVertex2f(extent, -extent);
    glVertex2f(0.0F, extent);
    glEnd();

    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    const size_t center = ((size_t)(DD2_PROBE_HEIGHT / 2) * DD2_PROBE_WIDTH + DD2_PROBE_WIDTH / 2) *
                          DD2_RGBA_CHANNELS;
    const size_t expected_bytes = (size_t)DD2_PROBE_WIDTH * DD2_PROBE_HEIGHT * DD2_RGBA_CHANNELS;
    GLint actual_samples = -1;
    glGetIntegerv(GL_SAMPLES, &actual_samples);
    size_t partial = 0;
    if (pixels != NULL) {
        for (size_t index = 0; index < expected_bytes; index += DD2_RGBA_CHANNELS) {
            partial += pixels[index] > 0 && pixels[index] < DD2_CHANNEL_MAX;
        }
    }
    const bool passed = actual_samples == samples && (samples == 0 || partial != 0) &&
                        pixels != NULL && dd2_renderer_rgba_bytes(renderer) == expected_bytes &&
                        pixels[0] == 0 && pixels[1] == 0 && pixels[2] == 0 &&
                        pixels[3] == DD2_CHANNEL_MAX && pixels[center] == DD2_CHANNEL_MAX &&
                        pixels[center + 1] == 0 && pixels[center + 2] == 0 &&
                        pixels[center + 3] == DD2_CHANNEL_MAX && glGetError() == GL_NO_ERROR;
    dd2_renderer_destroy(renderer);
    return passed;
}

int main(void) {
    const dd2_render_options invalid = {.width = 0, .height = DD2_PROBE_HEIGHT};
    if (dd2_renderer_create(NULL) != NULL || dd2_renderer_create(&invalid) != NULL ||
        dd2_renderer_rgba_bytes(NULL) != 0 || dd2_renderer_pixels(NULL) != NULL) {
        return EXIT_FAILURE;
    }
    dd2_renderer_destroy(NULL);
    const int sample_counts[] = {0, 2, 4};
    for (size_t index = 0; index < sizeof(sample_counts) / sizeof(sample_counts[0]); ++index) {
        if (!dd2_probe_render(sample_counts[index])) {
            return EXIT_FAILURE;
        }
    }
    puts("{\"scope\":\"rewrite SoftGL bootstrap only\",\"pass\":true}");
    return EXIT_SUCCESS;
}
