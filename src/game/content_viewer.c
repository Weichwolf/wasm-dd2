#include "game/content_viewer.h"

#include "assets/bytes.h"
#include "assets/image.h"
#include "assets/model.h"
#include "platform/file.h"
#include "platform/window.h"
#include "render/model_draw.h"
#include "render/model_view.h"
#include "render/renderer.h"

#include <GL/softgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

enum {
    DD2_CONTENT_LODS = 3,
    DD2_CONTENT_WIDTH = 640,
    DD2_CONTENT_HEIGHT = 360,
    DD2_CONTENT_SAMPLES = 4,
    DD2_CONTENT_CHANNELS = 4,
    DD2_CONTENT_RGB = 3,
    DD2_CONTENT_GRID_HALF = 10,
    DD2_CONTENT_PATH_BYTES = 4096
};
static const float dd2_content_front_steer = 28;
static const float dd2_content_wheel_roll = 42;
static const float dd2_content_steering = -90;
static const float dd2_content_grid_spacing = 2;

static const dd2_render_options dd2_content_viewport = {
    .width = DD2_CONTENT_WIDTH, .height = DD2_CONTENT_HEIGHT, .samples = DD2_CONTENT_SAMPLES};

typedef struct {
    dd2_renderer *renderer;
    dd2_window *window;
    dd2_model *models[DD2_CONTENT_LODS];
    dd2_model_draw *draws[DD2_CONTENT_LODS];
    dd2_model_view view;
    int detail;
    bool pose;
    bool running;
    bool failed;
    bool main_loop;
} dd2_content_state;

static dd2_content_state *dd2_content_current;

static bool dd2_content_read(const char *root, const char *resource, dd2_file *file) {
    const size_t prefix = strlen(root);
    const size_t name = strlen(resource);
    if (prefix + name + 2 > DD2_CONTENT_PATH_BYTES) {
        return false;
    }
    char path[DD2_CONTENT_PATH_BYTES] = {0};
    for (size_t index = 0; index < prefix; ++index) {
        path[index] = root[index];
    }
    path[prefix] = '/';
    for (size_t index = 0; index < name; ++index) {
        path[prefix + index + 1] = resource[index];
    }
    return dd2_file_read(path, file);
}

static dd2_image *dd2_content_image(void *user, const char *resource) {
    dd2_file file = {0};
    if (!dd2_content_read(user, resource, &file)) {
        return NULL;
    }
    dd2_image *image = dd2_image_create_png((dd2_byte_view){file.data, file.size});
    dd2_file_release(&file);
    return image;
}

void dd2_content_close(void) {
    dd2_content_state *state = dd2_content_current;
    if (state == NULL) {
        return;
    }
    dd2_content_current = NULL;
#ifdef __EMSCRIPTEN__
    if (state->main_loop) {
        emscripten_cancel_main_loop();
    }
#endif
    dd2_renderer_make_current(state->renderer);
    for (size_t index = 0; index < DD2_CONTENT_LODS; ++index) {
        dd2_model_draw_destroy(state->draws[index]);
        dd2_model_destroy(state->models[index]);
    }
    dd2_renderer_destroy(state->renderer);
    dd2_window_destroy(state->window);
    free(state);
}

static bool dd2_content_open(const char *root, bool window) {
    if (root == NULL || dd2_content_current != NULL) {
        return false;
    }
    dd2_content_state *state = calloc(1, sizeof(*state));
    if (state == NULL) {
        return false;
    }
    dd2_content_current = state;
    state->renderer = dd2_renderer_create(&dd2_content_viewport);
    if (state->renderer == NULL) {
        dd2_content_close();
        return false;
    }
    if (window) {
        state->window = dd2_window_create(dd2_content_viewport);
        if (state->window == NULL) {
            dd2_content_close();
            return false;
        }
        dd2_window_set_title(state->window, "Destruction Derby 2 - Racer R1 preview");
    }
    const char *names[] = {"models/racer-r1.dd2mesh", "models/racer-r1-exterior.dd2mesh",
                           "models/racer-r1-npc.dd2mesh"};
    for (size_t index = 0; index < DD2_CONTENT_LODS; ++index) {
        dd2_file file = {0};
        if (!dd2_content_read(root, names[index], &file)) {
            dd2_content_close();
            return false;
        }
        state->models[index] = dd2_model_create((dd2_byte_view){file.data, file.size});
        dd2_file_release(&file);
        state->draws[index] =
            dd2_model_draw_create(state->models[index], dd2_content_image, (void *)root);
        if (state->draws[index] == NULL) {
            dd2_content_close();
            return false;
        }
    }
    state->view = dd2_model_view_default(false);
    state->running = true;
    return true;
}

