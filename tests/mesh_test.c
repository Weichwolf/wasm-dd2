#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/lz.h"
#include "assets/mesh.h"
#include "assets/scene.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_TEST_VERTEX_COUNT_OFFSET = 10,
    DD2_TEST_NORMAL_COUNT_OFFSET = 12,
    DD2_TEST_VERTICES_OFFSET = 32,
    DD2_TEST_NORMALS_OFFSET = 36,
    DD2_TEST_POLYGONS_OFFSET = 40,
    DD2_TEST_FACE_VERTICES = 12,
    DD2_TEST_SCENE_INSTANCE = 8,
    DD2_TEST_SCENE_X = 12,
    DD2_TEST_SCENE_Y = 16,
    DD2_TEST_SCENE_Z = 20,
    DD2_TEST_MESH_BYTES = 160,
    DD2_TEST_MESH_HEADER = 44,
    DD2_TEST_MESH_NORMALS = 76,
    DD2_TEST_MESH_POLYGONS = 108,
    DD2_TEST_MESH_FACE = 112,
    DD2_TEST_TEXTURE_QUAD = 12,
    DD2_TEST_LIT_QUAD = 29,
    DD2_TEST_SPRITE_QUAD = 34,
    DD2_TEST_SPRITE_STRIDE = 20,
    DD2_TEST_LIT_STRIDE = 28,
    DD2_TEST_VECTOR_BYTES = 8,
    DD2_TEST_X = 100,
    DD2_TEST_Y = 200,
    DD2_TEST_Z = 300,
    DD2_TEST_SCENE_MESH = 24,
    DD2_TEST_SCENE_BYTES = DD2_TEST_SCENE_MESH + DD2_TEST_MESH_BYTES,
    DD2_TEST_OVERLAP_LENGTH = 10,
    DD2_TEST_LZ_SOURCE_BYTES = 8,
    DD2_TEST_LZ_DISTANCE_LOW = 255,
    DD2_TEST_LZ_DISTANCE_HIGH = 246
};

static void dd2_test_mesh_fixture(uint8_t *bytes, unsigned opcode) {
    for (size_t index = 0; index < DD2_TEST_MESH_BYTES; ++index) {
        bytes[index] = 0;
    }
    dd2_test_write_le16(bytes + DD2_TEST_VERTEX_COUNT_OFFSET, 4);
    dd2_test_write_le16(bytes + DD2_TEST_NORMAL_COUNT_OFFSET, 4);
    dd2_test_write_le32(bytes + DD2_TEST_VERTICES_OFFSET, DD2_TEST_MESH_HEADER);
    dd2_test_write_le32(bytes + DD2_TEST_NORMALS_OFFSET, DD2_TEST_MESH_NORMALS);
    dd2_test_write_le32(bytes + DD2_TEST_POLYGONS_OFFSET, DD2_TEST_MESH_POLYGONS);
    for (size_t vertex = 0; vertex < DD2_MESH_CORNERS; ++vertex) {
        uint8_t *data = bytes + DD2_TEST_MESH_HEADER + (vertex * DD2_TEST_VECTOR_BYTES);
        dd2_test_write_le16(data, (uint16_t)-DD2_TEST_X);
        dd2_test_write_le16(data + 2, (uint16_t)INT16_MIN);
        dd2_test_write_le16(data + 4, INT16_MAX);
    }
    dd2_test_write_le16(bytes + DD2_TEST_MESH_POLYGONS, 1);
    bytes[DD2_TEST_MESH_POLYGONS + 2] = (uint8_t)opcode;
    bytes[DD2_TEST_MESH_POLYGONS + 3] = 1;
    dd2_test_write_le32(bytes + DD2_TEST_MESH_FACE + 4, DD2_TEST_TEXEL_A);
    for (size_t corner = 0; corner < DD2_MESH_CORNERS; ++corner) {
        dd2_test_write_le16(bytes + DD2_TEST_MESH_FACE + DD2_TEST_FACE_VERTICES + (corner * 2),
                            (uint16_t)corner);
        if (opcode == DD2_TEST_LIT_QUAD) {
            dd2_test_write_le16(bytes + DD2_TEST_MESH_FACE + DD2_TEST_SPRITE_STRIDE + (corner * 2),
                                (uint16_t)corner);
        }
    }
}

