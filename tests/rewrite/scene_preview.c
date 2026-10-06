#include "archive_fixture.h"
#include "assets/archive.h"
#include "assets/level.h"
#include "assets/mesh.h"
#include "assets/scene.h"
#include "assets/textures.h"
#include "render/mesh_draw.h"
#include "render/renderer.h"

#include <GL/softgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_PREVIEW_ARGUMENTS = 5,
    DD2_PREVIEW_WIDTH = 640,
    DD2_PREVIEW_HEIGHT = 480,
    DD2_PREVIEW_RGBA_CHANNELS = 4,
    DD2_PREVIEW_RGB_CHANNELS = 3,
    DD2_PREVIEW_MIN_PIXELS = 100
};
static const float dd2_preview_pitch = 55.0F;
static const float dd2_preview_yaw = 30.0F;
static const float dd2_preview_padding = 1.2F;

typedef struct {
    float min[3];
    float max[3];
    bool valid;
} dd2_preview_bounds;

static void dd2_preview_mesh_bounds(dd2_preview_bounds *bounds, const dd2_mesh *mesh,
                                    dd2_track_vertex position) {
    const dd2_mesh_vector *vertices = dd2_mesh_vertices(mesh);
    for (size_t index = 0; index < dd2_mesh_vertex_count(mesh); ++index) {
        const float values[] = {(float)position.x + (float)vertices[index].x,
                                (float)position.y + (float)vertices[index].y,
                                (float)position.z + (float)vertices[index].z};
        for (size_t axis = 0; axis < 3; ++axis) {
            if (!bounds->valid || values[axis] < bounds->min[axis]) {
                bounds->min[axis] = values[axis];
            }
            if (!bounds->valid || values[axis] > bounds->max[axis]) {
                bounds->max[axis] = values[axis];
            }
        }
        bounds->valid = true;
    }
}

static void dd2_preview_camera(dd2_preview_bounds bounds) {
    float radius = 1;
    for (size_t axis = 0; axis < 3; ++axis) {
        const float span = bounds.max[axis] - bounds.min[axis];
        if (span > radius) {
            radius = span;
        }
    }
    radius *= dd2_preview_padding / 2;
    const float aspect = (float)DD2_PREVIEW_WIDTH / (float)DD2_PREVIEW_HEIGHT;
    glViewport(0, 0, DD2_PREVIEW_WIDTH, DD2_PREVIEW_HEIGHT);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-radius * aspect, radius * aspect, -radius, radius, -radius * 4, radius * 4);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(dd2_preview_pitch, 1.0F, 0.0F, 0.0F);
    glRotatef(dd2_preview_yaw, 0.0F, 1.0F, 0.0F);
    glTranslatef(-(bounds.min[0] + bounds.max[0]) / 2, -(bounds.min[1] + bounds.max[1]) / 2,
                 -(bounds.min[2] + bounds.max[2]) / 2);
}

static bool dd2_preview_image(dd2_renderer *renderer, const char *path) {
    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    if (pixels == NULL) {
        return false;
    }
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    const char header[] = "P6\n640 480\n255\n";
    bool passed = fwrite(header, 1, sizeof(header) - 1, file) == sizeof(header) - 1;
    size_t visible = 0;
    uint8_t row[DD2_PREVIEW_WIDTH * DD2_PREVIEW_RGB_CHANNELS] = {0};
    for (size_t y_pos = 0; y_pos < DD2_PREVIEW_HEIGHT && passed; ++y_pos) {
        for (size_t x_pos = 0; x_pos < DD2_PREVIEW_WIDTH; ++x_pos) {
            const size_t source = (((DD2_PREVIEW_HEIGHT - y_pos - 1) * DD2_PREVIEW_WIDTH) + x_pos) *
                                  DD2_PREVIEW_RGBA_CHANNELS;
            for (size_t channel = 0; channel < DD2_PREVIEW_RGB_CHANNELS; ++channel) {
                row[(x_pos * DD2_PREVIEW_RGB_CHANNELS) + channel] = pixels[source + channel];
            }
            visible += pixels[source] != 0 || pixels[source + 1] != 0 || pixels[source + 2] != 0;
        }
        passed = fwrite(row, 1, sizeof(row), file) == sizeof(row);
    }
    const bool closed = fclose(file) == 0;
    printf("{\"scope\":\"static original mesh preview\",\"visible_pixels\":%zu}\n", visible);
    return passed && closed && visible >= DD2_PREVIEW_MIN_PIXELS;
}