static void dd2_content_grid(void) {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glBegin(GL_TRIANGLES);
    for (int row = -DD2_CONTENT_GRID_HALF; row < DD2_CONTENT_GRID_HALF; ++row) {
        for (int column = -DD2_CONTENT_GRID_HALF; column < DD2_CONTENT_GRID_HALF; ++column) {
            const float shade = (row + column) % 2 == 0 ? 0.18F : 0.21F;
            glColor3f(shade, shade, shade);
            const float left = (float)column * dd2_content_grid_spacing;
            const float back = (float)row * dd2_content_grid_spacing;
            const float right = left + dd2_content_grid_spacing;
            const float front = back + dd2_content_grid_spacing;
            glVertex3f(left, 0, back);
            glVertex3f(right, 0, back);
            glVertex3f(right, 0, front);
            glVertex3f(left, 0, back);
            glVertex3f(right, 0, front);
            glVertex3f(left, 0, front);
        }
    }
    glEnd();
}

static bool dd2_content_draw(dd2_content_state *state) {
    dd2_renderer_make_current(state->renderer);
    dd2_model_draw_options options = dd2_model_view_apply(&state->view, dd2_content_viewport);
    if (state->pose) {
        options.front_steer = dd2_content_front_steer;
        options.wheel_roll = dd2_content_wheel_roll;
        options.steering = dd2_content_steering;
    }
    /* Diagnostic checker floor; this is not an authored playable course. */
    dd2_content_grid();
    return dd2_model_draw_frame(state->draws[state->detail], options);
}

int dd2_content_present(void) {
    dd2_content_state *state = dd2_content_current;
    if (state == NULL || !dd2_content_draw(state) ||
        (state->window != NULL &&
         !dd2_window_present(state->window, dd2_renderer_pixels(state->renderer)))) {
        return 0;
    }
    return 1;
}

int dd2_content_select(int detail, int cockpit) {
    dd2_content_state *state = dd2_content_current;
    if (state == NULL || detail < 0 || detail >= DD2_CONTENT_LODS ||
        (cockpit != 0 && cockpit != 1)) {
        return 0;
    }
    state->detail = cockpit != 0 ? 0 : detail;
    state->view = dd2_model_view_default(cockpit != 0);
    return dd2_content_present();
}

int dd2_content_set_pose(int enabled) {
    if (dd2_content_current == NULL || (enabled != 0 && enabled != 1)) {
        return 0;
    }
    dd2_content_current->pose = enabled != 0;
    return dd2_content_present();
}

int dd2_content_detail(void) {
    return dd2_content_current != NULL ? dd2_content_current->detail : -1;
}

int dd2_content_cockpit(void) {
    return dd2_content_current != NULL && dd2_content_current->view.cockpit;
}

int dd2_content_pose(void) {
    return dd2_content_current != NULL && dd2_content_current->pose;
}

static void dd2_content_frame(void *user) {
    dd2_content_state *state = user;
    const dd2_input input = dd2_window_poll(state->window);
    if (input.quit || input.pressed[DD2_KEY_QUIT]) {
        state->running = false;
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
        dd2_content_close();
#endif
        return;
    }
    bool redraw = input.redraw;
    if (input.pressed[DD2_KEY_VIEW]) {
        state->view = dd2_model_view_default(!state->view.cockpit);
        state->detail = 0;
        redraw = true;
    }
    if (input.pressed[DD2_KEY_NEXT] || input.pressed[DD2_KEY_PREVIOUS]) {
        state->detail = (state->detail + (input.pressed[DD2_KEY_NEXT] ? 1 : DD2_CONTENT_LODS - 1)) %
                        DD2_CONTENT_LODS;
        state->view = dd2_model_view_default(false);
        redraw = true;
    }
    if (input.pressed[DD2_KEY_RESET]) {
        state->view = dd2_model_view_default(state->view.cockpit);
        state->pose = false;
        redraw = true;
    }
    if (input.pressed[DD2_KEY_BRAKE]) {
        state->pose = !state->pose;
        redraw = true;
    }
    const dd2_model_view_motion motion = {
        .yaw = (float)input.held[DD2_KEY_RIGHT] - (float)input.held[DD2_KEY_LEFT],
        .pitch = (float)input.held[DD2_KEY_UP] - (float)input.held[DD2_KEY_DOWN],
        .zoom = (float)input.held[DD2_KEY_ZOOM_OUT] - (float)input.held[DD2_KEY_ZOOM_IN]};
    const float seconds = dd2_window_elapsed(state->window);
    if (input.focused && dd2_model_view_step(&state->view, motion, seconds)) {
        redraw = true;
    }
    if (redraw && !dd2_content_present()) {
        state->failed = true;
        state->running = false;
#ifdef __EMSCRIPTEN__
        dd2_content_close();
#endif
    }
}

