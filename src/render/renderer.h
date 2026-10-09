#ifndef DD2_RENDER_RENDERER_H
#define DD2_RENDER_RENDERER_H

#include <stddef.h>
#include <stdint.h>

typedef struct dd2_renderer dd2_renderer;

typedef struct {
    int width;
    int height;
    /* Zero preserves the single-sample reference renderer; 2/4 enable MSAA. */
    int samples;
} dd2_render_options;

/* Contexts are owned by one render thread. Returned pixels belong to SoftGL
 * and remain valid until the next draw or context destruction. Row zero is
 * the bottom row; the eventual presentation adapter owns the vertical flip. */
dd2_renderer *dd2_renderer_create(const dd2_render_options *options);
void dd2_renderer_destroy(dd2_renderer *renderer);
void dd2_renderer_make_current(dd2_renderer *renderer);
size_t dd2_renderer_rgba_bytes(const dd2_renderer *renderer);
const uint8_t *dd2_renderer_pixels(dd2_renderer *renderer);

#endif
