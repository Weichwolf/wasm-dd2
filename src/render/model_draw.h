#ifndef DD2_RENDER_MODEL_DRAW_H
#define DD2_RENDER_MODEL_DRAW_H

#include "assets/image.h"
#include "assets/model.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct dd2_model_draw dd2_model_draw;
typedef struct dd2_model_texture_cache dd2_model_texture_cache;
typedef dd2_image *(*dd2_model_image_loader)(void *user, const char *resource);

typedef struct {
    float front_steer;
    float wheel_roll;
    float steering;
    float eye[3];
    bool lighting;
    bool clockwise_front;
    bool double_sided;
    /* Treat texture alpha as a coverage mask at 0.5 with depth writes.
     * Material opacity below one still uses the transparent pass. */
    bool cutout_textures;
    /* Submit only cockpit and steering roles for an interior camera. */
    bool cockpit_only;
} dd2_model_draw_options;

typedef struct {
    size_t tested;
    size_t role_filtered;
    size_t culled;
    size_t batches;
    size_t triangles;
} dd2_model_draw_stats;

/* Borrows the immutable model; owns batched indices and context-local albedo
 * textures with complete mip chains. Loader results transfer ownership and are
 * destroyed after upload. The same current GL context must outlive this cache.
 * The caller owns matrices/viewport and clear. Drawing owns lighting, texture,
 * blend, cull and client-array state and leaves arrays/lighting/textures/blending
 * disabled, depth writes enabled and texture unit zero active. Eye is model-local;
 * animation angles are degrees. Normal/roughness maps are not evaluated yet. */
dd2_model_draw *dd2_model_draw_create(const dd2_model *model, dd2_model_image_loader loader,
                                      void *user);
/* A shared context-local cache owns uploads, keyed by validated albedo paths.
 * It must outlive every shared draw and be destroyed before its GL context.
 * Loader results transfer ownership; loader user state is borrowed until cache
 * destruction. Missing/failed uploads are never cached. */
dd2_model_texture_cache *dd2_model_texture_cache_create(dd2_model_image_loader loader, void *user);
void dd2_model_texture_cache_destroy(dd2_model_texture_cache *cache);
size_t dd2_model_texture_cache_count(const dd2_model_texture_cache *cache);
dd2_model_draw *dd2_model_draw_create_shared(const dd2_model *model,
                                             dd2_model_texture_cache *cache);
void dd2_model_draw_destroy(dd2_model_draw *draw);
bool dd2_model_draw_frame(dd2_model_draw *draw, dd2_model_draw_options options);
size_t dd2_model_draw_batch_count(const dd2_model_draw *draw);
/* Last frame's actual submissions, after role selection and posed-bound culling.
 * Cached batch count is independent of the current view. NULL returns zeros. */
dd2_model_draw_stats dd2_model_draw_statistics(const dd2_model_draw *draw);

#endif
