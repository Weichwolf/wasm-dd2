#include "render/model_draw.h"

#include "assets/bounds.h"
#include "assets/image.h"
#include "assets/model.h"
#include "render/color.h"
#include "render/frustum.h"

#include <GL/softgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const float dd2_model_draw_dielectric = 0.04F;
static const float dd2_model_draw_metal_diffuse = 0.65F;
static const float dd2_model_draw_max_shininess = 96;
static const float dd2_model_draw_min_shininess = 2;
enum { DD2_MODEL_DRAW_CHANNELS = 4, DD2_MODEL_DRAW_AXES = 3 };

typedef struct {
    uint32_t material;
    dd2_model_role role;
    float pivot[DD2_MODEL_DRAW_AXES];
    float center[DD2_MODEL_DRAW_AXES];
    dd2_bounds bounds;
    size_t first;
    size_t count;
} dd2_model_batch;

struct dd2_model_draw {
    const dd2_model *model;
    GLuint *textures;
    bool *texture_alpha;
    dd2_model_batch *batches;
    uint32_t *indices;
    size_t *order;
    size_t batch_count;
    dd2_model_texture_cache *cache;
    bool owns_cache;
    float projection[DD2_FRUSTUM_MATRIX];
    dd2_model_draw_stats stats;
};

typedef struct dd2_model_cached_texture {
    char path[DD2_MODEL_NAME_BYTES];
    GLuint texture;
    bool alpha;
    struct dd2_model_cached_texture *next;
} dd2_model_cached_texture;

struct dd2_model_texture_cache {
    dd2_model_image_loader loader;
    void *user;
    dd2_model_cached_texture *first;
    size_t count;
};

typedef struct {
    size_t width;
    size_t height;
    uint8_t *pixels;
} dd2_model_mip;

static dd2_model_mip dd2_model_mip_create(const dd2_image *image) {
    dd2_model_mip mip = {.width = dd2_image_width(image), .height = dd2_image_height(image)};
    mip.pixels = calloc(dd2_image_rgba_bytes(image), 1);
    if (mip.pixels != NULL) {
        const size_t stride = mip.width * DD2_MODEL_DRAW_CHANNELS;
        const uint8_t *source = dd2_image_pixels(image);
        for (size_t row = 0; row < mip.height; ++row) {
            for (size_t byte = 0; byte < stride; ++byte) {
                const uint8_t value = source[((mip.height - row - 1) * stride) + byte];
                mip.pixels[(row * stride) + byte] =
                    byte % DD2_MODEL_DRAW_CHANNELS == DD2_MODEL_DRAW_CHANNELS - 1
                        ? value
                        : dd2_color_srgb8_to_linear8(value);
            }
        }
    }
    return mip;
}

static bool dd2_model_mip_reduce(dd2_model_mip *mip) {
    const size_t width = mip->width > 1 ? mip->width / 2 : 1;
    const size_t height = mip->height > 1 ? mip->height / 2 : 1;
    /* In-place box reduction reads only rows/columns at or after the destination.
     * Proportional boundaries retain every texel for odd and one-wide sizes. */
    for (size_t row = 0; row < height; ++row) {
        for (size_t column = 0; column < width; ++column) {
            const size_t top = row * mip->height / height;
            const size_t bottom = (row + 1) * mip->height / height;
            const size_t left = column * mip->width / width;
            const size_t right = (column + 1) * mip->width / width;
            const size_t count = (bottom - top) * (right - left);
            if (count == 0) {
                return false;
            }
            for (size_t channel = 0; channel < DD2_MODEL_DRAW_CHANNELS; ++channel) {
                unsigned sum = 0;
                for (size_t y_pos = top; y_pos < bottom; ++y_pos) {
                    for (size_t x_pos = left; x_pos < right; ++x_pos) {
                        sum +=
                            mip->pixels[(((y_pos * mip->width) + x_pos) * DD2_MODEL_DRAW_CHANNELS) +
                                        channel];
                    }
                }
                mip->pixels[(((row * width) + column) * DD2_MODEL_DRAW_CHANNELS) + channel] =
                    (uint8_t)((sum + (count / 2)) / count);
            }
        }
    }
    mip->width = width;
    mip->height = height;
    return true;
}

