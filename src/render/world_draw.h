#ifndef DD2_RENDER_WORLD_DRAW_H
#define DD2_RENDER_WORLD_DRAW_H

#include "assets/world.h"
#include "render/model_draw.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct dd2_world_draw dd2_world_draw;
typedef struct {
    size_t tested;
    size_t visible;
    size_t culled;
    size_t triangles;
    size_t batches;
    size_t uploaded_textures;
} dd2_world_draw_stats;

/* Borrows the world; owns lazily created draw batches and shared context-local
 * textures. Accurate model bounds are validated by the world owner at load.
 * Frame uses caller matrices and rejects invisible AABBs before creating caches
 * or submitting geometry. The GL context and world must outlive the cache. */
dd2_world_draw *dd2_world_draw_create(const dd2_world *world, dd2_model_image_loader loader,
                                      void *user);
void dd2_world_draw_destroy(dd2_world_draw *draw);
bool dd2_world_draw_frame(dd2_world_draw *draw, dd2_model_draw_options options,
                          dd2_world_draw_stats *stats);

#endif
