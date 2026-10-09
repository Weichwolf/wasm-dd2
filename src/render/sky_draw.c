#include "render/sky_draw.h"

#include "assets/bounds.h"
#include "assets/model.h"
#include "assets/track.h"
#include "render/frustum.h"
#include "render/model_draw.h"

#include <GL/softgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

enum { DD2_SKY_AXES = 3, DD2_SKY_TRANSLATION = 12 };

struct dd2_sky_draw {
    const dd2_model *sources[DD2_TRACK_SKY_PATCHES];
    dd2_model_draw *patches[DD2_TRACK_SKY_PATCHES];
    dd2_bounds bounds[DD2_TRACK_SKY_PATCHES];
    dd2_model_texture_cache *cache;
    dd2_sky_draw_stats stats;
};

void dd2_sky_draw_destroy(dd2_sky_draw *draw) {
    if (draw != NULL) {
        for (size_t patch = 0; patch < DD2_TRACK_SKY_PATCHES; ++patch) {
            dd2_model_draw_destroy(draw->patches[patch]);
        }
        free(draw);
    }
}

static bool dd2_sky_bounds(const dd2_model *model, dd2_bounds *bounds) {
    const dd2_model_vertex *vertices = dd2_model_vertices(model);
    if (vertices == NULL) {
        return false;
    }
    for (size_t vertex = 0; vertex < dd2_model_vertex_count(model); ++vertex) {
        for (size_t axis = 0; axis < DD2_SKY_AXES; ++axis) {
            const float value = vertices[vertex].position[axis];
            if (vertex == 0 || value < bounds->minimum[axis]) {
                bounds->minimum[axis] = value;
            }
            if (vertex == 0 || value > bounds->maximum[axis]) {
                bounds->maximum[axis] = value;
            }
        }
    }
    return true;
}

dd2_sky_draw *dd2_sky_draw_create(const dd2_track *track, dd2_model_texture_cache *cache) {
    if (track == NULL || cache == NULL) {
        return NULL;
    }
    dd2_sky_draw *draw = calloc(1, sizeof(*draw));
    if (draw == NULL) {
        return NULL;
    }
    draw->cache = cache;
    for (unsigned patch = 0; patch < DD2_TRACK_SKY_PATCHES; ++patch) {
        draw->sources[patch] = dd2_track_prepared_sky(track, patch);
        if (!dd2_sky_bounds(draw->sources[patch], &draw->bounds[patch])) {
            dd2_sky_draw_destroy(draw);
            return NULL;
        }
    }
    return draw;
}

static bool dd2_sky_patch(dd2_sky_draw *draw, unsigned patch) {
    if (draw->patches[patch] == NULL) {
        draw->patches[patch] = dd2_model_draw_create_shared(draw->sources[patch], draw->cache);
    }
    if (draw->patches[patch] == NULL ||
        !dd2_model_draw_frame(draw->patches[patch],
                              (dd2_model_draw_options){.double_sided = true,
                                                       .cutout_textures = true,
                                                       .background = true})) {
        return false;
    }
    const dd2_model_draw_stats stats = dd2_model_draw_statistics(draw->patches[patch]);
    draw->stats.triangles += stats.triangles;
    draw->stats.batches += stats.batches;
    ++draw->stats.visible;
    return true;
}

bool dd2_sky_draw_frame(dd2_sky_draw *draw) {
    if (draw == NULL) {
        return false;
    }
    float projection[DD2_FRUSTUM_MATRIX] = {0};
    float rotation[DD2_FRUSTUM_MATRIX] = {0};
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glGetFloatv(GL_MODELVIEW_MATRIX, rotation);
    for (size_t axis = 0; axis < DD2_SKY_AXES; ++axis) {
        rotation[DD2_SKY_TRANSLATION + axis] = 0;
    }
    const dd2_frustum frustum = dd2_frustum_create(projection, rotation);
    draw->stats = (dd2_sky_draw_stats){0};
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(rotation);
    bool passed = true;
    for (unsigned patch = 0; patch < DD2_TRACK_SKY_PATCHES && passed; ++patch) {
        ++draw->stats.tested;
        if (!dd2_frustum_visible(&frustum, &draw->bounds[patch])) {
            ++draw->stats.culled;
            continue;
        }
        passed = dd2_sky_patch(draw, patch);
    }
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    return passed;
}

dd2_sky_draw_stats dd2_sky_draw_statistics(const dd2_sky_draw *draw) {
    return draw == NULL ? (dd2_sky_draw_stats){0} : draw->stats;
}