static bool dd2_model_texture_upload(GLuint *texture, const dd2_image *image) {
    dd2_model_mip mip = dd2_model_mip_create(image);
    if (mip.pixels == NULL || mip.width == 0 || mip.height == 0) {
        free(mip.pixels);
        return false;
    }
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    GLint level = 0;
    for (;;) {
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, (GLsizei)mip.width, (GLsizei)mip.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, mip.pixels);
        if (glGetError() != GL_NO_ERROR || *texture == 0) {
            free(mip.pixels);
            return false;
        }
        if (mip.width == 1 && mip.height == 1) {
            break;
        }
        if (!dd2_model_mip_reduce(&mip)) {
            free(mip.pixels);
            return false;
        }
        ++level;
    }
    free(mip.pixels);
    return true;
}

static bool dd2_model_draw_transparent(const dd2_model_draw *draw, uint32_t index, bool cutout) {
    const dd2_model_material *material = &dd2_model_materials(draw->model)[index];
    return material->color[3] < 1 ||
           (!cutout && material->texture != DD2_MODEL_NO_TEXTURE && draw->texture_alpha != NULL &&
            draw->texture_alpha[material->texture]);
}

static bool dd2_model_image_alpha(const dd2_image *image) {
    const uint8_t *pixels = dd2_image_pixels(image);
    for (size_t index = DD2_MODEL_DRAW_CHANNELS - 1; index < dd2_image_rgba_bytes(image);
         index += DD2_MODEL_DRAW_CHANNELS) {
        if (pixels[index] != UINT8_MAX) {
            return true;
        }
    }
    return false;
}

dd2_model_texture_cache *dd2_model_texture_cache_create(dd2_model_image_loader loader, void *user) {
    dd2_model_texture_cache *cache = calloc(1, sizeof(*cache));
    if (cache != NULL) {
        cache->loader = loader;
        cache->user = user;
    }
    return cache;
}

void dd2_model_texture_cache_destroy(dd2_model_texture_cache *cache) {
    if (cache != NULL) {
        dd2_model_cached_texture *entry = cache->first;
        while (entry != NULL) {
            dd2_model_cached_texture *next = entry->next;
            glDeleteTextures(1, &entry->texture);
            free(entry);
            entry = next;
        }
        free(cache);
    }
}

size_t dd2_model_texture_cache_count(const dd2_model_texture_cache *cache) {
    return cache != NULL ? cache->count : 0;
}

static const dd2_model_cached_texture *dd2_model_cache_get(dd2_model_texture_cache *cache,
                                                           const char *path) {
    for (dd2_model_cached_texture *entry = cache->first; entry != NULL; entry = entry->next) {
        if (strcmp(path, entry->path) == 0) {
            return entry;
        }
    }
    if (cache->loader == NULL) {
        return NULL;
    }
    dd2_model_cached_texture *entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }
    dd2_image *image = cache->loader(cache->user, path);
    const bool uploaded = image != NULL && dd2_model_texture_upload(&entry->texture, image);
    entry->alpha = image != NULL && dd2_model_image_alpha(image);
    dd2_image_destroy(image);
    if (!uploaded) {
        if (entry->texture != 0) {
            glDeleteTextures(1, &entry->texture);
        }
        free(entry);
        return NULL;
    }
    for (size_t byte = 0; byte < sizeof(entry->path); ++byte) {
        entry->path[byte] = path[byte];
    }
    entry->next = cache->first;
    cache->first = entry;
    ++cache->count;
    return entry;
}

static size_t dd2_model_batch_find(dd2_model_draw *draw, const dd2_model_part *part) {
    for (size_t index = 0; index < draw->batch_count; ++index) {
        const dd2_model_batch *batch = &draw->batches[index];
        const bool static_role =
            part->role == DD2_MODEL_EXTERIOR || part->role == DD2_MODEL_COCKPIT;
        if (!dd2_model_draw_transparent(draw, part->material, false) &&
            batch->material == part->material && batch->role == part->role &&
            (static_role ||
             (batch->pivot[0] == part->pivot[0] && batch->pivot[1] == part->pivot[1] &&
              batch->pivot[2] == part->pivot[2]))) {
            return index;
        }
    }
    const size_t index = draw->batch_count++;
    draw->batches[index] = (dd2_model_batch){.material = part->material, .role = part->role};
    for (size_t axis = 0; axis < DD2_MODEL_DRAW_AXES; ++axis) {
        draw->batches[index].pivot[axis] = part->pivot[axis];
    }
    return index;
}

