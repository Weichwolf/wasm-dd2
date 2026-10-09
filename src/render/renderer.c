#include "render/renderer.h"

#include <GL/softgl.h>
#include <stdint.h>
#include <stdlib.h>

enum { DD2_RENDER_MAX_DIMENSION = 4096, DD2_RGBA_CHANNELS = 4, DD2_RENDER_MSAA = 4 };

struct dd2_renderer {
    softgl_ctx *context;
    size_t rgba_bytes;
};

dd2_renderer *dd2_renderer_create(const dd2_render_options *options) {
    if (options == NULL || options->width <= 0 || options->height <= 0 ||
        options->width > DD2_RENDER_MAX_DIMENSION || options->height > DD2_RENDER_MAX_DIMENSION ||
        (options->samples != 0 && options->samples != 2 && options->samples != DD2_RENDER_MSAA)) {
        return NULL;
    }

    dd2_renderer *renderer = calloc(1, sizeof(*renderer));
    if (renderer == NULL) {
        return NULL;
    }

    renderer->context =
        softgl_create_multisample(options->width, options->height, options->samples);
    if (renderer->context == NULL) {
        free(renderer);
        return NULL;
    }

    renderer->rgba_bytes = (size_t)options->width * (size_t)options->height * DD2_RGBA_CHANNELS;
    softgl_make_current(renderer->context);
    return renderer;
}

void dd2_renderer_destroy(dd2_renderer *renderer) {
    if (renderer != NULL) {
        softgl_destroy(renderer->context);
        free(renderer);
    }
}

void dd2_renderer_make_current(dd2_renderer *renderer) {
    softgl_make_current(renderer != NULL ? renderer->context : NULL);
}

size_t dd2_renderer_rgba_bytes(const dd2_renderer *renderer) {
    return renderer != NULL ? renderer->rgba_bytes : 0;
}

const uint8_t *dd2_renderer_pixels(dd2_renderer *renderer) {
    return renderer != NULL ? softgl_read_rgba8(renderer->context) : NULL;
}