static bool dd2_test_lz(void) {
    uint8_t source[DD2_TEST_LZ_SOURCE_BYTES] = {
        DD2_TEST_OVERLAP_LENGTH,  0, 0, 0, 1, 'A', DD2_TEST_LZ_DISTANCE_LOW,
        DD2_TEST_LZ_DISTANCE_HIGH};
    uint8_t output[DD2_TEST_OVERLAP_LENGTH] = {0};
    const dd2_byte_buffer buffer = {.data = output, .size = sizeof(output)};
    const dd2_byte_view bytes = {.data = source, .size = sizeof(source)};
    size_t written = 0;
    if (!dd2_lz_decode(bytes, buffer, &written) || written != sizeof(output)) {
        return false;
    }
    for (size_t index = 0; index < sizeof(output); ++index) {
        if (output[index] != 'A') {
            return false;
        }
    }
    if (dd2_lz_decode(bytes, (dd2_byte_buffer){.data = output, .size = sizeof(output) - 1},
                      &written) ||
        written != 0 || dd2_lz_decode(bytes, buffer, NULL) ||
        dd2_lz_decode((dd2_byte_view){0}, buffer, &written) ||
        dd2_lz_decode(bytes, (dd2_byte_buffer){0}, &written) ||
        dd2_lz_decode((dd2_byte_view){.data = source, .size = sizeof(source) - 1}, buffer,
                      &written)) {
        return false;
    }
    source[4] = 0;
    if (dd2_lz_decode(bytes, buffer, &written) || written != 0) {
        return false;
    }
    source[4] = 1;
    source[0] = 2;
    if (dd2_lz_decode(bytes, buffer, &written)) {
        return false;
    }
    source[0] = 0;
    return dd2_lz_decode(bytes, buffer, &written) && written == 0;
}

static bool dd2_test_mesh_valid(uint8_t *bytes, unsigned opcode) {
    dd2_test_mesh_fixture(bytes, opcode);
    dd2_mesh *mesh =
        dd2_mesh_create((dd2_byte_view){.data = bytes, .size = DD2_TEST_MESH_BYTES},
                        (dd2_mesh_limits){.texture_definitions = 1, .palette_banks = 1});
    if (mesh == NULL) {
        return false;
    }
    for (size_t index = 0; index < DD2_TEST_MESH_BYTES; ++index) {
        bytes[index] = 0;
    }
    const dd2_mesh_vector *vertices = dd2_mesh_vertices(mesh);
    const dd2_mesh_face *faces = dd2_mesh_faces(mesh);
    const bool passed = dd2_mesh_vertex_count(mesh) == 4 && dd2_mesh_normal_count(mesh) == 4 &&
                        dd2_mesh_normals(mesh) != NULL && dd2_mesh_face_count(mesh) == 1 &&
                        vertices[0].x == -DD2_TEST_X && vertices[0].y == INT16_MIN &&
                        vertices[0].z == INT16_MAX && faces[0].opcode == opcode &&
                        faces[0].corner_count == 4 && faces[0].textured &&
                        faces[0].indices[3] == 3 && faces[0].colors[3] == DD2_TEST_TEXEL_A &&
                        (opcode != DD2_TEST_LIT_QUAD ||
                         (faces[0].lit && faces[0].gouraud && faces[0].normals[3] == 3));
    dd2_mesh_destroy(mesh);
    return passed;
}

