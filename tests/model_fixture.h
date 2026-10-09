#ifndef DD2_TEST_MODEL_FIXTURE_H
#define DD2_TEST_MODEL_FIXTURE_H

#include "asset_fixture.h"
#include "assets/model.h"

#include <stddef.h>
#include <stdint.h>

enum {
    DD2_MODEL_TEST_BYTES = 480,
    DD2_MODEL_TEST_TEXTURE = 28,
    DD2_MODEL_TEST_MATERIAL = 220,
    DD2_MODEL_TEST_PART = 280,
    DD2_MODEL_TEST_VERTICES = 372,
    DD2_MODEL_TEST_INDICES = 468,
    DD2_MODEL_TEST_VERTEX_BYTES = 32,
    DD2_MODEL_TEST_WORD = 4,
    DD2_MODEL_TEST_NORMAL = 20,
    DD2_MODEL_TEST_TEXTURE_INDEX = 276,
    DD2_MODEL_TEST_PART_MATERIAL = 344,
    DD2_MODEL_TEST_PART_FIRST = 348,
    DD2_MODEL_TEST_PART_COUNT = 352,
    DD2_MODEL_TEST_ROLE = 356,
    DD2_MODEL_TEST_PIVOT = 360,
    DD2_MODEL_TEST_COLOR = 252,
    DD2_MODEL_TEST_SCALARS = 268,
    DD2_MODEL_TEST_VERTEX_COUNT = 20,
    DD2_MODEL_TEST_INDEX_COUNT = 24,
    DD2_MODEL_TEST_HEADER_COUNTS_END = 5
};
static const uint32_t dd2_model_test_one = UINT32_C(0x3f800000);
static const uint32_t dd2_model_test_half = UINT32_C(0x3f000000);
static const uint32_t dd2_model_test_nan = UINT32_C(0x7fc12345);
static const uint32_t dd2_model_test_infinity = UINT32_C(0xff800000);

static void dd2_model_test_clear(uint8_t *bytes) {
    for (size_t index = 0; index < DD2_MODEL_TEST_BYTES; ++index) {
        bytes[index] = 0;
    }
}

static void dd2_model_test_name(uint8_t *bytes, const char *name) {
    for (size_t index = 0; name[index] != '\0'; ++index) {
        bytes[index] = (uint8_t)name[index];
    }
}

static void dd2_model_test_source(uint8_t *bytes) {
    dd2_model_test_clear(bytes);
    dd2_model_test_name(bytes, "DD2MESH2");
    for (size_t index = 2; index < DD2_MODEL_TEST_HEADER_COUNTS_END; ++index) {
        dd2_test_write_le32(bytes + (index * DD2_MODEL_TEST_WORD), 1);
    }
    dd2_test_write_le32(bytes + (DD2_MODEL_TEST_VERTEX_COUNT), 3);
    dd2_test_write_le32(bytes + (DD2_MODEL_TEST_INDEX_COUNT), 3);
    const char *paths[DD2_MODEL_TEXTURE_MAPS] = {"textures/paint-flake-albedo.png",
                                                 "textures/paint-flake-roughness.png",
                                                 "textures/paint-flake-normal.png"};
    for (size_t index = 0; index < DD2_MODEL_TEXTURE_MAPS; ++index) {
        dd2_model_test_name(bytes + DD2_MODEL_TEST_TEXTURE + (index * DD2_MODEL_NAME_BYTES),
                            paths[index]);
    }
    dd2_model_test_name(bytes + DD2_MODEL_TEST_MATERIAL, "Paint");
    for (size_t channel = 0; channel < 4; ++channel) {
        dd2_test_write_le32(bytes + DD2_MODEL_TEST_COLOR + (channel * DD2_MODEL_TEST_WORD),
                            dd2_model_test_one);
    }
    dd2_test_write_le32(bytes + DD2_MODEL_TEST_SCALARS, dd2_model_test_half);
    dd2_test_write_le32(bytes + DD2_MODEL_TEST_SCALARS + DD2_MODEL_TEST_WORD, dd2_model_test_half);
    dd2_model_test_name(bytes + DD2_MODEL_TEST_PART, "Body.Panel");
    dd2_test_write_le32(bytes + DD2_MODEL_TEST_PART_COUNT, 3);
    dd2_test_write_le32(bytes + DD2_MODEL_TEST_VERTICES + DD2_MODEL_TEST_VERTEX_BYTES,
                        dd2_model_test_one);
    dd2_test_write_le32(bytes + DD2_MODEL_TEST_VERTICES +
                            ((size_t)DD2_MODEL_TEST_VERTEX_BYTES * 2) + DD2_MODEL_TEST_WORD,
                        dd2_model_test_one);
    for (size_t index = 0; index < 3; ++index) {
        dd2_test_write_le32(bytes + DD2_MODEL_TEST_VERTICES +
                                (index * DD2_MODEL_TEST_VERTEX_BYTES) + DD2_MODEL_TEST_NORMAL,
                            dd2_model_test_one);
        dd2_test_write_le32(bytes + DD2_MODEL_TEST_INDICES + (index * DD2_MODEL_TEST_WORD),
                            (uint32_t)index);
    }
}

#endif
