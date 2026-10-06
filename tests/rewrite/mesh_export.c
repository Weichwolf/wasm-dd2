#include "archive_fixture.h"
#include "asset_fixture.h"
#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/mesh.h"
#include "assets/scene.h"
#include "assets/textures.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_EXPORT_SHAPE_FIRST = 5, DD2_EXPORT_SHAPE_END = 22 };

static bool dd2_mesh_export_word(FILE *file, uint32_t value) {
    uint8_t bytes[DD2_TEST_WORD_BYTES] = {0};
    dd2_test_write_le32(bytes, value);
    return fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes);
}

static bool dd2_mesh_export_vectors(FILE *file, const dd2_mesh_vector *vectors, size_t count) {
    for (size_t index = 0; index < count; ++index) {
        const dd2_mesh_vector vector = vectors[index];
        if (!dd2_mesh_export_word(file, (uint32_t)(int32_t)vector.x) ||
            !dd2_mesh_export_word(file, (uint32_t)(int32_t)vector.y) ||
            !dd2_mesh_export_word(file, (uint32_t)(int32_t)vector.z) ||
            !dd2_mesh_export_word(file, vector.auxiliary)) {
            return false;
        }
    }
    return true;
}

static bool dd2_mesh_export_face(FILE *file, const dd2_mesh_face *face) {
    for (size_t corner = 0; corner < DD2_MESH_CORNERS; ++corner) {
        if (!dd2_mesh_export_word(file, face->indices[corner]) ||
            !dd2_mesh_export_word(file, face->normals[corner]) ||
            !dd2_mesh_export_word(file, face->colors[corner])) {
            return false;
        }
    }
    const uint32_t words[] = {face->texture,  face->palette_bank, face->attributes,
                              face->opcode,   face->group_flags,  face->corner_count,
                              face->textured, face->lit,          face->gouraud};
    for (size_t index = 0; index < sizeof(words) / sizeof(words[0]); ++index) {
        if (!dd2_mesh_export_word(file, words[index])) {
            return false;
        }
    }
    return true;
}

static bool dd2_mesh_export_mesh(FILE *file, const dd2_mesh *mesh) {
    if (!dd2_mesh_export_word(file, (uint32_t)dd2_mesh_vertex_count(mesh)) ||
        !dd2_mesh_export_word(file, (uint32_t)dd2_mesh_normal_count(mesh)) ||
        !dd2_mesh_export_word(file, (uint32_t)dd2_mesh_face_count(mesh)) ||
        !dd2_mesh_export_vectors(file, dd2_mesh_vertices(mesh), dd2_mesh_vertex_count(mesh)) ||
        !dd2_mesh_export_vectors(file, dd2_mesh_normals(mesh), dd2_mesh_normal_count(mesh))) {
        return false;
    }
    const dd2_mesh_face *faces = dd2_mesh_faces(mesh);
    for (size_t index = 0; index < dd2_mesh_face_count(mesh); ++index) {
        if (!dd2_mesh_export_face(file, &faces[index])) {
            return false;
        }
    }
    return true;
}

static bool dd2_mesh_export_scene(FILE *file, const dd2_scene *scene) {
    if (!dd2_mesh_export_word(file, (uint32_t)dd2_scene_block_count(scene)) ||
        !dd2_mesh_export_word(file, (uint32_t)dd2_scene_object_count(scene))) {
        return false;
    }
    const dd2_scene_object *objects = dd2_scene_objects(scene);
    for (size_t index = 0; index < dd2_scene_object_count(scene); ++index) {
        if (!dd2_mesh_export_word(file, (uint32_t)objects[index].position.x) ||
            !dd2_mesh_export_word(file, (uint32_t)objects[index].position.y) ||
            !dd2_mesh_export_word(file, (uint32_t)objects[index].position.z) ||
            !dd2_mesh_export_mesh(file, objects[index].mesh)) {
            return false;
        }
    }
    return true;
}

static bool dd2_mesh_export_shapes(FILE *file, const dd2_level_data *level,
                                   dd2_mesh_limits limits) {
    for (size_t section = DD2_EXPORT_SHAPE_FIRST; section < DD2_EXPORT_SHAPE_END; ++section) {
        const dd2_byte_view bytes = level->sections[section];
        if (bytes.size == 0) {
            continue;
        }
        dd2_mesh *mesh = dd2_mesh_create(bytes, limits);
        if (mesh == NULL) {
            return false;
        }
        const bool passed =
            dd2_mesh_export_word(file, (uint32_t)section) && dd2_mesh_export_mesh(file, mesh);
        dd2_mesh_destroy(mesh);
        if (!passed) {
            return false;
        }
    }
    return dd2_mesh_export_word(file, DD2_EXPORT_SHAPE_END);
}

static bool dd2_mesh_export_level(FILE *file, const dd2_archive *archive, char code) {
    char name[] = "LEV0\\LEVEL.DAT";
    name[3] = code;
    dd2_level_data level = {0};
    if (!dd2_level_decode(dd2_find_view(archive, name), &level)) {
        return false;
    }
    const dd2_texture_sources sources = dd2_export_texture_sources(archive, code);
    dd2_texture_set *textures = dd2_texture_set_create(&sources);
    if (textures == NULL) {
        return false;
    }
    const dd2_mesh_limits limits = {.texture_definitions = level.texture_definition_count,
                                    .palette_banks = dd2_texture_palette_bank_count(textures)};
    dd2_scene *scene = dd2_scene_create(
        level.sections[DD2_LEVEL_SCENE_BLOCKS],
        (dd2_scene_options){.compressed = code >= '1' && code <= '7', .limits = limits});
    const bool passed = scene != NULL && fwrite(&code, 1, 1, file) == 1 &&
                        dd2_mesh_export_scene(file, scene) &&
                        dd2_mesh_export_shapes(file, &level, limits);
    dd2_scene_destroy(scene);
    dd2_texture_set_destroy(textures);
    return passed;
}

int main(int argc, char **argv) {
    const char allowed[] = "/tmp/wasm-dd2/";
    if (argc != 3 || strncmp(argv[2], allowed, sizeof(allowed) - 1) != 0 ||
        strstr(argv[2], "..") != NULL) {
        return EXIT_FAILURE;
    }
    dd2_archive_fixture fixture = {0};
    if (!dd2_archive_fixture_open(argv[1], &fixture)) {
        return EXIT_FAILURE;
    }
    FILE *file = fopen(argv[2], "wb");
    if (file == NULL) {
        dd2_archive_fixture_close(&fixture);
        return EXIT_FAILURE;
    }
    const char levels[] = "123456789AB";
    bool passed = true;
    for (size_t index = 0; index < sizeof(levels) - 1 && passed; ++index) {
        passed = dd2_mesh_export_level(file, fixture.archive, levels[index]);
    }
    const bool closed = fclose(file) == 0;
    dd2_archive_fixture_close(&fixture);
    if (!passed || !closed) {
        return EXIT_FAILURE;
    }
    puts("{\"scope\":\"original scene and mesh exports\",\"pass\":true}");
    return EXIT_SUCCESS;
}
