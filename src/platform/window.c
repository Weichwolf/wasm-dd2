#include "platform/window.h"

#include "render/renderer.h"

#include <SDL.h>
#include <SDL_blendmode.h>
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_pixels.h>
#include <SDL_rect.h>
#include <SDL_scancode.h>
#include <SDL_surface.h>
#include <SDL_timer.h>
#include <SDL_video.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    DD2_WINDOW_CHANNELS = 4,
    DD2_WINDOW_PIXEL_BITS = 32,
    DD2_WINDOW_MAX_SIDE = 4096,
    DD2_WINDOW_WAIT_MS = 8
};
static const float dd2_window_max_elapsed = 0.25F;

struct dd2_window {
    SDL_Window *native;
    SDL_Surface *frame;
    uint8_t *top_rows;
    dd2_render_options size;
    bool held[DD2_KEY_COUNT];
    uint64_t previous;
    bool focused;
    bool text_input;
    bool menu_input;
};

void dd2_window_destroy(dd2_window *window) {
    if (window != NULL) {
        SDL_FreeSurface(window->frame);
        SDL_DestroyWindow(window->native);
        free(window->top_rows);
        free(window);
        SDL_Quit();
    }
}

dd2_window *dd2_window_create(dd2_render_options size) {
    if (size.width <= 0 || size.height <= 0 || size.width > DD2_WINDOW_MAX_SIDE ||
        size.height > DD2_WINDOW_MAX_SIDE) {
        return NULL;
    }
    dd2_window *window = calloc(1, sizeof(*window));
    if (window == NULL) {
        return NULL;
    }
    SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
    SDL_SetHint("SDL_EMSCRIPTEN_KEYBOARD_ELEMENT", "#canvas");
    window->size = size;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        dd2_window_destroy(window);
        return NULL;
    }
    window->native =
        SDL_CreateWindow("Destruction Derby 2 - Track viewer", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, size.width, size.height, SDL_WINDOW_RESIZABLE);
    window->top_rows = malloc((size_t)size.width * (size_t)size.height * DD2_WINDOW_CHANNELS);
    if (window->native != NULL && window->top_rows != NULL) {
        window->frame = SDL_CreateRGBSurfaceWithFormatFrom(
            window->top_rows, size.width, size.height, DD2_WINDOW_PIXEL_BITS,
            size.width * DD2_WINDOW_CHANNELS, SDL_PIXELFORMAT_RGBA32);
    }
    if (window->frame == NULL || SDL_SetSurfaceBlendMode(window->frame, SDL_BLENDMODE_NONE) != 0) {
        dd2_window_destroy(window);
        return NULL;
    }
    window->focused = true;
    window->previous = SDL_GetPerformanceCounter();
    return window;
}

static SDL_Rect dd2_window_rectangle(const dd2_window *window, const SDL_Surface *surface) {
    SDL_Rect rectangle = {.w = surface->w, .h = surface->h};
    if ((int64_t)surface->w * window->size.height > (int64_t)surface->h * window->size.width) {
        rectangle.w = (int)((int64_t)surface->h * window->size.width / window->size.height);
    } else {
        rectangle.h = (int)((int64_t)surface->w * window->size.height / window->size.width);
    }
    rectangle.x = (surface->w - rectangle.w) / 2;
    rectangle.y = (surface->h - rectangle.h) / 2;
    return rectangle;
}

bool dd2_window_present(dd2_window *window, const uint8_t *rgba) {
    if (window == NULL || rgba == NULL) {
        return false;
    }
    const size_t stride = (size_t)window->size.width * DD2_WINDOW_CHANNELS;
    for (size_t row = 0; row < (size_t)window->size.height; ++row) {
        const size_t source = ((size_t)window->size.height - row - 1) * stride;
        for (size_t column = 0; column < stride; ++column) {
            window->top_rows[(row * stride) + column] = rgba[source + column];
        }
    }
    SDL_Surface *surface = SDL_GetWindowSurface(window->native);
    if (surface == NULL) {
        return false;
    }
    SDL_Rect rectangle = dd2_window_rectangle(window, surface);
    return SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0)) == 0 &&
           SDL_BlitScaled(window->frame, NULL, surface, &rectangle) == 0 &&
           SDL_UpdateWindowSurface(window->native) == 0;
}

