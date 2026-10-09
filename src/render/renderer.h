#ifndef DD2_RENDER_RENDERER_H
#define DD2_RENDER_RENDERER_H

#include <stddef.h>
#include <stdint.h>

typedef struct dd2_renderer dd2_renderer;

typedef enum {
    DD2_RENDER_RAW = 0,
    DD2_RENDER_LINEAR_TO_SRGB = 1,
    DD2_RENDER_OUTPUT_COUNT = 2
} dd2_render_output;

typedef struct {
    int width;
    int height;
    /* Zero preserves the single-sample reference renderer; 2/4 enable MSAA. */
    int samples;
    /* Authored scenes render in linear light and encode RGB after resolve.
     * Zero retains the raw reference renderer and allocates no display copy. */
    dd2_render_output output;
} dd2_render_options;

/* Contexts are owned by one render thread. Returned pixels are borrowed from
 * SoftGL or the adapter's display copy until the next read/draw or destruction.
 * The display transform never modifies SoftGL's linear framebuffer. Row zero
 * is the bottom row; the presentation adapter owns the vertical flip. */
dd2_renderer *dd2_renderer_create(const dd2_render_options *options);
void dd2_renderer_destroy(dd2_renderer *renderer);
void dd2_renderer_make_current(dd2_renderer *renderer);
size_t dd2_renderer_rgba_bytes(const dd2_renderer *renderer);
const uint8_t *dd2_renderer_pixels(dd2_renderer *renderer);

#endif
