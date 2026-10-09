#include "assets/bounds.h"
#include "assets/bytes.h"
#include "assets/image.h"
#include "assets/model.h"
#include "assets/track.h"
#include "assets/world.h"
#include "content_file_fixture.h"
#include "platform/content.h"
#include "platform/file.h"
#include "render/model_draw.h"
#include "render/model_view.h"
#include "render/renderer.h"
#include "render/sky_draw.h"
#include "render/world_draw.h"

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

static void dd2_prepared_world_camera(const dd2_world *world, dd2_render_options viewport,
                                      bool crop) {
    const dd2_world_resource *resources = dd2_world_resources(world);
    const dd2_world_instance *instances = dd2_world_instances(world);
    dd2_bounds bounds = {0};
    for (size_t index = 0; index < dd2_world_instance_count(world); ++index) {
        for (size_t axis = 0; axis < 3; ++axis) {
            const dd2_world_instance *instance = &instances[index];
            const dd2_bounds *local = &resources[instance->resource].bounds;
            const float low = local->minimum[axis] + instance->position[axis];
            const float high = local->maximum[axis] + instance->position[axis];
            bounds.minimum[axis] =
                index == 0 || low < bounds.minimum[axis] ? low : bounds.minimum[axis];
            bounds.maximum[axis] =
                index == 0 || high > bounds.maximum[axis] ? high : bounds.maximum[axis];
        }
    }
    const float padding = 0.6F;
    float radius = 1;
    float center[3] = {0};
    for (size_t axis = 0; axis < 3; ++axis) {
        const float span = bounds.maximum[axis] - bounds.minimum[axis];
        radius = span > radius ? span : radius;
        center[axis] = (bounds.maximum[axis] + bounds.minimum[axis]) / 2;
    }
    radius *= padding;
    if (crop) {
        const float scale = 0.25F;
        radius *= scale;
    }
    const double aspect = (double)viewport.width / viewport.height;
    glViewport(0, 0, viewport.width, viewport.height);
    const float sky[] = {.17F, .23F, .31F, 1};
    glClearColor(sky[0], sky[1], sky[2], sky[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-radius * aspect, radius * aspect, -radius, radius, -radius * 4, radius * 4);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    const float pitch = 55;
    const float yaw = 30;
    glRotatef(pitch, 1, 0, 0);
    glRotatef(yaw, 0, 1, 0);
    glTranslatef(-center[0], -center[1], -center[2]);
}

typedef struct {
    const char *root;
    const char *resource;
    const char *output;
    bool crop;
    unsigned number;
} dd2_prepared_world_capture;

static bool dd2_prepared_world(dd2_prepared_world_capture capture) {
    const char *root = capture.root;
    dd2_file file = {0};
    dd2_track *track = NULL;
    dd2_world *owned_world = NULL;
    const dd2_world *world = NULL;
    if (capture.number != 0) {
        track = dd2_track_load(dd2_content_track_provider(root), capture.number);
        world = dd2_track_world(track);
    } else {
        if (capture.resource == NULL || !dd2_content_test_read(root, capture.resource, &file)) {
            return false;
        }
        owned_world = dd2_world_create((dd2_byte_view){file.data, file.size},
                                       dd2_content_test_model, (void *)root);
        dd2_file_release(&file);
        world = owned_world;
    }
    const dd2_render_options viewport = {.width = DD2_PREPARED_WIDTH,
                                         .height = DD2_PREPARED_HEIGHT,
                                         .samples = DD2_PREPARED_SAMPLES,
                                         .output = DD2_RENDER_LINEAR_TO_SRGB};
    dd2_renderer *renderer = world != NULL ? dd2_renderer_create(&viewport) : NULL;
    dd2_world_draw *draw =
        renderer != NULL ? dd2_world_draw_create(world, dd2_prepared_image, (void *)root) : NULL;
    dd2_world_draw_stats stats = {0};
    bool passed = false;
    if (draw != NULL) {
        dd2_prepared_world_camera(world, viewport, capture.crop);
        passed = dd2_world_draw_frame(
                     draw, (dd2_model_draw_options){.double_sided = true, .cutout_textures = true},
                     &stats) &&
                 dd2_prepared_write(renderer, capture.output);
        printf("{\"scope\":\"prepared static world "
               "diagnostic\",\"objects\":%zu,\"visible\":%zu,\"culled\":%zu,\"triangles\":%zu,"
               "\"batches\":%zu,\"textures\":%zu}\n",
               stats.tested, stats.visible, stats.culled, stats.triangles, stats.batches,
               stats.uploaded_textures);
    }
    dd2_world_draw_destroy(draw);
    dd2_world_destroy(owned_world);
    dd2_track_destroy(track);
    dd2_renderer_destroy(renderer);
    return passed;
}

static void dd2_prepared_sky_camera(char angle) {
    const double height = 0.10825317547305482;
    const double aspect = (double)DD2_PREPARED_WIDTH / (double)DD2_PREPARED_HEIGHT;
    const double near_plane = 0.1875;
    const double far_plane = 937.5;
    const float step = 45;
    const float vertical = 90;
    glViewport(0, 0, DD2_PREPARED_WIDTH, DD2_PREPARED_HEIGHT);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-height * aspect, height * aspect, -height, height, near_plane, far_plane);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (angle == 'U' || angle == 'D') {
        glRotatef(angle == 'U' ? -vertical : vertical, 1, 0, 0);
    } else {
        glRotatef(-step * (float)(angle - '0'), 0, 1, 0);
    }
}