static bool dd2_expect_bad_mesh(dd2_byte_view bytes) {
    dd2_mesh *mesh =
        dd2_mesh_create(bytes, (dd2_mesh_limits){.texture_definitions = 1, .palette_banks = 1});
    const bool rejected = mesh == NULL;
    dd2_mesh_destroy(mesh);
    return rejected;
}

static bool dd2_test_mesh_bounds(void) {
    uint8_t bytes[DD2_TEST_MESH_BYTES] = {0};
    const dd2_byte_view view = {.data = bytes, .size = sizeof(bytes)};
    if (!dd2_test_mesh_valid(bytes, DD2_TEST_TEXTURE_QUAD) ||
        !dd2_test_mesh_valid(bytes, DD2_TEST_LIT_QUAD) ||
        !dd2_test_mesh_valid(bytes, DD2_TEST_SPRITE_QUAD) ||
        !dd2_expect_bad_mesh((dd2_byte_view){0})) {
        return false;
    }
    dd2_test_mesh_fixture(bytes, DD2_TEST_TEXTURE_QUAD);
    if (!dd2_expect_bad_mesh((dd2_byte_view){.data = bytes, .size = DD2_TEST_MESH_HEADER - 1})) {
        return false;
    }
    bytes[DD2_TEST_MESH_POLYGONS + 2] = DD2_MESH_OPCODE_COUNT;
    if (!dd2_expect_bad_mesh(view)) {
        return false;
    }
    dd2_test_mesh_fixture(bytes, DD2_TEST_TEXTURE_QUAD);
    dd2_test_write_le16(bytes + DD2_TEST_MESH_FACE + DD2_TEST_FACE_VERTICES, 4);
    if (!dd2_expect_bad_mesh(view)) {
        return false;
    }
    dd2_test_mesh_fixture(bytes, DD2_TEST_LIT_QUAD);
    dd2_test_write_le16(bytes + DD2_TEST_MESH_FACE + DD2_TEST_SPRITE_STRIDE, 4);
    if (!dd2_expect_bad_mesh(view)) {
        return false;
    }
    dd2_test_mesh_fixture(bytes, DD2_TEST_TEXTURE_QUAD);
    dd2_test_write_le16(bytes + DD2_TEST_MESH_FACE + DD2_TEST_VECTOR_BYTES, 1);
    if (!dd2_expect_bad_mesh(view)) {
        return false;
    }
    dd2_test_mesh_fixture(bytes, DD2_TEST_TEXTURE_QUAD);
    dd2_test_write_le32(bytes + DD2_TEST_VERTICES_OFFSET, UINT32_MAX);
    return dd2_expect_bad_mesh(view);
}

static void dd2_test_scene_fixture(uint8_t *bytes) {
    dd2_test_write_le32(bytes, 4);
    dd2_test_write_le32(bytes + 4, 1);
    dd2_test_write_le32(bytes + DD2_TEST_SCENE_INSTANCE, DD2_TEST_SCENE_MESH - 4);
    dd2_test_write_le32(bytes + DD2_TEST_SCENE_X, (uint32_t)-DD2_TEST_X);
    dd2_test_write_le32(bytes + DD2_TEST_SCENE_Y, DD2_TEST_Y);
    dd2_test_write_le32(bytes + DD2_TEST_SCENE_Z, DD2_TEST_Z);
    dd2_test_mesh_fixture(bytes + DD2_TEST_SCENE_MESH, DD2_TEST_TEXTURE_QUAD);
}

