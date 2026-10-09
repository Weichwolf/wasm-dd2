#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/world.h"
#include "model_fixture.h"
#include "world_fixture.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static bool dd2_world_test_ownership(void) {
    uint8_t storage[DD2_WORLD_TEST_BYTES + 1] = {0};
    uint8_t *bytes = storage + 1;
    dd2_world_test_source(bytes);
    dd2_world_test_loader_state state = {0};
    dd2_world *world = dd2_world_create((dd2_byte_view){bytes, DD2_WORLD_TEST_BYTES},
                                        dd2_world_test_loader, &state);
    if (world == NULL) {
        return false;
    }
    for (size_t byte = 0; byte < DD2_WORLD_TEST_BYTES; ++byte) {
        bytes[byte] = 0;
    }
    const bool passed = state.calls == DD2_WORLD_TEST_RESOURCES &&
                        dd2_world_resource_count(world) == DD2_WORLD_TEST_RESOURCES &&
                        dd2_world_instance_count(world) == DD2_WORLD_TEST_RESOURCES &&
                        dd2_world_template_count(world) == 1 &&
                        strcmp(dd2_world_templates(world)[0].name, "car-close") == 0 &&
                        strcmp(dd2_world_resources(world)[1].path, "models/b.dd2mesh") == 0 &&
                        dd2_world_instances(world)[2].position[0] == 8;
    dd2_world_destroy(world);
    return passed;
}

static bool dd2_world_test_reject(size_t offset, uint32_t value) {
    uint8_t bytes[DD2_WORLD_TEST_BYTES] = {0};
    dd2_world_test_source(bytes);
    dd2_test_write_le32(bytes + offset, value);
    dd2_world_test_loader_state state = {0};
    dd2_world *world =
        dd2_world_create((dd2_byte_view){bytes, sizeof(bytes)}, dd2_world_test_loader, &state);
    const bool rejected = world == NULL;
    dd2_world_destroy(world);
    return rejected;
}

static bool dd2_world_test_invalid_names(void) {
    const char *names[] = {"../outside.dd2mesh", "/outside.dd2mesh", "a//b.dd2mesh",
                           "a/./b.dd2mesh",      "a\\b.dd2mesh",     "a.png"};
    for (size_t index = 0; index < sizeof(names) / sizeof(names[0]); ++index) {
        uint8_t bytes[DD2_WORLD_TEST_BYTES] = {0};
        dd2_world_test_source(bytes);
        for (size_t byte = 0; byte < DD2_WORLD_TEST_PATH; ++byte) {
            bytes[DD2_WORLD_TEST_HEADER + byte] = 0;
        }
        dd2_model_test_name(bytes + DD2_WORLD_TEST_HEADER, names[index]);
        dd2_world_test_loader_state state = {0};
        dd2_world *world =
            dd2_world_create((dd2_byte_view){bytes, sizeof(bytes)}, dd2_world_test_loader, &state);
        const bool rejected = world == NULL && state.calls == 0;
        dd2_world_destroy(world);
        if (!rejected) {
            return false;
        }
    }
    return true;
}

static bool dd2_world_test_lifetime_failures(void) {
    uint8_t bytes[DD2_WORLD_TEST_BYTES + 1] = {0};
    dd2_world_test_source(bytes);
    for (size_t fail = 1; fail <= DD2_WORLD_TEST_RESOURCES; ++fail) {
        dd2_world_test_loader_state state = {.fail_at = fail};
        dd2_world *world = dd2_world_create((dd2_byte_view){bytes, DD2_WORLD_TEST_BYTES},
                                            dd2_world_test_loader, &state);
        const bool rejected = world == NULL && state.calls == fail;
        dd2_world_destroy(world);
        if (!rejected) {
            return false;
        }
    }
    dd2_world_test_loader_state state = {0};
    for (size_t size = 0; size < DD2_WORLD_TEST_BYTES; ++size) {
        dd2_world *world =
            dd2_world_create((dd2_byte_view){bytes, size}, dd2_world_test_loader, &state);
        const bool rejected = world == NULL;
        dd2_world_destroy(world);
        if (!rejected) {
            return false;
        }
    }
    dd2_world *world =
        dd2_world_create((dd2_byte_view){bytes, sizeof(bytes)}, dd2_world_test_loader, &state);
    const bool rejected = world == NULL;
    dd2_world_destroy(world);
    return rejected;
}

int main(void) {
    const size_t position = DD2_WORLD_TEST_INSTANCE + DD2_WORLD_TEST_NAME + DD2_WORLD_TEST_WORD;
    const size_t bounds = DD2_WORLD_TEST_HEADER + DD2_WORLD_TEST_PATH;
    const bool passed =
        dd2_world_test_ownership() && dd2_world_test_invalid_names() &&
        dd2_world_test_lifetime_failures() && dd2_world_test_reject(0, 0) &&
        dd2_world_test_reject(DD2_WORLD_TEST_RESOURCE_COUNT, UINT32_MAX) &&
        dd2_world_test_reject(DD2_WORLD_TEST_INSTANCE_COUNT, UINT32_MAX) &&
        dd2_world_test_reject(DD2_WORLD_TEST_TEMPLATE_COUNT, UINT32_MAX) &&
        dd2_world_test_reject(bounds, dd2_model_test_nan) &&
        dd2_world_test_reject(bounds, dd2_world_test_eight) &&
        dd2_world_test_reject(position, dd2_model_test_infinity) &&
        dd2_world_test_reject(DD2_WORLD_TEST_INSTANCE + DD2_WORLD_TEST_NAME,
                              DD2_WORLD_TEST_RESOURCES) &&
        dd2_world_test_reject(DD2_WORLD_TEST_TEMPLATE + DD2_WORLD_TEST_NAME,
                              DD2_WORLD_TEST_RESOURCES) &&
        dd2_world_test_reject(bounds + ((size_t)3 * DD2_WORLD_TEST_WORD), dd2_world_test_eight);
    dd2_world_destroy(NULL);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
