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
    DD2_KEY_DRIVE,
    DD2_KEY_PAUSE,
    DD2_KEY_BRAKE,
    DD2_KEY_WRECKING,
    DD2_KEY_STOCKCAR,
    DD2_KEY_WITHDRAW,
    DD2_KEY_TIME_TRIAL,
    DD2_KEY_TOTAL_DESTRUCTION,
    DD2_KEY_MUSIC,
    DD2_KEY_MUSIC_PREVIOUS,
    DD2_KEY_MUSIC_NEXT,
    DD2_KEY_CHAMP_WRECKING,
    DD2_KEY_CHAMP_STOCKCAR,
    DD2_KEY_PROFILE_NAME,
    DD2_KEY_PROFILE_SAVE,
    DD2_KEY_PROFILE_LOAD,
    DD2_KEY_BACKSPACE,
    DD2_KEY_PROFILE_DELETE,
    DD2_KEY_CAR_CLASS,
    DD2_KEY_COUNT
} dd2_key;

enum { DD2_INPUT_ACTION_LIMIT = 1 };
enum { DD2_INPUT_TEXT_BYTES = 32 };
typedef enum { DD2_INPUT_KEY_ACTION, DD2_INPUT_WHEEL_ACTION } dd2_input_action_kind;
typedef struct {
    dd2_input_action_kind kind;
    dd2_key key;
    int wheel;
} dd2_input_action;

typedef struct {
    bool held[DD2_KEY_COUNT];
    bool pressed[DD2_KEY_COUNT];
    dd2_input_action actions[DD2_INPUT_ACTION_LIMIT];
    unsigned action_count;
    bool focused;
    bool quit;
    bool redraw;
    int wheel;
    char text[DD2_INPUT_TEXT_BYTES];
} dd2_input;

/* Owns SDL video/window and a top-first presentation buffer. Logical framebuffer
 * size remains fixed; resizing letterboxes it. No system GL context is created.
 * Present copies bottom-first RGBA before returning; the source stays borrowed.
 * One window is supported on the application's main thread. */
dd2_window *dd2_window_create(dd2_render_options size);
void dd2_window_set_title(dd2_window *window, const char *title);
void dd2_window_destroy(dd2_window *window);
bool dd2_window_present(dd2_window *window, const uint8_t *rgba);
/* Poll stops after one keyboard command or wheel gesture, preserving later
 * control transitions and repeated presses in the SDL queue. The application
 * applies that command before polling the next frame. Focus loss discards queued
 * keyboard events. Held movement stays separate. */
dd2_input dd2_window_poll(dd2_window *window);
/* Clear held controls without dropping later queued commands. Focus loss also
 * discards queued keyboard events; autorepeat cannot restore released controls. */
void dd2_window_release_input(dd2_window *window);
/* Refresh held/focus observations after menu actions without polling events. */
void dd2_window_refresh_controls(const dd2_window *window, dd2_input *input);
void dd2_window_set_focus(dd2_window *window, bool focused);
/* Exclusive text entry consumes ordinary keyboard controls. Each text event
 * remains ordered against Enter/Escape/Backspace; focus loss drops queued text. */
void dd2_window_text_input(dd2_window *window, bool enabled);
/* Menu arrows are discrete ordered commands; camera/driving arrows remain held
 * movement. Switching modes releases held controls without dropping later input. */
void dd2_window_menu_input(dd2_window *window, bool enabled);
float dd2_window_elapsed(dd2_window *window);
void dd2_window_wait(void);

#endif