static void dd2_model_batch_vertex(dd2_model_draw *draw, dd2_model_batch *batch, uint32_t index) {
    const dd2_model_vertex *vertex = &dd2_model_vertices(draw->model)[index];
    for (size_t axis = 0; axis < DD2_MODEL_DRAW_AXES; ++axis) {
        const float position = vertex->position[axis];
        batch->center[axis] += position;
        if (batch->count == 0 || position < batch->bounds.minimum[axis]) {
            batch->bounds.minimum[axis] = position;
        }
        if (batch->count == 0 || position > batch->bounds.maximum[axis]) {
            batch->bounds.maximum[axis] = position;
        }
    }
    draw->indices[batch->first + batch->count++] = index;
}

static bool dd2_model_batches_create(dd2_model_draw *draw) {
    const size_t parts = dd2_model_part_count(draw->model);
    size_t *groups = calloc(parts, sizeof(*groups));
    if (groups == NULL) {
        return false;
    }
    const dd2_model_part *source_parts = dd2_model_parts(draw->model);
    for (size_t index = 0; index < parts; ++index) {
        groups[index] = dd2_model_batch_find(draw, &source_parts[index]);
        draw->batches[groups[index]].count += source_parts[index].index_count;
    }
    size_t first = 0;
    for (size_t index = 0; index < draw->batch_count; ++index) {
        dd2_model_batch *batch = &draw->batches[index];
        batch->first = first;
        first += batch->count;
        batch->count = 0;
    }
    const uint32_t *source_indices = dd2_model_indices(draw->model);
    for (size_t index = 0; index < parts; ++index) {
        const dd2_model_part *part = &source_parts[index];
        dd2_model_batch *batch = &draw->batches[groups[index]];
        for (size_t item = 0; item < part->index_count; ++item) {
            const uint32_t vertex = source_indices[part->first_index + item];
            dd2_model_batch_vertex(draw, batch, vertex);
        }
    }
    for (size_t index = 0; index < draw->batch_count; ++index) {
        for (size_t axis = 0; axis < DD2_MODEL_DRAW_AXES; ++axis) {
            draw->batches[index].center[axis] /= (float)draw->batches[index].count;
        }
    }
    free(groups);
    return true;
}

void dd2_model_draw_destroy(dd2_model_draw *draw) {
    if (draw != NULL) {
        if (draw->owns_cache) {
            dd2_model_texture_cache_destroy(draw->cache);
        }
        free(draw->textures);
        free(draw->texture_alpha);
        free(draw->batches);
        free(draw->indices);
        free(draw->order);
        free(draw);
    }
}

