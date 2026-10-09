#include "assets/bytes.h"
#include "assets/image.h"
#include "assets/model.h"
#include "platform/file.h"
#include "render/model_draw.h"
#include "render/model_view.h"
#include "render/renderer.h"

#include <GL/softgl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_PREPARED_WIDTH = 640,
    DD2_PREPARED_HEIGHT = 360,
    DD2_PREPARED_SAMPLES = 4,
    DD2_PREPARED_PATH_BYTES = 1024,
    DD2_PREPARED_CHANNELS = 4,
    DD2_PREPARED_RGB = 3,
    DD2_PREPARED_ARGUMENTS = 4
};

static bool dd2_prepared_read(const char *root, const char *resource, dd2_file *file) {
    char path[DD2_PREPARED_PATH_BYTES] = {0};
    const size_t prefix = strlen(root);
    const size_t name = strlen(resource);
    if (prefix + name + 2 > sizeof(path)) {
        return false;
    }
    for (size_t index = 0; index < prefix; ++index) {
        path[index] = root[index];
    }
    path[prefix] = '/';
    for (size_t index = 0; index < name; ++index) {
        path[prefix + 1 + index] = resource[index];
    }
    return dd2_file_read(path, file);
}

static dd2_image *dd2_prepared_image(void *user, const char *resource) {
    dd2_file file = {0};
    if (!dd2_prepared_read(user, resource, &file)) {
        return NULL;
    }
    dd2_image *image = dd2_image_create_png((dd2_byte_view){file.data, file.size});
    dd2_file_release(&file);
    return image;
}

static bool dd2_prepared_write(dd2_renderer *renderer, const char *path) {
    const char allowed[] = "/tmp/wasm-dd2/";
    if (strncmp(path, allowed, sizeof(allowed) - 1) != 0 || strstr(path, "..") != NULL) {
        return false;
    }
    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    if (pixels == NULL) {
        return false;
    }
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    const char header[] = "P6\n640 360\n255\n";
    bool passed = fwrite(header, 1, sizeof(header) - 1, file) == sizeof(header) - 1;
    uint8_t row[DD2_PREPARED_WIDTH * DD2_PREPARED_RGB] = {0};
    for (size_t y_pos = 0; passed && y_pos < DD2_PREPARED_HEIGHT; ++y_pos) {
        for (size_t x_pos = 0; x_pos < DD2_PREPARED_WIDTH; ++x_pos) {
            const size_t source = ((DD2_PREPARED_HEIGHT - y_pos - 1) * DD2_PREPARED_WIDTH + x_pos) *
                                  DD2_PREPARED_CHANNELS;
            for (size_t channel = 0; channel < DD2_PREPARED_RGB; ++channel) {
                row[(x_pos * DD2_PREPARED_RGB) + channel] = pixels[source + channel];
            }
        }
        passed = fwrite(row, 1, sizeof(row), file) == sizeof(row);
    }
    const bool closed = fclose(file) == 0;
    return passed && closed;
}

int main(int argc, char **argv) {
    if (argc != DD2_PREPARED_ARGUMENTS) {
        return EXIT_FAILURE;
    }
    dd2_file file = {0};
    if (!dd2_prepared_read(argv[1], argv[2], &file)) {
        return EXIT_FAILURE;
    }
    dd2_model *model = dd2_model_create((dd2_byte_view){file.data, file.size});
    dd2_file_release(&file);
    const dd2_render_options viewport = {.width = DD2_PREPARED_WIDTH,
                                         .height = DD2_PREPARED_HEIGHT,
                                         .samples = DD2_PREPARED_SAMPLES,
                                         .output = DD2_RENDER_LINEAR_TO_SRGB};
    dd2_renderer *renderer = model != NULL ? dd2_renderer_create(&viewport) : NULL;
    dd2_model_draw *draw =
        renderer != NULL ? dd2_model_draw_create(model, dd2_prepared_image, argv[1]) : NULL;
    bool passed = false;
    if (draw != NULL) {
        const dd2_model_view view = dd2_model_view_default(false);
        dd2_model_draw_options options = dd2_model_view_apply(&view, viewport);
        options.lighting = false;
        passed = dd2_model_draw_frame(draw, options) && dd2_prepared_write(renderer, argv[3]) &&
                 glGetError() == GL_NO_ERROR;
        printf("{\"scope\":\"prepared model diagnostic only\",\"triangles\":%zu,\"batches\":%zu}\n",
               dd2_model_index_count(model) / DD2_PREPARED_RGB, dd2_model_draw_batch_count(draw));
    }
    dd2_model_draw_destroy(draw);
    dd2_model_destroy(model);
    dd2_renderer_destroy(renderer);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
