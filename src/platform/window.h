#ifndef DD2_PLATFORM_WINDOW_H
#define DD2_PLATFORM_WINDOW_H

#include "render/renderer.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct dd2_window dd2_window;
typedef enum {
    DD2_KEY_LEFT,
    DD2_KEY_RIGHT,
    DD2_KEY_UP,
    DD2_KEY_DOWN,
    DD2_KEY_ZOOM_IN,
    DD2_KEY_ZOOM_OUT,
    DD2_KEY_PAN_LEFT,
    DD2_KEY_PAN_RIGHT,
    DD2_KEY_PAN_UP,
    DD2_KEY_PAN_DOWN,
    DD2_KEY_RESET,
    DD2_KEY_VIEW,
    DD2_KEY_PREVIOUS,
    DD2_KEY_NEXT,
    DD2_KEY_QUIT,
    DD2_KEY_COUNT
} dd2_key;

typedef struct {
    bool held[DD2_KEY_COUNT];
    bool pressed[DD2_KEY_COUNT];
    bool quit;
    bool redraw;
    int wheel;
} dd2_input;

/* Owns SDL video/window and a top-first presentation buffer. Logical framebuffer
 * size remains fixed; resizing letterboxes it. No system GL context is created.
 * Present copies bottom-first RGBA before returning; the source stays borrowed.
 * One window is supported on the application's main thread. */
dd2_window *dd2_window_create(dd2_render_options size);
void dd2_window_destroy(dd2_window *window);
bool dd2_window_present(dd2_window *window, const uint8_t *rgba);
dd2_input dd2_window_poll(dd2_window *window);
void dd2_window_release_input(dd2_window *window);
float dd2_window_elapsed(dd2_window *window);
void dd2_window_wait(void);

#endif