static bool dd2_preview_draw(const dd2_level_data *level, const dd2_texture_set *textures,
                             const dd2_scene *scene, const dd2_mesh *car, const char *path) {
    dd2_preview_bounds bounds = {0};
    if (car != NULL) {
        dd2_preview_mesh_bounds(&bounds, car, (dd2_track_vertex){0});
    } else {
        const dd2_scene_object *objects = dd2_scene_objects(scene);
        for (size_t index = 0; index < dd2_scene_object_count(scene); ++index) {
            dd2_preview_mesh_bounds(&bounds, objects[index].mesh, objects[index].position);
        }
    }
    if (!bounds.valid) {
        return false;
    }
    dd2_renderer *renderer = dd2_renderer_create(
        &(dd2_render_options){.width = DD2_PREVIEW_WIDTH, .height = DD2_PREVIEW_HEIGHT});
    if (renderer == NULL) {
        return false;
    }
    dd2_mesh_materials *materials = dd2_mesh_materials_create(level, textures);
    dd2_preview_camera(bounds);
    const bool drawn = car != NULL ? dd2_mesh_draw(materials, car, (dd2_track_vertex){0})
                                   : dd2_scene_draw(materials, scene);
    const bool passed = drawn && dd2_preview_image(renderer, path);
    dd2_mesh_materials_destroy(materials);
    dd2_renderer_destroy(renderer);
    return passed;
}

typedef struct {
    const char *output;
    char code;
    bool car;
} dd2_preview_options;
static bool dd2_preview_level(const dd2_archive *archive, dd2_preview_options options) {
    char name[] = "LEV0\\LEVEL.DAT";
    name[3] = options.code;
    dd2_level_data level = {0};
    if (!dd2_level_decode(dd2_find_view(archive, name), &level)) {
        return false;
    }
    const dd2_texture_sources sources = dd2_export_texture_sources(archive, options.code);
    dd2_texture_set *textures = dd2_texture_set_create(&sources);
    if (textures == NULL) {
        return false;
    }
    const dd2_mesh_limits limits = {.texture_definitions = level.texture_definition_count,
                                    .palette_banks = dd2_texture_palette_bank_count(textures)};
    dd2_scene *scene = NULL;
    dd2_mesh *car = NULL;
    if (options.car) {
        car = dd2_mesh_create(level.sections[DD2_LEVEL_CAR_HIGH], limits);
    } else {
        scene = dd2_scene_create(
            level.sections[DD2_LEVEL_SCENE_BLOCKS],
            (dd2_scene_options){.compressed = options.code >= '1' && options.code <= '7',
                                .limits = limits});
    }
    const bool passed = (scene != NULL || car != NULL) &&
                        dd2_preview_draw(&level, textures, scene, car, options.output);
    dd2_scene_destroy(scene);
    dd2_mesh_destroy(car);
    dd2_texture_set_destroy(textures);
    return passed;
}

int main(int argc, char **argv) {
    const char allowed[] = "/tmp/wasm-dd2/";
    if (argc != DD2_PREVIEW_ARGUMENTS || strncmp(argv[2], allowed, sizeof(allowed) - 1) != 0 ||
        strstr(argv[2], "..") != NULL || strlen(argv[3]) != 1 ||
        strchr("123456789AB", argv[3][0]) == NULL ||
        (strcmp(argv[4], "scene") != 0 && strcmp(argv[4], "car") != 0)) {
        return EXIT_FAILURE;
    }
    dd2_archive_fixture fixture = {0};
    if (!dd2_archive_fixture_open(argv[1], &fixture)) {
        return EXIT_FAILURE;
    }
    const bool passed = dd2_preview_level(
        fixture.archive, (dd2_preview_options){.output = argv[2],
                                               .code = argv[3][0],
                                               .car = strcmp(argv[4], "car") == 0});
    dd2_archive_fixture_close(&fixture);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