static dd2_key dd2_window_key(SDL_Scancode code) {
    switch (code) {
    case SDL_SCANCODE_LEFT:
        return DD2_KEY_LEFT;
    case SDL_SCANCODE_RIGHT:
        return DD2_KEY_RIGHT;
    case SDL_SCANCODE_UP:
        return DD2_KEY_UP;
    case SDL_SCANCODE_DOWN:
        return DD2_KEY_DOWN;
    case SDL_SCANCODE_A:
        return DD2_KEY_PAN_LEFT;
    case SDL_SCANCODE_D:
        return DD2_KEY_PAN_RIGHT;
    case SDL_SCANCODE_W:
        return DD2_KEY_PAN_UP;
    case SDL_SCANCODE_S:
        return DD2_KEY_PAN_DOWN;
    case SDL_SCANCODE_R:
        return DD2_KEY_RESET;
    case SDL_SCANCODE_F1:
        return DD2_KEY_CAR_CLASS;
    case SDL_SCANCODE_TAB:
        return DD2_KEY_VIEW;
    case SDL_SCANCODE_PAGEDOWN:
        return DD2_KEY_PREVIOUS;
    case SDL_SCANCODE_PAGEUP:
        return DD2_KEY_NEXT;
    case SDL_SCANCODE_RETURN:
        return DD2_KEY_DRIVE;
    case SDL_SCANCODE_P:
        return DD2_KEY_PAUSE;
    case SDL_SCANCODE_SPACE:
        return DD2_KEY_BRAKE;
    case SDL_SCANCODE_F5:
        return DD2_KEY_WRECKING;
    case SDL_SCANCODE_F6:
        return DD2_KEY_STOCKCAR;
    case SDL_SCANCODE_F9:
        return DD2_KEY_TOTAL_DESTRUCTION;
    case SDL_SCANCODE_F10:
        return DD2_KEY_MUSIC;
    case SDL_SCANCODE_F11:
        return DD2_KEY_MUSIC_PREVIOUS;
    case SDL_SCANCODE_F12:
        return DD2_KEY_MUSIC_NEXT;
    case SDL_SCANCODE_F8:
        return DD2_KEY_TIME_TRIAL;
    case SDL_SCANCODE_C:
        return DD2_KEY_CHAMP_WRECKING;
    case SDL_SCANCODE_N:
        return DD2_KEY_CHAMP_STOCKCAR;
    case SDL_SCANCODE_F7:
        return DD2_KEY_WITHDRAW;
    case SDL_SCANCODE_ESCAPE:
        return DD2_KEY_QUIT;
    case SDL_SCANCODE_F2:
        return DD2_KEY_PROFILE_NAME;
    case SDL_SCANCODE_F3:
        return DD2_KEY_PROFILE_SAVE;
    case SDL_SCANCODE_F4:
        return DD2_KEY_PROFILE_LOAD;
    case SDL_SCANCODE_BACKSPACE:
        return DD2_KEY_BACKSPACE;
    case SDL_SCANCODE_DELETE:
        return DD2_KEY_PROFILE_DELETE;
    default:
        return DD2_KEY_COUNT;
    }
}

void dd2_window_release_input(dd2_window *window) {
    if (window != NULL) {
        for (size_t index = 0; index < DD2_KEY_COUNT; ++index) {
            window->held[index] = false;
        }
    }
}

static void dd2_window_keyboard(dd2_window *window, dd2_input *input,
                                const SDL_KeyboardEvent *event) {
    if (window->text_input && event->keysym.scancode != SDL_SCANCODE_RETURN &&
        event->keysym.scancode != SDL_SCANCODE_ESCAPE &&
        event->keysym.scancode != SDL_SCANCODE_BACKSPACE) {
        return;
    }
    dd2_key key = dd2_window_key(event->keysym.scancode);
    if (event->keysym.sym == SDLK_PLUS || event->keysym.sym == SDLK_EQUALS ||
        event->keysym.sym == SDLK_KP_PLUS) {
        key = DD2_KEY_ZOOM_IN;
    } else if (event->keysym.sym == SDLK_MINUS || event->keysym.sym == SDLK_KP_MINUS) {
        key = DD2_KEY_ZOOM_OUT;
    }
    if (key == DD2_KEY_COUNT) {
        return;
    }
    const bool down = event->type == SDL_KEYDOWN;
    const bool menu_repeat = (window->text_input && key == DD2_KEY_BACKSPACE) ||
                             (window->menu_input && (key == DD2_KEY_LEFT || key == DD2_KEY_RIGHT));
    if (down && event->repeat != 0 && !menu_repeat) {
        return;
    }
    if (down && (!window->held[key] || (event->repeat != 0 && menu_repeat))) {
        input->pressed[key] = true;
        if ((key >= DD2_KEY_RESET && key != DD2_KEY_BRAKE) ||
            (window->menu_input && (key == DD2_KEY_LEFT || key == DD2_KEY_RIGHT))) {
            input->actions[input->action_count++] =
                (dd2_input_action){.kind = DD2_INPUT_KEY_ACTION, .key = key};
        }
    }
    window->held[key] = down;
}

