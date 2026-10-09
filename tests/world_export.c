#include "assets/bytes.h"
#include "assets/world.h"
#include "content_file_fixture.h"
#include "platform/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static bool dd2_world_export_word(FILE *file, uint32_t value) {
    uint8_t bytes[sizeof(value)] = {0};
    for (size_t byte = 0; byte < sizeof(value); ++byte) {
        bytes[byte] = (uint8_t)(value >> (byte * DD2_BYTE_BITS));
    }
    return fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes);
}

static bool dd2_world_export_floats(FILE *file, const float *values, size_t count) {
    for (size_t index = 0; index < count; ++index) {
        uint32_t bits = 0;
        const unsigned char *source = (const unsigned char *)&values[index];
        unsigned char *destination = (unsigned char *)&bits;
        for (size_t byte = 0; byte < sizeof(bits); ++byte) {
            destination[byte] = source[byte];
        }
        if (!dd2_world_export_word(file, bits)) {
            return false;
        }
    }
    return true;
}

static bool dd2_world_export_resources(FILE *file, const dd2_world *world) {
    const dd2_world_resource *resources = dd2_world_resources(world);
    for (size_t index = 0; index < dd2_world_resource_count(world); ++index) {
        const dd2_world_resource *resource = &resources[index];
        const size_t axes = 3;
        if (fwrite(resource->path, 1, sizeof(resource->path), file) != sizeof(resource->path) ||
            !dd2_world_export_floats(file, resource->bounds.minimum, axes) ||
            !dd2_world_export_floats(file, resource->bounds.maximum, axes)) {
            return false;
        }
    }
    return true;
}

static bool dd2_world_export_instances(FILE *file, const dd2_world *world) {
    const dd2_world_instance *instances = dd2_world_instances(world);
    for (size_t index = 0; index < dd2_world_instance_count(world); ++index) {
        const dd2_world_instance *instance = &instances[index];
        const size_t axes = 3;
        if (fwrite(instance->name, 1, sizeof(instance->name), file) != sizeof(instance->name) ||
            !dd2_world_export_word(file, instance->resource) ||
            !dd2_world_export_floats(file, instance->position, axes)) {
            return false;
        }
    }
    return true;
}

static bool dd2_world_export_templates(FILE *file, const dd2_world *world) {
    const dd2_world_template *templates = dd2_world_templates(world);
    for (size_t index = 0; index < dd2_world_template_count(world); ++index) {
        const dd2_world_template *item = &templates[index];
        if (fwrite(item->name, 1, sizeof(item->name), file) != sizeof(item->name) ||
            !dd2_world_export_word(file, item->resource)) {
            return false;
        }
    }
    return true;
}

static bool dd2_world_export(const dd2_world *world, const char *path) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    const char magic[] = "DD2SCN1";
    const bool passed = fwrite(magic, 1, sizeof(magic), file) == sizeof(magic) &&
                        dd2_world_export_word(file, (uint32_t)dd2_world_resource_count(world)) &&
                        dd2_world_export_word(file, (uint32_t)dd2_world_instance_count(world)) &&
                        dd2_world_export_word(file, (uint32_t)dd2_world_template_count(world)) &&
                        dd2_world_export_resources(file, world) &&
                        dd2_world_export_instances(file, world) &&
                        dd2_world_export_templates(file, world);
    const bool closed = fclose(file) == 0;
    if (!passed || !closed) {
        (void)remove(path);
    }
    return passed && closed;
}

int main(int argc, char **argv) {
    const int arguments = 4;
    if (argc != arguments) {
        return EXIT_FAILURE;
    }
    dd2_file file = {0};
    if (!dd2_file_read(argv[2], &file)) {
        return EXIT_FAILURE;
    }
    dd2_world *world =
        dd2_world_create((dd2_byte_view){file.data, file.size}, dd2_content_test_model, argv[1]);
    dd2_file_release(&file);
    const bool passed = world != NULL && dd2_world_export(world, argv[3]);
    dd2_world_destroy(world);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