dd2_model_draw *dd2_model_draw_create_shared(const dd2_model *model,
                                             dd2_model_texture_cache *cache) {
    if (model == NULL || cache == NULL) {
        return NULL;
    }
    dd2_model_draw *draw = calloc(1, sizeof(*draw));
    if (draw == NULL) {
        return NULL;
    }
    draw->model = model;
    draw->cache = cache;
    const size_t textures = dd2_model_texture_count(model);
    draw->textures = textures == 0 ? NULL : calloc(textures, sizeof(*draw->textures));
    draw->texture_alpha = textures == 0 ? NULL : calloc(textures, sizeof(*draw->texture_alpha));
    draw->batches = calloc(dd2_model_part_count(model), sizeof(*draw->batches));
    draw->order = calloc(dd2_model_part_count(model), sizeof(*draw->order));
    draw->indices = calloc(dd2_model_index_count(model), sizeof(*draw->indices));
    if ((textures != 0 && (draw->textures == NULL || draw->texture_alpha == NULL)) ||
        draw->batches == NULL || draw->order == NULL || draw->indices == NULL) {
        dd2_model_draw_destroy(draw);
        return NULL;
    }
    glActiveTexture(GL_TEXTURE0);
    const dd2_model_texture *sources = dd2_model_textures(model);
    for (size_t index = 0; index < textures; ++index) {
        const dd2_model_cached_texture *entry = dd2_model_cache_get(cache, sources[index].paths[0]);
        if (entry == NULL) {
            dd2_model_draw_destroy(draw);
            return NULL;
        }
        draw->textures[index] = entry->texture;
        draw->texture_alpha[index] = entry->alpha;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    if (!dd2_model_batches_create(draw)) {
        dd2_model_draw_destroy(draw);
        return NULL;
    }
    return draw;
}

dd2_model_draw *dd2_model_draw_create(const dd2_model *model, dd2_model_image_loader loader,
                                      void *user) {
    if (model == NULL || (dd2_model_texture_count(model) != 0 && loader == NULL)) {
        return NULL;
    }
    dd2_model_texture_cache *cache = dd2_model_texture_cache_create(loader, user);
    if (cache == NULL) {
        return NULL;
    }
    dd2_model_draw *draw = dd2_model_draw_create_shared(model, cache);
    if (draw == NULL) {
        dd2_model_texture_cache_destroy(cache);
        return NULL;
    }
    draw->owns_cache = true;
    return draw;
}

static void dd2_model_draw_lights(void) {
    const float ambient[] = {0.23F, 0.25F, 0.29F, 1};
    const float key[] = {0.78F, 0.74F, 0.68F, 1};
    const float fill[] = {0.19F, 0.25F, 0.34F, 1};
    const float key_position[] = {0.45F, 0.85F, 0.3F, 0};
    const float fill_position[] = {-0.7F, 0.3F, -0.6F, 0};
    const float black[] = {0, 0, 0, 1};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightfv(GL_LIGHT0, GL_POSITION, key_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, key);
    glLightfv(GL_LIGHT0, GL_SPECULAR, key);
    glLightfv(GL_LIGHT0, GL_AMBIENT, black);
    glLightfv(GL_LIGHT1, GL_POSITION, fill_position);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, fill);
    glLightfv(GL_LIGHT1, GL_SPECULAR, fill);
    glLightfv(GL_LIGHT1, GL_AMBIENT, black);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_NORMALIZE);
    glDisable(GL_COLOR_MATERIAL);
}

static void dd2_model_draw_material(const dd2_model_draw *draw, uint32_t index) {
    const dd2_model_material *material = &dd2_model_materials(draw->model)[index];
    float diffuse[DD2_MODEL_DRAW_CHANNELS] = {0};
    float specular[DD2_MODEL_DRAW_CHANNELS] = {0};
    for (size_t channel = 0; channel < DD2_MODEL_DRAW_AXES; ++channel) {
        diffuse[channel] =
            material->color[channel] * (1 - (dd2_model_draw_metal_diffuse * material->metallic));
        specular[channel] = (dd2_model_draw_dielectric * (1 - material->metallic)) +
                            (material->color[channel] * material->metallic);
    }
    diffuse[3] = material->color[3];
    specular[3] = 1;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    const float gloss = 1 - material->roughness;
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS,
                dd2_model_draw_min_shininess + (gloss * gloss * dd2_model_draw_max_shininess));
    glColor4fv(material->color);
    if (material->texture == DD2_MODEL_NO_TEXTURE) {
        glDisable(GL_TEXTURE_2D);
    } else {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, draw->textures[material->texture]);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
}

