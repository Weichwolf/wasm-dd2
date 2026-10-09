#include "platform/window.h"
#include "render/renderer.h"

#include <SDL_events.h>
#include <SDL_scancode.h>
#include <SDL_stdinc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Uint32 type;
    Uint8 repeat;
} dd2_input_test_event;

static bool dd2_input_test_key(SDL_Scancode code, dd2_input_test_event command) {
    SDL_Event event = {0};
    event.type = command.type;
    event.key.keysym.scancode = code;
    event.key.repeat = command.repeat;
    return SDL_PushEvent(&event) == 1;
}

static bool dd2_input_test_press(SDL_Scancode code) {
    return dd2_input_test_key(code, (dd2_input_test_event){.type = SDL_KEYDOWN}) &&
           dd2_input_test_key(code, (dd2_input_test_event){.type = SDL_KEYUP});
}

static bool dd2_input_test_order(dd2_window *window) {
    const SDL_Scancode codes[] = {SDL_SCANCODE_F8,    SDL_SCANCODE_P, SDL_SCANCODE_R,
                                  SDL_SCANCODE_F7,    SDL_SCANCODE_P, SDL_SCANCODE_P,
                                  SDL_SCANCODE_DELETE};
    const dd2_key expected[] = {DD2_KEY_TIME_TRIAL,    DD2_KEY_PAUSE, DD2_KEY_RESET,
                                DD2_KEY_WITHDRAW,      DD2_KEY_PAUSE, DD2_KEY_PAUSE,
                                DD2_KEY_PROFILE_DELETE};
    for (size_t key = 0; key < sizeof(codes) / sizeof(codes[0]); ++key) {
        if (!dd2_input_test_press(codes[key])) {
            return false;
        }
    }
    for (unsigned action = 0; action < sizeof(expected) / sizeof(expected[0]); ++action) {
        const dd2_input input = dd2_window_poll(window);
        if (input.action_count != 1 || input.actions[0].kind != DD2_INPUT_KEY_ACTION ||
            input.actions[0].key != expected[action]) {
            return false;
        }
        dd2_window_release_input(window);
    }
    /* Drain the final key-up and SDL pump boundary before the next batch. */
    return dd2_window_poll(window).action_count == 0;
}

static bool dd2_input_test_pressure(dd2_window *window) {
    enum { DD2_INPUT_TEST_PRESSURES = 66 };
    for (unsigned action = 0; action < DD2_INPUT_TEST_PRESSURES; ++action) {
        if (!dd2_input_test_press(SDL_SCANCODE_R)) {
            return false;
        }
    }
    for (unsigned action = 0; action < DD2_INPUT_TEST_PRESSURES; ++action) {
        const dd2_input input = dd2_window_poll(window);
        if (input.action_count != 1 || input.actions[0].key != DD2_KEY_RESET) {
            printf("Queued reset %u: actions=%u key=%u\n", action, input.action_count,
                   (unsigned)input.actions[0].key);
            return false;
        }
        dd2_window_release_input(window);
    }
    return dd2_window_poll(window).action_count == 0;
}

static bool dd2_input_test_release(dd2_window *window) {
    if (!dd2_input_test_key(SDL_SCANCODE_W, (dd2_input_test_event){.type = SDL_KEYDOWN})) {
        return false;
    }
    dd2_input input = dd2_window_poll(window);
    if (input.action_count != 0 || !input.held[DD2_KEY_PAN_UP]) {
        return false;
    }
    dd2_window_release_input(window);
    dd2_window_refresh_controls(window, &input);
    if (input.held[DD2_KEY_PAN_UP] ||
        !dd2_input_test_key(SDL_SCANCODE_W,
                            (dd2_input_test_event){.type = SDL_KEYDOWN, .repeat = 1})) {
        return false;
    }
    input = dd2_window_poll(window);
    if (input.held[DD2_KEY_PAN_UP] || input.action_count != 0 ||
        !dd2_input_test_press(SDL_SCANCODE_P)) {
        return false;
    }
    dd2_window_set_focus(window, false);
    input = dd2_window_poll(window);
    return !input.focused && input.action_count == 0;
}

