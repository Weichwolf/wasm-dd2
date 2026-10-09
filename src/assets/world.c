#include "assets/world.h"

#include "assets/bounds.h"
#include "assets/bytes.h"
#include "assets/model.h"

#include <float.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_WORLD_HEADER_BYTES = 20,
    DD2_WORLD_RESOURCE_BYTES = 88,
    DD2_WORLD_INSTANCE_BYTES = 48,
    DD2_WORLD_TEMPLATE_BYTES = 36,
    DD2_WORLD_MAX_RESOURCES = 4096,
    DD2_WORLD_MAX_INSTANCES = 65536,
    DD2_WORLD_MAX_TEMPLATES = 128,
    DD2_WORLD_AXES = 3,
    DD2_WORLD_WORD_BYTES = 4,
    DD2_WORLD_MANTISSA = 24
};
static const float dd2_world_position_limit = 1000000;
_Static_assert(sizeof(float) == sizeof(uint32_t) && FLT_RADIX == 2 &&
                   FLT_MANT_DIG == DD2_WORLD_MANTISSA,
               "Scene files require IEEE binary32 floats");

struct dd2_world {
    size_t resource_count;
    size_t instance_count;
    size_t template_count;
    dd2_world_resource *resources;
    dd2_world_instance *instances;
    dd2_world_template *templates;
};

static bool dd2_world_scalar(const uint8_t *source, float *value) {
    const uint32_t bits = dd2_read_le32(source);
    const uint32_t exponent = UINT32_C(0x7f800000);
    if ((bits & exponent) == exponent) {
        return false;
    }
    const unsigned char *encoded = (const unsigned char *)&bits;
    unsigned char *decoded = (unsigned char *)value;
    for (size_t byte = 0; byte < sizeof(*value); ++byte) {
        decoded[byte] = encoded[byte];
    }
    return *value >= -dd2_world_position_limit && *value <= dd2_world_position_limit;
}

static bool dd2_world_name(const uint8_t *source, char *target, size_t length) {
    bool terminated = false;
    for (size_t byte = 0; byte < length; ++byte) {
        const uint8_t value = source[byte];
        const uint8_t first = 32;
        const uint8_t last = 126;
        if ((terminated && value != 0) || (value != 0 && (value < first || value > last))) {
            return false;
        }
        terminated = terminated || value == 0;
        target[byte] = (char)value;
    }
    return terminated && target[0] != '\0';
}