static bool dd2_prepared_sky(dd2_prepared_world_capture capture, char angle) {
    dd2_track *track = dd2_track_load(dd2_content_track_provider(capture.root), capture.number);
    const dd2_render_options viewport = {.width = DD2_PREPARED_WIDTH,
                                         .height = DD2_PREPARED_HEIGHT,
                                         .samples = DD2_PREPARED_SAMPLES,
                                         .output = DD2_RENDER_LINEAR_TO_SRGB};
    dd2_renderer *renderer = track != NULL ? dd2_renderer_create(&viewport) : NULL;
    dd2_model_texture_cache *cache =
        renderer != NULL ? dd2_model_texture_cache_create(dd2_prepared_image, (void *)capture.root)
                         : NULL;
    dd2_sky_draw *draw = dd2_sky_draw_create(track, cache);
    bool passed = false;
    if (draw != NULL) {
        dd2_prepared_sky_camera(angle);
        passed = dd2_sky_draw_frame(draw) && dd2_prepared_write(renderer, capture.output) &&
                 glGetError() == GL_NO_ERROR;
        const dd2_sky_draw_stats stats = dd2_sky_draw_statistics(draw);
        printf("{\"scope\":\"prepared sky diagnostic\",\"tested\":%zu,\"visible\":%zu,"
               "\"culled\":%zu,\"triangles\":%zu,\"batches\":%zu,\"textures\":%zu}\n",
               stats.tested, stats.visible, stats.culled, stats.triangles, stats.batches,
               dd2_model_texture_cache_count(cache));
    }
    dd2_sky_draw_destroy(draw);
    dd2_model_texture_cache_destroy(cache);
    dd2_renderer_destroy(renderer);
    dd2_track_destroy(track);
    return passed;
}

static bool dd2_prepared_sky_name(const char *name) {
    enum {
        DD2_PREPARED_SKY_NAME = 7,
        DD2_PREPARED_SKY_LEVEL = 4,
        DD2_PREPARED_SKY_SEPARATOR = 5,
        DD2_PREPARED_SKY_ANGLE = 6
    };
    return strlen(name) == DD2_PREPARED_SKY_NAME && strncmp(name, "sky-", 4) == 0 &&
           name[DD2_PREPARED_SKY_SEPARATOR] == '-' &&
           strchr("123456789AB", name[DD2_PREPARED_SKY_LEVEL]) != NULL &&
           strchr("01234567UD", name[DD2_PREPARED_SKY_ANGLE]) != NULL;
}

int main(int argc, char **argv) {
    if (argc != DD2_PREPARED_ARGUMENTS && argc != DD2_PREPARED_ARGUMENTS + 1) {
        return EXIT_FAILURE;
    }
    const bool crop = argc == DD2_PREPARED_ARGUMENTS + 1;
    if (crop && strcmp(argv[DD2_PREPARED_ARGUMENTS], "crop") != 0) {
        return EXIT_FAILURE;
    }
    const char *extension = strrchr(argv[2], '.');
    const char codes[] = "123456789AB";
    if (!crop && dd2_prepared_sky_name(argv[2])) {
        enum { DD2_PREPARED_SKY_LEVEL = 4, DD2_PREPARED_SKY_ANGLE = 6 };
        return dd2_prepared_sky(
                   (dd2_prepared_world_capture){
                       .root = argv[1],
                       .output = argv[3],
                       .number =
                           (unsigned)(strchr(codes, argv[2][DD2_PREPARED_SKY_LEVEL]) - codes) + 1},
                   argv[2][DD2_PREPARED_SKY_ANGLE])
                   ? EXIT_SUCCESS
                   : EXIT_FAILURE;
    }
    if (strlen(argv[2]) == 1 && strchr(codes, argv[2][0]) != NULL) {
        return dd2_prepared_world((dd2_prepared_world_capture){
                   .root = argv[1],
                   .output = argv[3],
                   .crop = crop,
                   .number = (unsigned)(strchr(codes, argv[2][0]) - codes) + 1})
                   ? EXIT_SUCCESS
                   : EXIT_FAILURE;
    }
    if (extension != NULL && strcmp(extension, ".dd2scene") == 0) {
        return dd2_prepared_world((dd2_prepared_world_capture){
                   .root = argv[1], .resource = argv[2], .output = argv[3], .crop = crop})
                   ? EXIT_SUCCESS
                   : EXIT_FAILURE;
    }
    if (crop) {
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
