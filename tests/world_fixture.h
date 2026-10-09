#ifndef DD2_TEST_WORLD_FIXTURE_H
#define DD2_TEST_WORLD_FIXTURE_H

#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/model.h"
#include "model_fixture.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum {
    DD2_WORLD_TEST_BYTES = 464,
    DD2_WORLD_TEST_HEADER = 20,
    DD2_WORLD_TEST_RESOURCE_BYTES = 88,
    DD2_WORLD_TEST_RESOURCES = 3,
    DD2_WORLD_TEST_INSTANCE = 284,
    DD2_WORLD_TEST_INSTANCE_BYTES = 48,
    DD2_WORLD_TEST_TEMPLATE = 428,
    DD2_WORLD_TEST_WORD = 4,
    DD2_WORLD_TEST_PATH = 64,
    DD2_WORLD_TEST_NAME = 32,
    DD2_WORLD_TEST_RESOURCE_COUNT = 8,
    DD2_WORLD_TEST_INSTANCE_COUNT = 12,
    DD2_WORLD_TEST_TEMPLATE_COUNT = 16
};
static const uint32_t dd2_world_test_eight = UINT32_C(0x41000000);

typedef struct {
    size_t calls;
    size_t fail_at;
    bool hidden_texture;
} dd2_world_test_loader_state;

static void dd2_world_test_source(uint8_t *bytes) {
    for (size_t byte = 0; byte < DD2_WORLD_TEST_BYTES; ++byte) {
        bytes[byte] = 0;
    }
    dd2_model_test_name(bytes, "DD2SCN1");
    dd2_test_write_le32(bytes + DD2_WORLD_TEST_RESOURCE_COUNT, DD2_WORLD_TEST_RESOURCES);
    dd2_test_write_le32(bytes + DD2_WORLD_TEST_INSTANCE_COUNT, DD2_WORLD_TEST_RESOURCES);
    dd2_test_write_le32(bytes + DD2_WORLD_TEST_TEMPLATE_COUNT, 1);
    const char *paths[] = {"models/a.dd2mesh", "models/b.dd2mesh", "models/c.dd2mesh"};
    const char *names[] = {"first", "second", "third"};
    for (size_t index = 0; index < DD2_WORLD_TEST_RESOURCES; ++index) {
        uint8_t *resource = bytes + DD2_WORLD_TEST_HEADER + (index * DD2_WORLD_TEST_RESOURCE_BYTES);
        dd2_model_test_name(resource, paths[index]);
        dd2_test_write_le32(resource + DD2_WORLD_TEST_PATH + ((size_t)3 * DD2_WORLD_TEST_WORD),
                            dd2_model_test_one);
        dd2_test_write_le32(resource + DD2_WORLD_TEST_PATH + ((size_t)4 * DD2_WORLD_TEST_WORD),
                            dd2_model_test_one);
        uint8_t *instance =
            bytes + DD2_WORLD_TEST_INSTANCE + (index * DD2_WORLD_TEST_INSTANCE_BYTES);
        dd2_model_test_name(instance, names[index]);
        dd2_test_write_le32(instance + DD2_WORLD_TEST_NAME, (uint32_t)index);
    }
    dd2_test_write_le32(bytes + DD2_WORLD_TEST_INSTANCE +
                            ((size_t)2 * DD2_WORLD_TEST_INSTANCE_BYTES) + DD2_WORLD_TEST_NAME +
                            DD2_WORLD_TEST_WORD,
                        dd2_world_test_eight);
    dd2_model_test_name(bytes + DD2_WORLD_TEST_TEMPLATE, "car-close");
}

static dd2_model *dd2_world_test_loader(void *user, const char *path) {
    dd2_world_test_loader_state *state = user;
    ++state->calls;
    if (state->calls == state->fail_at) {
        return NULL;
    }
    uint8_t bytes[DD2_MODEL_TEST_BYTES] = {0};
    dd2_model_test_source(bytes);
    if (state->hidden_texture && strcmp(path, "models/c.dd2mesh") == 0) {
        for (size_t byte = 0; byte < DD2_MODEL_NAME_BYTES; ++byte) {
            bytes[DD2_MODEL_TEST_TEXTURE + byte] = 0;
        }
        dd2_model_test_name(bytes + DD2_MODEL_TEST_TEXTURE, "textures/missing.png");
    }
    return dd2_model_create((dd2_byte_view){bytes, sizeof(bytes)});
}

#endif
