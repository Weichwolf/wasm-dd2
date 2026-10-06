#include "asset_fixture.h"
#include "assets/bytes.h"
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

enum {
    DD2_DRAW_SIDE = 64,
    DD2_DRAW_SHAPE_BYTES = 104,
    DD2_DRAW_VERTEX_COUNT = 10,
    DD2_DRAW_NORMAL_COUNT = 12,
    DD2_DRAW_VERTEX_OFFSET = 32,
    DD2_DRAW_NORMAL_OFFSET = 36,
    DD2_DRAW_POLYGON_OFFSET = 40,
    DD2_DRAW_VERTICES = 44,
    DD2_DRAW_POLYGONS = 76,
    DD2_DRAW_FACE = 80,
    DD2_DRAW_VECTOR_BYTES = 8,
    DD2_DRAW_TEXTURED_QUAD = 12,
    DD2_DRAW_PLAIN_QUAD = 4,
    DD2_DRAW_FACE_TEXTURE = 8,
    DD2_DRAW_FACE_INDICES = 12,
    DD2_DRAW_DEFINITION_BYTES = 12,
    DD2_DRAW_DEFINITIONS = 3,
    DD2_DRAW_DEFINITIONS_BYTES = 4 + (DD2_DRAW_DEFINITIONS * DD2_DRAW_DEFINITION_BYTES),
    DD2_DRAW_SCENE_MESH = 24,
    DD2_DRAW_SCENE_INSTANCE = 2 * DD2_TEST_WORD_BYTES,
    DD2_DRAW_SCENE_BYTES = DD2_DRAW_SCENE_MESH + DD2_DRAW_SHAPE_BYTES,
    DD2_DRAW_CELL_CENTER = 16384,
    DD2_DRAW_GREEN = 0x00ff00
};

typedef struct {
    bool textured;
    unsigned palette_bank;
    uint16_t texture;
} dd2_draw_shape;

static void dd2_draw_source(uint8_t *bytes, dd2_draw_shape shape) {
    const bool textured = shape.textured;
    dd2_test_write_le16(bytes + DD2_DRAW_VERTEX_COUNT, DD2_MESH_CORNERS);
    dd2_test_write_le16(bytes + DD2_DRAW_NORMAL_COUNT, 0);
    dd2_test_write_le32(bytes + DD2_DRAW_VERTEX_OFFSET, DD2_DRAW_VERTICES);
    dd2_test_write_le32(bytes + DD2_DRAW_NORMAL_OFFSET, DD2_DRAW_POLYGONS);
    dd2_test_write_le32(bytes + DD2_DRAW_POLYGON_OFFSET, DD2_DRAW_POLYGONS);
    for (size_t corner = 0; corner < DD2_MESH_CORNERS; ++corner) {
        uint8_t *vertex = bytes + DD2_DRAW_VERTICES + (corner * DD2_DRAW_VECTOR_BYTES);
        dd2_test_write_le16(vertex, (uint16_t)(corner % 2 == 0 ? -1 : 1));
        dd2_test_write_le16(vertex + 2, (uint16_t)(corner < 2 ? -1 : 1));
        dd2_test_write_le16(bytes + DD2_DRAW_FACE +
                                (textured ? DD2_DRAW_FACE_INDICES : DD2_DRAW_FACE_TEXTURE) +
                                (corner * 2),
                            (uint16_t)corner);
    }
    dd2_test_write_le16(bytes + DD2_DRAW_POLYGONS, 1);
    bytes[DD2_DRAW_POLYGONS + 2] = textured ? DD2_DRAW_TEXTURED_QUAD : DD2_DRAW_PLAIN_QUAD;
    bytes[DD2_DRAW_POLYGONS + 3] = 1;
    dd2_test_write_le32(bytes + DD2_DRAW_FACE + 4, DD2_DRAW_GREEN);
    if (textured) {
        dd2_test_write_le16(bytes + DD2_DRAW_FACE + DD2_DRAW_FACE_TEXTURE, shape.texture);
        dd2_test_write_le16(bytes + DD2_DRAW_FACE + DD2_DRAW_FACE_INDICES - 2,
                            (uint16_t)shape.palette_bank);
    }
}

static dd2_mesh *dd2_draw_fixture(dd2_draw_shape shape) {
    uint8_t bytes[DD2_DRAW_SHAPE_BYTES] = {0};
    dd2_draw_source(bytes, shape);
    return dd2_mesh_create(
        (dd2_byte_view){.data = bytes, .size = sizeof(bytes)},
        (dd2_mesh_limits){.texture_definitions = DD2_DRAW_DEFINITIONS, .palette_banks = 2});
}