static bool dd2_world_path(const char *path) {
    const char extension[] = ".dd2mesh";
    const size_t length = strlen(path);
    if (length <= sizeof(extension) - 1 ||
        memcmp(path + length - (sizeof(extension) - 1), extension, sizeof(extension) - 1) != 0) {
        return false;
    }
    size_t first = 0;
    for (size_t byte = 0; byte <= length; ++byte) {
        const char value = path[byte];
        if (value == '/' || value == '\0') {
            const size_t count = byte - first;
            if (count == 0 || (count == 1 && path[first] == '.') ||
                (count == 2 && path[first] == '.' && path[first + 1] == '.')) {
                return false;
            }
            first = byte + 1;
        } else if (!((value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
                     (value >= '0' && value <= '9') || value == '_' || value == '-' ||
                     value == '.')) {
            return false;
        }
    }
    return true;
}

static bool dd2_world_resources_decode(dd2_world *world, const uint8_t *bytes) {
    for (size_t index = 0; index < world->resource_count; ++index) {
        dd2_world_resource *resource = &world->resources[index];
        const uint8_t *source = bytes + (index * DD2_WORLD_RESOURCE_BYTES);
        if (!dd2_world_name(source, resource->path, sizeof(resource->path)) ||
            !dd2_world_path(resource->path)) {
            return false;
        }
        for (size_t previous = 0; previous < index; ++previous) {
            if (strcmp(resource->path, world->resources[previous].path) == 0) {
                return false;
            }
        }
        source += sizeof(resource->path);
        for (size_t axis = 0; axis < DD2_WORLD_AXES; ++axis) {
            if (!dd2_world_scalar(source + (axis * DD2_WORLD_WORD_BYTES),
                                  &resource->bounds.minimum[axis]) ||
                !dd2_world_scalar(source + ((axis + DD2_WORLD_AXES) * DD2_WORLD_WORD_BYTES),
                                  &resource->bounds.maximum[axis]) ||
                resource->bounds.minimum[axis] > resource->bounds.maximum[axis]) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_world_instances_decode(dd2_world *world, const uint8_t *bytes) {
    for (size_t index = 0; index < world->instance_count; ++index) {
        dd2_world_instance *instance = &world->instances[index];
        const uint8_t *source = bytes + (index * DD2_WORLD_INSTANCE_BYTES);
        if (!dd2_world_name(source, instance->name, sizeof(instance->name))) {
            return false;
        }
        for (size_t previous = 0; previous < index; ++previous) {
            if (strcmp(instance->name, world->instances[previous].name) == 0) {
                return false;
            }
        }
        source += sizeof(instance->name);
        instance->resource = dd2_read_le32(source);
        if (instance->resource >= world->resource_count) {
            return false;
        }
        for (size_t axis = 0; axis < DD2_WORLD_AXES; ++axis) {
            if (!dd2_world_scalar(source + ((axis + 1) * DD2_WORLD_WORD_BYTES),
                                  &instance->position[axis])) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_world_templates_decode(dd2_world *world, const uint8_t *bytes) {
    for (size_t index = 0; index < world->template_count; ++index) {
        dd2_world_template *item = &world->templates[index];
        const uint8_t *source = bytes + (index * DD2_WORLD_TEMPLATE_BYTES);
        if (!dd2_world_name(source, item->name, sizeof(item->name))) {
            return false;
        }
        item->resource = dd2_read_le32(source + sizeof(item->name));
        if (item->resource >= world->resource_count) {
            return false;
        }
        for (size_t previous = 0; previous < index; ++previous) {
            if (strcmp(item->name, world->templates[previous].name) == 0) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_world_model_bounds(const dd2_world_resource *resource) {
    const dd2_model_vertex *vertices = dd2_model_vertices(resource->model);
    if (vertices == NULL) {
        return false;
    }
    dd2_bounds bounds = {0};
    for (size_t axis = 0; axis < DD2_WORLD_AXES; ++axis) {
        bounds.minimum[axis] = vertices[0].position[axis];
        bounds.maximum[axis] = vertices[0].position[axis];
    }
    for (size_t index = 1; index < dd2_model_vertex_count(resource->model); ++index) {
        for (size_t axis = 0; axis < DD2_WORLD_AXES; ++axis) {
            const float value = vertices[index].position[axis];
            bounds.minimum[axis] = value < bounds.minimum[axis] ? value : bounds.minimum[axis];
            bounds.maximum[axis] = value > bounds.maximum[axis] ? value : bounds.maximum[axis];
        }
    }
    for (size_t axis = 0; axis < DD2_WORLD_AXES; ++axis) {
        if (bounds.minimum[axis] != resource->bounds.minimum[axis] ||
            bounds.maximum[axis] != resource->bounds.maximum[axis]) {
            return false;
        }
    }
    return true;
}

void dd2_world_destroy(dd2_world *world) {
    if (world != NULL) {
        for (size_t index = 0; world->resources != NULL && index < world->resource_count; ++index) {
            dd2_model_destroy(world->resources[index].model);
        }
        free(world->resources);
        free(world->instances);
        free(world->templates);
        free(world);
    }
}

static bool dd2_world_decode(dd2_world *world, dd2_byte_view bytes) {
    const char magic[] = "DD2SCN1";
    if (bytes.data == NULL || bytes.size < DD2_WORLD_HEADER_BYTES ||
        memcmp(bytes.data, magic, sizeof(magic)) != 0) {
        return false;
    }
    world->resource_count = dd2_read_le32(bytes.data + sizeof(magic));
    world->instance_count = dd2_read_le32(bytes.data + sizeof(magic) + DD2_WORLD_WORD_BYTES);
    world->template_count =
        dd2_read_le32(bytes.data + sizeof(magic) + ((size_t)2 * DD2_WORLD_WORD_BYTES));
    if (world->resource_count > DD2_WORLD_MAX_RESOURCES ||
        world->instance_count > DD2_WORLD_MAX_INSTANCES ||
        world->template_count > DD2_WORLD_MAX_TEMPLATES) {
        return false;
    }
    const size_t expected = DD2_WORLD_HEADER_BYTES +
                            (world->resource_count * DD2_WORLD_RESOURCE_BYTES) +
                            (world->instance_count * DD2_WORLD_INSTANCE_BYTES) +
                            (world->template_count * DD2_WORLD_TEMPLATE_BYTES);
    if (bytes.size != expected) {
        return false;
    }
    world->resources = world->resource_count != 0
                           ? calloc(world->resource_count, sizeof(*world->resources))
                           : NULL;
    world->instances = world->instance_count != 0
                           ? calloc(world->instance_count, sizeof(*world->instances))
                           : NULL;
    world->templates = world->template_count != 0
                           ? calloc(world->template_count, sizeof(*world->templates))
                           : NULL;
    if ((world->resource_count != 0 && world->resources == NULL) ||
        (world->instance_count != 0 && world->instances == NULL) ||
        (world->template_count != 0 && world->templates == NULL)) {
        return false;
    }
    const uint8_t *source = bytes.data + DD2_WORLD_HEADER_BYTES;
    return dd2_world_resources_decode(world, source) &&
           dd2_world_instances_decode(
               world, source + (world->resource_count * DD2_WORLD_RESOURCE_BYTES)) &&
           dd2_world_templates_decode(world,
                                      source + (world->resource_count * DD2_WORLD_RESOURCE_BYTES) +
                                          (world->instance_count * DD2_WORLD_INSTANCE_BYTES));
}

dd2_world *dd2_world_create(dd2_byte_view bytes, dd2_world_model_loader loader, void *user) {
    dd2_world *world = calloc(1, sizeof(*world));
    if (world == NULL) {
        return NULL;
    }
    if (!dd2_world_decode(world, bytes) || (world->resource_count != 0 && loader == NULL)) {
        dd2_world_destroy(world);
        return NULL;
    }
    for (size_t index = 0; index < world->resource_count; ++index) {
        dd2_world_resource *resource = &world->resources[index];
        resource->model = loader(user, resource->path);
        if (!dd2_world_model_bounds(resource)) {
            dd2_world_destroy(world);
            return NULL;
        }
    }
    return world;
}

size_t dd2_world_resource_count(const dd2_world *world) {
    return world != NULL ? world->resource_count : 0;
}
size_t dd2_world_instance_count(const dd2_world *world) {
    return world != NULL ? world->instance_count : 0;
}
size_t dd2_world_template_count(const dd2_world *world) {
    return world != NULL ? world->template_count : 0;
}
const dd2_world_resource *dd2_world_resources(const dd2_world *world) {
    return world != NULL ? world->resources : NULL;
}
const dd2_world_instance *dd2_world_instances(const dd2_world *world) {
    return world != NULL ? world->instances : NULL;
}
const dd2_world_template *dd2_world_templates(const dd2_world *world) {
    return world != NULL ? world->templates : NULL;
}