static void dd2_model_draw_batch(dd2_model_draw *draw, const dd2_model_batch *batch,
                                 dd2_model_draw_options options) {
    ++draw->stats.tested;
    if (options.cockpit_only && batch->role != DD2_MODEL_COCKPIT &&
        batch->role != DD2_MODEL_STEERING) {
        ++draw->stats.role_filtered;
        return;
    }
    glPushMatrix();
    glTranslatef(batch->pivot[0], batch->pivot[1], batch->pivot[2]);
    if (batch->role >= DD2_MODEL_WHEEL_FRONT_LEFT && batch->role <= DD2_MODEL_WHEEL_REAR_RIGHT) {
        if (batch->role <= DD2_MODEL_WHEEL_FRONT_RIGHT) {
            glRotatef(options.front_steer, 0, 1, 0);
        }
        glRotatef(options.wheel_roll, 1, 0, 0);
    } else if (batch->role == DD2_MODEL_STEERING) {
        glRotatef(options.steering, 0, 0, 1);
    }
    glTranslatef(-batch->pivot[0], -batch->pivot[1], -batch->pivot[2]);
    float modelview[DD2_FRUSTUM_MATRIX] = {0};
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    const dd2_frustum frustum = dd2_frustum_create(draw->projection, modelview);
    if (dd2_frustum_visible(&frustum, &batch->bounds)) {
        dd2_model_draw_material(draw, batch->material);
        glDrawElements(GL_TRIANGLES, (GLsizei)batch->count, GL_UNSIGNED_INT,
                       draw->indices + batch->first);
        ++draw->stats.batches;
        draw->stats.triangles += batch->count / DD2_MODEL_DRAW_AXES;
    } else {
        ++draw->stats.culled;
    }
    glPopMatrix();
}

static float dd2_model_batch_distance(const dd2_model_batch *batch, const float *eye) {
    float distance = 0;
    for (size_t axis = 0; axis < DD2_MODEL_DRAW_AXES; ++axis) {
        const float delta = batch->center[axis] - eye[axis];
        distance += delta * delta;
    }
    return distance;
}

static size_t dd2_model_transparent_order(dd2_model_draw *draw, const float *eye, bool cutout) {
    size_t count = 0;
    for (size_t index = 0; index < draw->batch_count; ++index) {
        if (!dd2_model_draw_transparent(draw, draw->batches[index].material, cutout)) {
            continue;
        }
        size_t position = count++;
        const float distance = dd2_model_batch_distance(&draw->batches[index], eye);
        while (position > 0 && distance > dd2_model_batch_distance(
                                              &draw->batches[draw->order[position - 1]], eye)) {
            draw->order[position] = draw->order[position - 1];
            --position;
        }
        draw->order[position] = index;
    }
    return count;
}

bool dd2_model_draw_frame(dd2_model_draw *draw, dd2_model_draw_options options) {
    if (draw == NULL) {
        return false;
    }
    draw->stats = (dd2_model_draw_stats){0};
    glGetFloatv(GL_PROJECTION_MATRIX, draw->projection);
    glActiveTexture(GL_TEXTURE0);
    glClientActiveTexture(GL_TEXTURE0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    const dd2_model_vertex *vertices = dd2_model_vertices(draw->model);
    glVertexPointer(DD2_MODEL_DRAW_AXES, GL_FLOAT, sizeof(*vertices), vertices[0].position);
    glNormalPointer(GL_FLOAT, sizeof(*vertices), vertices[0].normal);
    glTexCoordPointer(2, GL_FLOAT, sizeof(*vertices), vertices[0].uv);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    if (options.cutout_textures) {
        const float threshold = 0.5F;
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, threshold);
    } else {
        glDisable(GL_ALPHA_TEST);
    }
    glDisable(GL_BLEND);
    if (options.double_sided) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
    }
    glCullFace(GL_BACK);
    glFrontFace(options.clockwise_front ? GL_CW : GL_CCW);
    dd2_model_draw_lights();
    if (options.lighting) {
        glEnable(GL_LIGHTING);
    } else {
        glDisable(GL_LIGHTING);
    }
    for (size_t index = 0; index < draw->batch_count; ++index) {
        if (!dd2_model_draw_transparent(draw, draw->batches[index].material,
                                        options.cutout_textures)) {
            dd2_model_draw_batch(draw, &draw->batches[index], options);
        }
    }
    const size_t transparent =
        dd2_model_transparent_order(draw, options.eye, options.cutout_textures);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    for (size_t index = 0; index < transparent; ++index) {
        dd2_model_draw_batch(draw, &draw->batches[draw->order[index]], options);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    return glGetError() == GL_NO_ERROR;
}

size_t dd2_model_draw_batch_count(const dd2_model_draw *draw) {
    return draw != NULL ? draw->batch_count : 0;
}

dd2_model_draw_stats dd2_model_draw_statistics(const dd2_model_draw *draw) {
    return draw != NULL ? draw->stats : (dd2_model_draw_stats){0};
}
