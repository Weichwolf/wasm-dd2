#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/model.h"
#include "model_fixture.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool dd2_model_test_valid(uint8_t *bytes) {
    dd2_model *model = dd2_model_create((dd2_byte_view){bytes, DD2_MODEL_TEST_BYTES});
    if (model == NULL) {
        return false;
    }
    dd2_model_test_clear(bytes);
    const bool passed =
        dd2_model_texture_count(model) == 1 && dd2_model_material_count(model) == 1 &&
        dd2_model_part_count(model) == 1 && dd2_model_vertex_count(model) == 3 &&
        dd2_model_index_count(model) == 3 &&
        strcmp(dd2_model_textures(model)[0].paths[0], "textures/paint-flake-albedo.png") == 0 &&
        dd2_model_materials(model)[0].roughness == 0.5F &&
        strcmp(dd2_model_parts(model)[0].name, "Body.Panel") == 0 &&
        dd2_model_parts(model)[0].role == DD2_MODEL_EXTERIOR &&
        dd2_model_vertices(model)[1].position[0] == 1 &&
        dd2_model_vertices(model)[2].normal[2] == 1 && dd2_model_indices(model)[2] == 2;
    dd2_model_destroy(model);
    return passed;
}

typedef struct {
    size_t offset;
    uint32_t value;
} dd2_model_test_corruption;

int main(void) {
    if (dd2_model_create((dd2_byte_view){0}) != NULL || dd2_model_vertex_count(NULL) != 0 ||
        dd2_model_part_count(NULL) != 0 || dd2_model_material_count(NULL) != 0 ||
        dd2_model_texture_count(NULL) != 0 || dd2_model_index_count(NULL) != 0 ||
        dd2_model_vertices(NULL) != NULL || dd2_model_parts(NULL) != NULL ||
        dd2_model_textures(NULL) != NULL || dd2_model_materials(NULL) != NULL ||
        dd2_model_indices(NULL) != NULL) {
        return EXIT_FAILURE;
    }
    dd2_model_destroy(NULL);
    uint8_t bytes[DD2_MODEL_TEST_BYTES] = {0};
    dd2_model_test_source(bytes);
    if (!dd2_model_test_valid(bytes)) {
        return EXIT_FAILURE;
    }
    uint8_t unaligned[DD2_MODEL_TEST_BYTES + 1] = {0};
    dd2_model_test_source(unaligned + 1);
    if (!dd2_model_test_valid(unaligned + 1)) {
        return EXIT_FAILURE;
    }
    dd2_model_test_source(bytes);
    for (size_t length = 0; length < DD2_MODEL_TEST_BYTES; ++length) {
        if (dd2_model_create((dd2_byte_view){bytes, length}) != NULL) {
            return EXIT_FAILURE;
        }
    }
    const dd2_model_test_corruption cases[] = {
        {(size_t)DD2_MODEL_TEST_WORD * 2, UINT32_MAX},
        {(size_t)DD2_MODEL_TEST_WORD * 3, 0},
        {(size_t)DD2_MODEL_TEST_WORD * 4, 0},
        {DD2_MODEL_TEST_VERTEX_COUNT, UINT32_MAX},
        {DD2_MODEL_TEST_INDEX_COUNT, 4},
        {DD2_MODEL_TEST_PART_MATERIAL, 1},
        {DD2_MODEL_TEST_PART_FIRST, 1},
        {DD2_MODEL_TEST_PART_COUNT, 0},
        {DD2_MODEL_TEST_ROLE, UINT32_MAX},
        {DD2_MODEL_TEST_PIVOT, dd2_model_test_infinity},
        {DD2_MODEL_TEST_COLOR, dd2_model_test_nan},
        {DD2_MODEL_TEST_SCALARS, dd2_model_test_infinity},
        {DD2_MODEL_TEST_TEXTURE_INDEX, 1},
        {DD2_MODEL_TEST_VERTICES, dd2_model_test_nan},
        {DD2_MODEL_TEST_VERTICES + DD2_MODEL_TEST_NORMAL, dd2_model_test_infinity},
        {DD2_MODEL_TEST_VERTICES + DD2_MODEL_TEST_NORMAL, 0},
        {DD2_MODEL_TEST_INDICES, 3},
        {DD2_MODEL_TEST_INDICES + DD2_MODEL_TEST_WORD, 0}};
    for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        dd2_model_test_source(bytes);
        dd2_test_write_le32(bytes + cases[index].offset, cases[index].value);
        dd2_model *model = dd2_model_create((dd2_byte_view){bytes, sizeof(bytes)});
        if (model != NULL) {
            dd2_model_destroy(model);
            return EXIT_FAILURE;
        }
    }
    dd2_model_test_source(bytes);
    bytes[DD2_MODEL_TEST_TEXTURE] = '/';
    if (dd2_model_create((dd2_byte_view){bytes, sizeof(bytes)}) != NULL) {
        return EXIT_FAILURE;
    }
    dd2_model_test_source(bytes);
    bytes[DD2_MODEL_TEST_MATERIAL + 1] = 0;
    if (dd2_model_create((dd2_byte_view){bytes, sizeof(bytes)}) != NULL) {
        return EXIT_FAILURE;
    }
    puts("{\"scope\":\"owned authored model bounds, floats and lifetime\",\"pass\":true}");
    return EXIT_SUCCESS;
}
