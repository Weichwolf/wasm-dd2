#include "assets/scene.h"

#include "assets/bytes.h"
#include "assets/lz.h"
#include "assets/mesh.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    DD2_SCENE_WORD_BYTES = 4,
    DD2_SCENE_INSTANCE_BYTES = 16,
    DD2_SCENE_MAX_BLOCK_BYTES = 16384,
    DD2_SCENE_MAX_BLOCKS = 4096
};

struct dd2_scene {
    size_t block_count;
    size_t object_count;
    dd2_scene_object *objects;
};

static bool dd2_scene_block(dd2_scene *scene, dd2_byte_view bytes, dd2_mesh_limits limits) {
    if (bytes.size < DD2_SCENE_WORD_BYTES) {
        return false;
    }
    const size_t count = dd2_read_le32(bytes.data);
    if (count > (bytes.size - DD2_SCENE_WORD_BYTES) / DD2_SCENE_INSTANCE_BYTES ||
        count > (SIZE_MAX / sizeof(*scene->objects)) - scene->object_count) {
        return false;
    }
    if (count == 0) {
        return true;
    }
    dd2_scene_object *objects =
        realloc(scene->objects, (scene->object_count + count) * sizeof(*objects));
    if (objects == NULL) {
        return false;
    }
    scene->objects = objects;
    const size_t table_end = DD2_SCENE_WORD_BYTES + (count * DD2_SCENE_INSTANCE_BYTES);
    for (size_t index = 0; index < count; ++index) {
        const uint8_t *row = bytes.data + DD2_SCENE_WORD_BYTES + (index * DD2_SCENE_INSTANCE_BYTES);
        const size_t start = dd2_read_le32(row);
        if (start < table_end || start >= bytes.size) {
            return false;
        }
        size_t end = bytes.size;
        for (size_t other = 0; other < count; ++other) {
            const size_t next = dd2_read_le32(bytes.data + DD2_SCENE_WORD_BYTES +
                                              (other * DD2_SCENE_INSTANCE_BYTES));
            if (next > start && next < end) {
                end = next;
            }
        }
        dd2_mesh *mesh = dd2_mesh_create(
            (dd2_byte_view){.data = bytes.data + start, .size = end - start}, limits);
        if (mesh == NULL) {
            return false;
        }
        scene->objects[scene->object_count++] = (dd2_scene_object){
            .mesh = mesh,
            .position = {.x = dd2_read_le_i32(row + DD2_SCENE_WORD_BYTES),
                         .y = dd2_read_le_i32(row + ((size_t)2 * DD2_SCENE_WORD_BYTES)),
                         .z = dd2_read_le_i32(row + ((size_t)3 * DD2_SCENE_WORD_BYTES))}};
    }
    return true;
}

static bool dd2_scene_load(dd2_scene *scene, dd2_byte_view section, dd2_scene_options options) {
    const size_t table_bytes = dd2_read_le32(section.data);
    if (table_bytes == 0 || table_bytes % DD2_SCENE_WORD_BYTES != 0 || table_bytes > section.size ||
        table_bytes / DD2_SCENE_WORD_BYTES > DD2_SCENE_MAX_BLOCKS) {
        return false;
    }
    scene->block_count = table_bytes / DD2_SCENE_WORD_BYTES;
    uint8_t *buffer = malloc(DD2_SCENE_MAX_BLOCK_BYTES);
    if (buffer == NULL) {
        return false;
    }
    bool passed = true;
    for (size_t block = 0; block < scene->block_count && passed; ++block) {
        const size_t start = dd2_read_le32(section.data + (block * DD2_SCENE_WORD_BYTES));
        const size_t end = block + 1 < scene->block_count
                               ? dd2_read_le32(section.data + ((block + 1) * DD2_SCENE_WORD_BYTES))
                               : section.size;
        if (start < table_bytes || start >= end || end > section.size) {
            passed = false;
            break;
        }
        dd2_byte_view bytes = {.data = section.data + start, .size = end - start};
        if (options.compressed) {
            size_t written = 0;
            passed = dd2_lz_decode(
                bytes, (dd2_byte_buffer){.data = buffer, .size = DD2_SCENE_MAX_BLOCK_BYTES},
                &written);
            bytes = (dd2_byte_view){.data = buffer, .size = written};
        }
        passed = passed && dd2_scene_block(scene, bytes, options.limits);
    }
    free(buffer);
    return passed;
}

dd2_scene *dd2_scene_create(dd2_byte_view section, dd2_scene_options options) {
    if (section.size != 0 && (section.data == NULL || section.size < DD2_SCENE_WORD_BYTES)) {
        return NULL;
    }
    dd2_scene *scene = calloc(1, sizeof(*scene));
    if (scene == NULL) {
        return NULL;
    }
    if (section.size != 0 && !dd2_scene_load(scene, section, options)) {
        dd2_scene_destroy(scene);
        return NULL;
    }
    return scene;
}

void dd2_scene_destroy(dd2_scene *scene) {
    if (scene != NULL) {
        for (size_t index = 0; index < scene->object_count; ++index) {
            dd2_mesh_destroy(scene->objects[index].mesh);
        }
        free(scene->objects);
        free(scene);
    }
}

size_t dd2_scene_block_count(const dd2_scene *scene) {
    return scene != NULL ? scene->block_count : 0;
}
size_t dd2_scene_object_count(const dd2_scene *scene) {
    return scene != NULL ? scene->object_count : 0;
}
const dd2_scene_object *dd2_scene_objects(const dd2_scene *scene) {
    return scene != NULL ? scene->objects : NULL;
}