static void dd2_window_text(const dd2_window *window, dd2_input *input,
                            const SDL_TextInputEvent *event) {
    if (!window->text_input || !window->focused) {
        return;
    }
    for (unsigned index = 0; index + 1 < sizeof(input->text); ++index) {
        input->text[index] = event->text[index];
        if (event->text[index] == '\0') {
            break;
        }
    }
}

dd2_input dd2_window_poll(dd2_window *window) {
    dd2_input input = {0};
    if (window == NULL) {
        input.quit = true;
        return input;
    }
    SDL_Event event = {0};
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
        case SDL_QUIT:
            input.quit = true;
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            dd2_window_keyboard(window, &input, &event.key);
            break;
        case SDL_TEXTINPUT:
            dd2_window_text(window, &input, &event.text);
            break;
        case SDL_MOUSEWHEEL:
            if (event.wheel.y != 0) {
                const int direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
                const int motion = (event.wheel.y > 0 ? 1 : -1) * direction;
                input.wheel += motion;
                input.actions[input.action_count++] =
                    (dd2_input_action){.kind = DD2_INPUT_WHEEL_ACTION, .wheel = motion};
            }
            break;
        case SDL_WINDOWEVENT:
            input.redraw = true;
            if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                dd2_window_set_focus(window, false);
            }
            if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                dd2_window_set_focus(window, true);
            }
            break;
        default:
            break;
        }
        if (input.action_count == DD2_INPUT_ACTION_LIMIT || input.text[0] != '\0') {
            break;
        }
    }
    dd2_window_refresh_controls(window, &input);
    return input;
}

void dd2_window_text_input(dd2_window *window, bool enabled) {
    if (window != NULL && window->text_input != enabled) {
        dd2_window_release_input(window);
        window->text_input = enabled;
        if (enabled) {
            SDL_StartTextInput();
        } else {
            SDL_StopTextInput();
        }
    }
}

void dd2_window_menu_input(dd2_window *window, bool enabled) {
    if (window != NULL && window->menu_input != enabled) {
        dd2_window_release_input(window);
        window->menu_input = enabled;
    }
}

void dd2_window_refresh_controls(const dd2_window *window, dd2_input *input) {
    if (window == NULL || input == NULL) {
        return;
    }
    for (size_t index = 0; index < DD2_KEY_COUNT; ++index) {
        input->held[index] = window->held[index];
    }
    input->focused = window->focused;
}

void dd2_window_set_focus(dd2_window *window, bool focused) {
    if (window != NULL) {
        if (window->focused != focused) {
            window->previous = SDL_GetPerformanceCounter();
        }
        window->focused = focused;
        if (!focused) {
            SDL_FlushEvents(SDL_KEYDOWN, SDL_KEYUP);
            SDL_FlushEvents(SDL_TEXTEDITING, SDL_TEXTINPUT);
            dd2_window_release_input(window);
        }
    }
}

float dd2_window_elapsed(dd2_window *window) {
    if (window == NULL) {
        return 0;
    }
    const uint64_t current = SDL_GetPerformanceCounter();
    const float seconds =
        (float)((double)(current - window->previous) / (double)SDL_GetPerformanceFrequency());
    window->previous = current;
    return seconds < dd2_window_max_elapsed ? seconds : dd2_window_max_elapsed;
}

void dd2_window_wait(void) {
    SDL_Delay(DD2_WINDOW_WAIT_MS);
}