static bool dd2_content_write(dd2_renderer *renderer, const char *path) {
    const char allowed[] = "/tmp/wasm-dd2/";
    if (path == NULL || strncmp(path, allowed, sizeof(allowed) - 1) != 0 ||
        strstr(path, "..") != NULL) {
        return false;
    }
    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    FILE *file = pixels != NULL ? fopen(path, "wb") : NULL;
    if (file == NULL) {
        return false;
    }
    const char header[] = "P6\n640 360\n255\n";
    bool passed = fwrite(header, 1, sizeof(header) - 1, file) == sizeof(header) - 1;
    uint8_t row[DD2_CONTENT_WIDTH * DD2_CONTENT_RGB] = {0};
    for (size_t y_pos = 0; y_pos < DD2_CONTENT_HEIGHT && passed; ++y_pos) {
        for (size_t x_pos = 0; x_pos < DD2_CONTENT_WIDTH; ++x_pos) {
            const size_t source = (((DD2_CONTENT_HEIGHT - y_pos - 1) * DD2_CONTENT_WIDTH) + x_pos) *
                                  DD2_CONTENT_CHANNELS;
            for (size_t channel = 0; channel < DD2_CONTENT_RGB; ++channel) {
                row[(x_pos * DD2_CONTENT_RGB) + channel] = pixels[source + channel];
            }
        }
        passed = fwrite(row, 1, sizeof(row), file) == sizeof(row);
    }
    const bool closed = fclose(file) == 0;
    return passed && closed;
}

bool dd2_content_capture(dd2_content_capture_options options) {
    if (!dd2_content_open(options.root, false)) {
        return false;
    }
    bool passed = dd2_content_select(options.selection.detail, options.selection.cockpit) != 0 &&
                  dd2_content_set_pose(options.selection.pose) != 0;
    dd2_content_state *state = dd2_content_current;
    passed = passed && dd2_content_write(state->renderer, options.path);
    GLint samples = 0;
    glGetIntegerv(GL_SAMPLES, &samples);
    printf("{\"scope\":\"authored model diagnostic "
           "only\",\"samples\":%d,\"batches\":%zu,\"triangles\":%zu}\n",
           samples, dd2_model_draw_batch_count(state->draws[state->detail]),
           dd2_model_index_count(state->models[state->detail]) / DD2_CONTENT_LODS);
    passed = passed && samples == DD2_CONTENT_SAMPLES && glGetError() == GL_NO_ERROR;
    dd2_content_close();
    return passed;
}

int dd2_content_run(const char *root) {
    if (!dd2_content_open(root, true) || !dd2_content_present()) {
        dd2_content_close();
        puts("Authored vehicle content could not be loaded or drawn.");
        return EXIT_FAILURE;
    }
    puts("Racer R1 preview: Tab cockpit; Page Up/Down detail; arrows camera; +/- zoom; Space "
         "wheels; R reset.");
    dd2_content_state *state = dd2_content_current;
#ifdef __EMSCRIPTEN__
    state->main_loop = true;
    emscripten_set_main_loop_arg(dd2_content_frame, state, 0, 0);
    return EXIT_SUCCESS;
#else
    while (state->running) {
        dd2_content_frame(state);
        dd2_window_wait();
    }
    const bool passed = !state->failed;
    dd2_content_close();
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
#endif
}
