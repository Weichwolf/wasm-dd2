#include "render/world_draw.h"

#include "assets/bounds.h"
#include "assets/model.h"
#include "assets/world.h"
#include "render/frustum.h"
#include "render/model_draw.h"

#include <GL/softgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

enum { DD2_WORLD_DRAW_AXES = 3 };

typedef struct {
    dd2_model_draw *handle;
} dd2_world_cached_model;

struct dd2_world_draw {
    const dd2_world *world;
    dd2_model_texture_cache *textures;
    dd2_world_cached_model *models;
};

void dd2_world_draw_destroy(dd2_world_draw *draw) {
    if (draw != NULL) {
        for (size_t index = 0;
             draw->models != NULL && index < dd2_world_resource_count(draw->world); ++index) {
            dd2_model_draw_destroy(draw->models[index].handle);
        }
        dd2_model_texture_cache_destroy(draw->textures);
        free(draw->models);
        free(draw);
    }
}

dd2_world_draw *dd2_world_draw_create(const dd2_world *world, dd2_model_image_loader loader,
                                      void *user) {
    if (world == NULL) {
        return NULL;
    }
    dd2_world_draw *draw = calloc(1, sizeof(*draw));
    if (draw == NULL) {
        return NULL;
    }
    draw->world = world;
    draw->textures = dd2_model_texture_cache_create(loader, user);
    const size_t count = dd2_world_resource_count(world);
    draw->models = count != 0 ? calloc(count, sizeof(*draw->models)) : NULL;
    if (draw->textures == NULL || (count != 0 && draw->models == NULL)) {
        dd2_world_draw_destroy(draw);
        return NULL;
    }
    return draw;
}

static bool dd2_world_draw_instance(dd2_world_draw *draw, const dd2_world_instance *instance,
                                    dd2_model_draw_options options, dd2_world_draw_stats *stats) {
    const dd2_world_resource *resource = &dd2_world_resources(draw->world)[instance->resource];
    dd2_model_draw **model = &draw->models[instance->resource].handle;
    if (*model == NULL) {
        *model = dd2_model_draw_create_shared(resource->model, draw->textures);
    }
    if (*model == NULL) {
        return false;
    }
    for (size_t axis = 0; axis < DD2_WORLD_DRAW_AXES; ++axis) {
        options.eye[axis] -= instance->position[axis];
    }
    glPushMatrix();
    glTranslatef(instance->position[0], instance->position[1], instance->position[2]);
    const bool passed = dd2_model_draw_frame(*model, options);
    glPopMatrix();
    if (passed) {
        ++stats->visible;
        stats->triangles += dd2_model_index_count(resource->model) / DD2_WORLD_DRAW_AXES;
        stats->batches += dd2_model_draw_batch_count(*model);
    }
    return passed;
}

bool dd2_world_draw_frame(dd2_world_draw *draw, dd2_model_draw_options options,
                          dd2_world_draw_stats *stats) {
    if (stats == NULL) {
        return false;
    }
    *stats = (dd2_world_draw_stats){0};
    if (draw == NULL) {
        return false;
    }
    float projection[DD2_FRUSTUM_MATRIX] = {0};
    float modelview[DD2_FRUSTUM_MATRIX] = {0};
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    const dd2_frustum frustum = dd2_frustum_create(projection, modelview);
    const dd2_world_instance *instances = dd2_world_instances(draw->world);
    glMatrixMode(GL_MODELVIEW);
    for (size_t index = 0; index < dd2_world_instance_count(draw->world); ++index) {
        const dd2_world_instance *instance = &instances[index];
        dd2_bounds bounds = dd2_world_resources(draw->world)[instance->resource].bounds;
        for (size_t axis = 0; axis < DD2_WORLD_DRAW_AXES; ++axis) {
            bounds.minimum[axis] += instance->position[axis];
            bounds.maximum[axis] += instance->position[axis];
        }
        ++stats->tested;
        if (!dd2_frustum_visible(&frustum, &bounds)) {
            ++stats->culled;
        } else if (!dd2_world_draw_instance(draw, instance, options, stats)) {
            stats->uploaded_textures = dd2_model_texture_cache_count(draw->textures);
            return false;
        }
    }
    stats->uploaded_textures = dd2_model_texture_cache_count(draw->textures);
    return glGetError() == GL_NO_ERROR;
}
