#ifndef DD2_RENDER_SKY_DRAW_H
#define DD2_RENDER_SKY_DRAW_H

#include "assets/track.h"
#include "render/model_draw.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct dd2_sky_draw dd2_sky_draw;
typedef struct {
    size_t tested;
    size_t visible;
    size_t culled;
    size_t batches;
    size_t triangles;
} dd2_sky_draw_stats;

/* Borrows eight immutable prepared patches and a shared context-local texture
 * cache. Both track/cache and the current GL context must outlive this owner.
 * Frame uses the caller's projection and modelview rotation, ignores camera
 * translation, culls patches before lazy upload and writes no depth. Matrices
 * are restored; depth testing/writes are enabled on return. */
dd2_sky_draw *dd2_sky_draw_create(const dd2_track *track, dd2_model_texture_cache *cache);
void dd2_sky_draw_destroy(dd2_sky_draw *draw);
bool dd2_sky_draw_frame(dd2_sky_draw *draw);
dd2_sky_draw_stats dd2_sky_draw_statistics(const dd2_sky_draw *draw);

#endif