static bool dd2_input_test_wheel(dd2_window *window) {
    SDL_Event event = {0};
    event.type = SDL_MOUSEWHEEL;
    event.wheel.y = 1;
    if (!dd2_input_test_press(SDL_SCANCODE_R) || SDL_PushEvent(&event) != 1 ||
        !dd2_input_test_press(SDL_SCANCODE_R)) {
        return false;
    }
    const dd2_input first = dd2_window_poll(window);
    dd2_window_release_input(window);
    const dd2_input wheel = dd2_window_poll(window);
    const dd2_input last = dd2_window_poll(window);
    return first.action_count == 1 && first.actions[0].key == DD2_KEY_RESET &&
           wheel.action_count == 1 && wheel.actions[0].kind == DD2_INPUT_WHEEL_ACTION &&
           wheel.actions[0].wheel == 1 && last.action_count == 1 &&
           last.actions[0].key == DD2_KEY_RESET;
}

static bool dd2_input_test_text(dd2_window *window) {
    dd2_window_poll(window);
    dd2_window_set_focus(window, true);
    dd2_window_text_input(window, true);
    SDL_Event event = {0};
    event.type = SDL_TEXTINPUT;
    event.text.text[0] = 'A';
    event.text.text[1] = 'b';
    if (!dd2_input_test_press(SDL_SCANCODE_C) || SDL_PushEvent(&event) != 1 ||
        !dd2_input_test_press(SDL_SCANCODE_BACKSPACE) ||
        !dd2_input_test_press(SDL_SCANCODE_RETURN)) {
        return false;
    }
    const dd2_input text = dd2_window_poll(window);
    const dd2_input backspace = dd2_window_poll(window);
    dd2_window_release_input(window);
    const dd2_input enter = dd2_window_poll(window);
    dd2_window_release_input(window);
    const bool ordered = strcmp(text.text, "Ab") == 0 && text.action_count == 0 &&
                         !text.held[DD2_KEY_CHAMP_WRECKING] && backspace.action_count == 1 &&
                         backspace.actions[0].key == DD2_KEY_BACKSPACE && enter.action_count == 1 &&
                         enter.actions[0].key == DD2_KEY_DRIVE;
    if (SDL_PushEvent(&event) != 1) {
        return false;
    }
    dd2_window_set_focus(window, false);
    const bool dropped = dd2_window_poll(window).text[0] == '\0';
    dd2_window_text_input(window, false);
    dd2_window_set_focus(window, true);
    return ordered && dropped;
}

static bool dd2_input_test_menu(dd2_window *window) {
    dd2_window_poll(window);
    dd2_window_set_focus(window, true);
    dd2_window_menu_input(window, true);
    const SDL_Scancode codes[] = {SDL_SCANCODE_RIGHT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_LEFT};
    const dd2_key keys[] = {DD2_KEY_RIGHT, DD2_KEY_RIGHT, DD2_KEY_LEFT};
    for (unsigned index = 0; index < sizeof(codes) / sizeof(codes[0]); ++index) {
        if (!dd2_input_test_press(codes[index])) {
            return false;
        }
    }
    for (unsigned index = 0; index < sizeof(keys) / sizeof(keys[0]); ++index) {
        const dd2_input input = dd2_window_poll(window);
        if (input.action_count != 1 || input.actions[0].key != keys[index]) {
            return false;
        }
        dd2_window_release_input(window);
    }
    dd2_window_poll(window);
    dd2_window_menu_input(window, false);
    if (!dd2_input_test_key(SDL_SCANCODE_RIGHT, (dd2_input_test_event){.type = SDL_KEYDOWN})) {
        return false;
    }
    const dd2_input held = dd2_window_poll(window);
    dd2_window_release_input(window);
    return held.action_count == 0 && held.held[DD2_KEY_RIGHT];
}

int main(void) {
    enum { DD2_INPUT_TEST_SIDE = 64 };
    dd2_window *window = dd2_window_create(
        (dd2_render_options){.width = DD2_INPUT_TEST_SIDE, .height = DD2_INPUT_TEST_SIDE});
    if (window == NULL) {
        return EXIT_FAILURE;
    }
    dd2_window_poll(window);
    dd2_window_set_focus(window, true);
    const bool order = dd2_input_test_order(window);
    const bool pressure = dd2_input_test_pressure(window);
    const bool release = dd2_input_test_release(window);
    const bool wheel = dd2_input_test_wheel(window);
    const bool text = dd2_input_test_text(window);
    const bool menu = dd2_input_test_menu(window);
    dd2_window_destroy(window);
    printf("Synthetic SDL input: order=%u pressure=%u release=%u wheel=%u\n", (unsigned)order,
           (unsigned)pressure, (unsigned)release, (unsigned)wheel);
    return order && pressure && release && wheel && text && menu ? EXIT_SUCCESS : EXIT_FAILURE;
}