static bool dd2_test_scene_origins(void) {
    typedef struct {
        int32_t coordinate;
        int32_t center;
    } dd2_origin_case;
    /* Cover truncation versus floor for negative values, exact cell boundaries
     * and full signed-coordinate limits without relying on signed shifts. */
    const dd2_origin_case cases[] = {{0, 16384},
                                     {-1, -16384},
                                     {32767, 16384},
                                     {32768, 49152},
                                     {-32768, -16384},
                                     {-32769, -49152},
                                     {INT32_MIN, INT32_MIN + 16384},
                                     {INT32_MAX, INT32_MAX - 16383}};
    const dd2_scene_options options = {.limits = {.texture_definitions = 1, .palette_banks = 1}};
    uint8_t bytes[DD2_TEST_SCENE_BYTES] = {0};
    dd2_test_scene_fixture(bytes);
    for (unsigned local = 0; local < 2; ++local) {
        const uint8_t flags = (uint8_t)(1U | (local != 0 ? DD2_MESH_LOCAL_ORIGIN : 0U));
        bytes[DD2_TEST_SCENE_MESH + 4] = flags;
        for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
            const int32_t coordinate = cases[index].coordinate;
            dd2_test_write_le32(bytes + DD2_TEST_SCENE_X, (uint32_t)coordinate);
            dd2_test_write_le32(bytes + DD2_TEST_SCENE_Y, (uint32_t)coordinate);
            dd2_test_write_le32(bytes + DD2_TEST_SCENE_Z, (uint32_t)coordinate);
            dd2_scene *scene =
                dd2_scene_create((dd2_byte_view){.data = bytes, .size = sizeof(bytes)}, options);
            if (scene == NULL) {
                return false;
            }
            const dd2_scene_object *objects = dd2_scene_objects(scene);
            const int32_t expected = local != 0 ? coordinate : cases[index].center;
            const bool passed =
                objects[0].position.x == coordinate && objects[0].position.y == coordinate &&
                objects[0].position.z == coordinate && objects[0].origin.x == expected &&
                objects[0].origin.y == expected && objects[0].origin.z == expected &&
                dd2_mesh_flags(objects[0].mesh) == flags;
            dd2_scene_destroy(scene);
            if (!passed) {
                return false;
            }
        }
    }
    return dd2_mesh_flags(NULL) == 0;
}

static bool dd2_test_scene(void) {
    uint8_t bytes[DD2_TEST_SCENE_BYTES] = {0};
    dd2_test_scene_fixture(bytes);
    const dd2_scene_options options = {.limits = {.texture_definitions = 1, .palette_banks = 1}};
    const dd2_byte_view view = {.data = bytes, .size = sizeof(bytes)};
    dd2_scene *scene = dd2_scene_create(view, options);
    if (scene == NULL) {
        return false;
    }
    const dd2_scene_object *objects = dd2_scene_objects(scene);
    const bool passed =
        dd2_scene_block_count(scene) == 1 && dd2_scene_object_count(scene) == 1 &&
        objects[0].position.x == -DD2_TEST_X && objects[0].position.y == DD2_TEST_Y &&
        objects[0].position.z == DD2_TEST_Z && dd2_mesh_face_count(objects[0].mesh) == 1;
    dd2_scene_destroy(scene);
    dd2_test_write_le32(bytes + DD2_TEST_SCENE_INSTANCE, UINT32_MAX);
    scene = dd2_scene_create(view, options);
    const bool rejected = scene == NULL;
    dd2_scene_destroy(scene);
    scene = dd2_scene_create((dd2_byte_view){0}, options);
    const bool empty = scene != NULL && dd2_scene_object_count(scene) == 0;
    dd2_scene_destroy(scene);
    dd2_scene_destroy(NULL);
    dd2_mesh_destroy(NULL);
    return passed && rejected && empty && dd2_scene_object_count(NULL) == 0 &&
           dd2_scene_objects(NULL) == NULL && dd2_mesh_face_count(NULL) == 0 &&
           dd2_mesh_vertices(NULL) == NULL && dd2_mesh_faces(NULL) == NULL;
}

int main(void) {
    if (!dd2_test_lz() || !dd2_test_mesh_bounds() || !dd2_test_scene() ||
        !dd2_test_scene_origins()) {
        return EXIT_FAILURE;
    }
    puts("{\"scope\":\"object decompression, typed meshes and scene ownership\",\"pass\":true}");
    return EXIT_SUCCESS;
}