static bool dd2_draw_pixels(dd2_renderer *renderer, bool green) {
    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    if (pixels == NULL) {
        return false;
    }
    for (size_t row = 0; row < DD2_DRAW_SIDE; ++row) {
        for (size_t column = 0; column < DD2_DRAW_SIDE; ++column) {
            const size_t index = ((row * DD2_DRAW_SIDE) + column) * DD2_TEST_COLOR_CHANNELS;
            const bool opaque = column < DD2_DRAW_SIDE / 2;
            const uint8_t red = green || !opaque ? 0 : DD2_TEST_TEXEL_A;
            const uint8_t blue = green || !opaque ? 0 : DD2_TEST_PALETTE_BLUE;
            const uint8_t mapped_green = opaque ? UINT8_MAX - DD2_TEST_TEXEL_A : 0;
            const uint8_t channel = green ? UINT8_MAX : mapped_green;
            if (pixels[index] != red || pixels[index + 1] != channel || pixels[index + 2] != blue ||
                pixels[index + 3] != UINT8_MAX) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_draw_opaque_pixels(dd2_renderer *renderer) {
    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    bool passed = pixels != NULL;
    for (size_t index = 0; index < (size_t)DD2_DRAW_SIDE * DD2_DRAW_SIDE && passed; ++index) {
        const size_t offset = index * DD2_TEST_COLOR_CHANNELS;
        passed = pixels[offset] == 1 && pixels[offset + 1] == UINT8_MAX - 1 &&
                 pixels[offset + 2] == DD2_TEST_PALETTE_BLUE && pixels[offset + 3] == UINT8_MAX;
    }
    return passed;
}

static bool dd2_draw_opaque_probe(dd2_renderer *renderer, dd2_mesh_materials *materials) {
    dd2_mesh *mesh = dd2_draw_fixture((dd2_draw_shape){.textured = true, .texture = 1});
    if (mesh == NULL) {
        return false;
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const bool passed =
        dd2_mesh_draw(materials, mesh, (dd2_track_vertex){0}) && dd2_draw_opaque_pixels(renderer);
    dd2_mesh_destroy(mesh);
    return passed;
}

static bool dd2_draw_scene_probe(dd2_renderer *renderer, dd2_mesh_materials *materials) {
    uint8_t bytes[DD2_DRAW_SCENE_BYTES] = {0};
    dd2_test_write_le32(bytes, 4);
    dd2_test_write_le32(bytes + 4, 1);
    dd2_test_write_le32(bytes + DD2_DRAW_SCENE_INSTANCE, DD2_DRAW_SCENE_MESH - 4);
    dd2_draw_source(bytes + DD2_DRAW_SCENE_MESH, (dd2_draw_shape){.textured = true, .texture = 1});
    const dd2_scene_options options = {
        .limits = {.texture_definitions = DD2_DRAW_DEFINITIONS, .palette_banks = 2}};
    for (unsigned local = 0; local < 2; ++local) {
        bytes[DD2_DRAW_SCENE_MESH + 4] = local != 0 ? DD2_MESH_LOCAL_ORIGIN : 0;
        dd2_scene *scene =
            dd2_scene_create((dd2_byte_view){.data = bytes, .size = sizeof(bytes)}, options);
        if (scene == NULL) {
            return false;
        }
        const float center = local != 0 ? 0 : DD2_DRAW_CELL_CENTER;
        glPushMatrix();
        glLoadIdentity();
        glTranslatef(-center, -center, -center);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const bool passed = dd2_scene_draw(materials, scene) && dd2_draw_opaque_pixels(renderer);
        glPopMatrix();
        dd2_scene_destroy(scene);
        if (!passed) {
            return false;
        }
    }
    return true;
}

static bool dd2_draw_cache_probe(dd2_renderer *renderer, dd2_mesh_materials *materials) {
    dd2_mesh *other =
        dd2_draw_fixture((dd2_draw_shape){.textured = true, .palette_bank = 1, .texture = 2});
    dd2_mesh *mesh = dd2_draw_fixture((dd2_draw_shape){.textured = true, .texture = 2});
    if (other == NULL || mesh == NULL) {
        dd2_mesh_destroy(other);
        dd2_mesh_destroy(mesh);
        return false;
    }
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2, 2, -1, 1, -2, 2);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const bool drawn = dd2_mesh_draw(materials, mesh, (dd2_track_vertex){.x = -1}) &&
                       dd2_mesh_draw(materials, other, (dd2_track_vertex){.x = 1});
    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    bool passed = drawn && pixels != NULL;
    for (size_t row = 0; row < DD2_DRAW_SIDE && passed; ++row) {
        for (size_t column = 0; column < DD2_DRAW_SIDE; ++column) {
            const size_t index = ((row * DD2_DRAW_SIDE) + column) * DD2_TEST_COLOR_CHANNELS;
            const uint8_t red = (uint8_t)(DD2_TEST_TEXEL_A + (column < DD2_DRAW_SIDE / 2 ? 0 : 1));
            passed = passed && pixels[index] == red && pixels[index + 1] == UINT8_MAX - red &&
                     pixels[index + 2] == DD2_TEST_PALETTE_BLUE && pixels[index + 3] == UINT8_MAX;
        }
    }
    dd2_mesh_destroy(mesh);
    dd2_mesh_destroy(other);
    return passed;
}

static bool dd2_draw_probe(const dd2_texture_set *textures) {
    uint8_t definition[DD2_DRAW_DEFINITIONS_BYTES] = {0};
    dd2_test_write_le32(definition, DD2_DRAW_DEFINITIONS);
    for (size_t corner = 0; corner < DD2_MESH_CORNERS; ++corner) {
        definition[DD2_DRAW_FACE_TEXTURE + (corner * 2)] =
            (uint8_t)(DD2_TEXTURE_PAGE_SIDE - (corner % 2 == 0 ? 2 : 1));
        definition[DD2_DRAW_FACE_TEXTURE + (corner * 2) + 1] = DD2_TEXTURE_PAGE_SIDE - 1;
    }
    for (size_t corner = 0; corner < DD2_MESH_CORNERS; ++corner) {
        const size_t offset =
            DD2_DRAW_FACE_TEXTURE + ((size_t)2 * DD2_DRAW_DEFINITION_BYTES) + (corner * 2);
        definition[offset] = DD2_TEXTURE_PAGE_SIDE - 2;
        definition[offset + 1] = DD2_TEXTURE_PAGE_SIDE - 1;
    }
    const dd2_level_data level = {
        .texture_definition_count = DD2_DRAW_DEFINITIONS,
        .sections = {
            [DD2_LEVEL_TEXTURE_DEFINITIONS] = {.data = definition, .size = sizeof(definition)}}};
    dd2_mesh *mesh = dd2_draw_fixture((dd2_draw_shape){.textured = true});
    dd2_mesh *plain = dd2_draw_fixture((dd2_draw_shape){0});
    dd2_renderer *renderer =
        dd2_renderer_create(&(dd2_render_options){.width = DD2_DRAW_SIDE, .height = DD2_DRAW_SIDE});
    if (mesh == NULL || plain == NULL || renderer == NULL) {
        dd2_mesh_destroy(mesh);
        dd2_mesh_destroy(plain);
        dd2_renderer_destroy(renderer);
        return false;
    }
    dd2_mesh_materials *materials = dd2_mesh_materials_create(&level, textures);
    glViewport(0, 0, DD2_DRAW_SIDE, DD2_DRAW_SIDE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -2, 2);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    bool passed =
        dd2_mesh_draw(materials, mesh, (dd2_track_vertex){0}) && dd2_draw_pixels(renderer, false);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    passed = passed && dd2_mesh_draw(materials, plain, (dd2_track_vertex){.z = 1}) &&
             dd2_mesh_draw(materials, mesh, (dd2_track_vertex){0}) &&
             dd2_draw_pixels(renderer, true);
    passed = passed && dd2_draw_opaque_probe(renderer, materials) &&
             dd2_draw_scene_probe(renderer, materials) && dd2_draw_cache_probe(renderer, materials);
    dd2_mesh_materials_destroy(materials);
    passed =
        passed && glGetError() == GL_NO_ERROR && !dd2_mesh_draw(NULL, mesh, (dd2_track_vertex){0});
    dd2_renderer_destroy(renderer);
    dd2_mesh_destroy(mesh);
    dd2_mesh_destroy(plain);
    return passed;
}

int main(void) {
    dd2_test_texture_fixture fixture = {0};
    const dd2_texture_sources sources = dd2_test_texture_sources(&fixture);
    fixture.cluts[(size_t)DD2_TEST_NEUTRAL_SHADE * DD2_TEXTURE_PAGE_SIDE] = 1;
    fixture.cluts[(DD2_TEST_NEUTRAL_SHADE * DD2_TEXTURE_PAGE_SIDE) + DD2_TEST_TEXEL_B] = 0;
    dd2_texture_set *textures = dd2_texture_set_create(&sources);
    if (textures == NULL) {
        return EXIT_FAILURE;
    }
    const bool passed = dd2_draw_probe(textures);
    dd2_texture_set_destroy(textures);
    if (!passed) {
        return EXIT_FAILURE;
    }
    puts("{\"scope\":\"decoded mesh quad order, UVs, cutout, depth and texture "
         "lifetime\",\"pass\":true}");
    return EXIT_SUCCESS;
}
